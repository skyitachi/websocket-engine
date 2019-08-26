//
// Created by skyitachi on 2019-07-31.
//
#include <ws/WebSocketConnection.h>
#include <boost/compute/detail/sha1.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

namespace ws {
  const char *BAD_REQUEST = "HTTP/1.1 400 Bad Request\r\n\r\n";
  const std::string UPGRADE_RESPONSE = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n";
  const char *MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  
  static int on_url(http_parser *httpParser, const char *p, size_t len) {
    WebSocketConnection *conn = (WebSocketConnection *) httpParser->data;
    assert(conn);
    
    BOOST_LOG_TRIVIAL(info) << "receive url :" << std::string(p, len);
    return 0;
  }
  
  static int on_header_field(http_parser *httpParser, const char *f, size_t len) {
    WebSocketConnection *conn = (WebSocketConnection *) httpParser->data;
    assert(conn);
    conn->setLastHeaderField(std::string(f, len));
//    BOOST_LOG_TRIVIAL(info) << "receive header field: " << std::string(f, len);
    return 0;
  }
  
  static int on_header_value(http_parser *httpParser, const char *v, size_t len) {
    WebSocketConnection *conn = (WebSocketConnection *) httpParser->data;
    assert(conn);
    conn->setHeaderValue(std::string(v, len));
//    BOOST_LOG_TRIVIAL(info) << "receive header value: " << std::string(v, len);
    return 0;
  }
  
  static int on_header_complete(http_parser *httpParser) {
    WebSocketConnection *conn = (WebSocketConnection *) httpParser->data;
    assert(conn);
    conn->onHeaderComplete();
    return 0;
  }
  
  static int on_status_cb(http_parser *httpParser) {
    return 0;
  }
  
  static int on_data_cb(http_parser *httpParser, const char *v, size_t len) {
    return 0;
  }
  
  void WebSocketConnection::onHeaderComplete() {
    if (httpParserPtr->method != HTTP_GET) {
      BOOST_LOG_TRIVIAL(debug) << "conn id " << conn_->id() << " doesn't receive get request";
      conn_->send(BAD_REQUEST);
      conn_->close();
      return;
    }
    //  至少HTTP 1.1
    if (httpParserPtr->http_major <= 1 && httpParserPtr->http_minor < 1) {
      conn_->send(BAD_REQUEST);
      conn_->close();
      return;
    }
    // TODO: check header要不区分大小写
    if (headers_.find("Host") == headers_.end() || headers_["Host"].empty()) {
      conn_->send(BAD_REQUEST);
      conn_->close();
      return;
    }
    if (headers_.find("Upgrade") == headers_.end()) {
      conn_->send(BAD_REQUEST);
      conn_->close();
      return;
    }
    std::string newUpgrade = boost::to_lower_copy<std::string>(headers_["Upgrade"]);
    if (newUpgrade != "websocket") {
      conn_->send(BAD_REQUEST);
      conn_->close();
      return;
    }
    if (headers_.find("Connection") == headers_.end() || headers_["Connection"] != "Upgrade") {
      conn_->send(BAD_REQUEST);
      conn_->close();
      return;
    }
    if (headers_.find("Sec-WebSocket-Version") == headers_.end() || headers_["Sec-WebSocket-Version"] != "13") {
      conn_->send(BAD_REQUEST);
      conn_->close();
      return;
    }
    auto it = headers_.find("Sec-WebSocket-Key");
    if (it == headers_.end() || it->second.size() != 24) {
      conn_->send(BAD_REQUEST);
      conn_->close();
      return;
    }
    const std::string acceptKey = computeAcceptKey(headers_["Sec-WebSocket-Key"]);
    BOOST_LOG_TRIVIAL(debug) << "base64 string: " << acceptKey;
    conn_->send(UPGRADE_RESPONSE + "Sec-WebSocket-Accept: " + acceptKey + "\r\n\r\n");
    handleConnection();
    // emit connect event
  }
 
  
  const std::string WebSocketConnection::computeAcceptKey(const std::string &nonce) {
    const std::string key = nonce + MAGIC;
    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(key.data(), key.size());
    unsigned int hash[5];
    sha1.get_digest(hash);
    
    for(int i = 0; i < 5; i++) {
      hash[i] = htonl(hash[i]);
    }
    const uint8_t* sha1_bytes = reinterpret_cast<uint8_t *>(hash);
    return boost::beast::detail::base64_encode(sha1_bytes, 20);
  }
  
  void WebSocketConnection::initHttpParser() {
    httpParserPtr->data = this;
    httpParserSettings_.on_message_begin = on_status_cb;
    httpParserSettings_.on_header_field = on_header_field;
    httpParserSettings_.on_header_value = on_header_value;
    httpParserSettings_.on_url = on_url;
    httpParserSettings_.on_headers_complete = on_header_complete;
    httpParserSettings_.on_body = on_data_cb;
    httpParserSettings_.on_message_complete = on_status_cb;
  }
  
  void WebSocketConnection::writeHeader(uint64_t payLoadLength, byte opcode) {
    assert(outputBuf_.empty());
    // fin is true
    byte control = 0x8f;
    // set opcode
    control = control & (0x80 | opcode);
    outputBuf_.putByte(control);
    if (payLoadLength < 126) {
      outputBuf_.putByte(static_cast<uint8_t>(payLoadLength));
    } else if (payLoadLength <= 0xffff) {
      outputBuf_.putByte(126);
      outputBuf_.putUInt16((static_cast<uint16_t>(payLoadLength)));
    } else if (payLoadLength > 0xfff) {
      outputBuf_.putByte(127);
      outputBuf_.putUInt64(payLoadLength);
    }
  }
  
  // scatter io, header和data的分离
  int WebSocketConnection::sendMessage(ws::String &&data, bool isBinary = false) {
    assert(outputBuf_.empty());
    // text
    if (isBinary) {
      writeHeader(data.size(), 2);
    } else {
      writeHeader(data.size(), 1);
    }
    return conn_->send(outputBuf_, std::move(data));
  }
  
  int WebSocketConnection::sendMessage(const String& data, bool isBinary = false) {
    if (isBinary) {
      writeHeader(data.size(), 2);
    } else {
      writeHeader(data.size(), 1);
    }
    outputBuf_.write(data.c_str(), data.size());
    return conn_->send(outputBuf_);
    
  }
  
  // NOTE: 客户端的必须要mask
  // TODO: 如何让一个函数参数既支持右值又支持左值
  int WebSocketConnection::ping(const String &message) {
    if (message.size() >= 126) {
      close(PROTOCOL_ERROR);
      return 0;
    }
    byte control = 0xff;
    control = control & (byte) 0x8f;
    control = control & (byte) 0xf9;
    outputBuf_.putByte(control);
    outputBuf_.putByte(static_cast<byte>(message.size()));
    outputBuf_.writeString(message);
    size_t readable = outputBuf_.readableBytes();
    int ret = conn_->send(outputBuf_.peek(), readable);
    outputBuf_.retrieve(readable);
    return ret;
  }
  
  // NOTE: no need to use rvalue reference
  int WebSocketConnection::pong(const String &message) {
    if (message.size() >= 126) {
      close(PROTOCOL_ERROR);
      return 0;
    }
    byte control = 0xff;
    control = control & (byte) 0x8f;
    control = control & (byte) 0xfa;
    outputBuf_.putByte(control);
    outputBuf_.putByte(static_cast<byte>(message.size()));
    outputBuf_.writeString(message);
    size_t readable = outputBuf_.readableBytes();
    int ret = conn_->send(outputBuf_.peek(), readable);
    outputBuf_.retrieve(readable);
    return ret;
  }
  
  int WebSocketConnection::close(StatusCode code) {
    if (outputBuf_.readableBytes() > 0) {
      outputBuf_.clear();
    }
    outputBuf_.putByte(0x88);
    outputBuf_.putByte(0x02);
    outputBuf_.putUInt16(code);
    status_ = CLOSING;
    if (code != NORMAL_CLOSE) {
      // 首先停止接收数据
      conn_->stopRead();
    }
    return conn_->send(outputBuf_.peek(), 4);
  }
  
  // NOTE: 解析websocket data frame, 一般而言都是客户端的消息
  // NOTE: 先使用stack variable
  // NOTE: 可以处理tcp分片
  // NOTE: 如果一次parse不了就先unread
  // NOTE: inputBuffer有可能有连续多帧的websocket frame
  void WebSocketConnection::decode(Buffer &inputBuffer) {
    // NOTE: 有状态的
    if (!headerRead_) {
      size_t readSize = 0;
      byte firstByte = inputBuffer.readTypedNumber<byte>();
      // RSV must be zero
      if (firstByte & static_cast<byte>(0x70)) {
        close(PROTOCOL_ERROR);
        return;
      }
      readSize += 1;
      isFin_ = false;
      if (firstByte >> 7) {
        isFin_ = true;
      }
      opcode_ = firstByte & (byte)0x0f;
      if (inputBuffer.readableBytes() < 1) {
        headerRead_ = false;
        wanted_ = 2;
        inputBuffer.unread(readSize);
        return;
      }
      byte secondByte = inputBuffer.readTypedNumber<byte>();
      readSize += 1;
      // NOTE: 客户端的必须mask
      masked_ = (secondByte >> 7) == 1;
      payloadLength_ = (uint64_t)(secondByte & (byte)0x7f);
      wanted_ = payloadLength_;
      if (payloadLength_ == 126) {
        if (inputBuffer.readableBytes() < 2) {
          headerRead_ = false;
          wanted_ = 4;
          inputBuffer.unread(readSize);
          return;
        }
        payloadLength_ = inputBuffer.readTypedNumber<uint16_t >();
        readSize += 2;
        BOOST_LOG_TRIVIAL(info) << "small payloadLength_: " << payloadLength_;
      } else if (payloadLength_ == 127) {
        if (inputBuffer.readableBytes() < 8) {
          headerRead_ = false;
          wanted_ = 10;
          inputBuffer.unread(readSize);
          return;
        }
        payloadLength_ = inputBuffer.readTypedNumber<uint64_t >();
        BOOST_LOG_TRIVIAL(info) << "big payloadLength_: " << payloadLength_;
        readSize += 8;
      }
      if (masked_) {
        if (inputBuffer.readableBytes() < 4) {
          headerRead_ = false;
          wanted_ = readSize + 4;
          inputBuffer.unread(readSize);
          return;
        } else {
          inputBuffer.read(maskKey_, 4);
        }
      }
    }
    headerRead_ = true;
    BOOST_LOG_TRIVIAL(info) << "conn id: " << conn_->id() << " headers parsed, payload size:  " << payloadLength_;
    if (payloadLength_ > inputBuffer.readableBytes()) {
      // 处理tcp分片
      BOOST_LOG_TRIVIAL(debug) << "conn id: " << conn_->id() << " found tcp splits";
      split_ = true;
      wanted_ = payloadLength_;
      return;
    }
    size_t originSize = decodeBuf_.readableBytes();

    decodeBuf_.write(inputBuffer, payloadLength_);
    if (masked_) {
      auto readEnd = decodeBuf_.peek() + decodeBuf_.readableBytes();
      char *begin = const_cast<char *>(decodeBuf_.peek() + originSize);
      for (auto it = begin; it != readEnd; it++) {
        auto idx = it - decodeBuf_.peek() - originSize;
        *it = (uint8_t)(*it) ^ (uint8_t) maskKey_[idx % 4];
      }
    }
    
    switch (opcode_) {
      case 0:
        // last frame
        if (fragmentedOpCode_ == 0) {
          // 没收到fragment, 但是对端又以fragment的形式发送, 这是认为是非法的.
          close(PROTOCOL_ERROR);
          return;
        }
        if (isFin_) {
          if (wsMessageCallback_ != nullptr) {
            if (fragmentedOpCode_ == 0x01 || fragmentedOpCode_ == 0x02) {
              wsMessageCallback_(decodeBuf_.readString(), fragmentedOpCode_ == 0x02);
            }
          } else {
            BOOST_LOG_TRIVIAL(debug) << "doesn't has messagecallback";
            decodeBuf_.retrieve(decodeBuf_.readableBytes());
          }
          fragmentedOpCode_ = 0;
          clearDecodeStatus();
        } else {
          BOOST_LOG_TRIVIAL(debug) << "decode receive fragment";
          // NOTE: 开始新帧的解析
          clearDecodeStatus();
          // fragment
        }
        break;
      case 1:
      case 2:
        // Text
        if (fragmentedOpCode_ != 0) {
          close(PROTOCOL_ERROR);
          return;
        }
        if (isFin_) {
          if (wsMessageCallback_ != nullptr) {
            wsMessageCallback_(decodeBuf_.readString(), opcode_ == 2);
          } else {
            decodeBuf_.retrieve(decodeBuf_.readableBytes());
          }
        } else {
          // NOTE: 分片
          fragmentedOpCode_ = opcode_;
          BOOST_LOG_TRIVIAL(debug) << "text message fragment found, opcode_: " << opcode_ << " fragmentedOpCode_: " << fragmentedOpCode_;
        }
        clearDecodeStatus();
        break;
      case 0x08: {
        // close
        BOOST_LOG_TRIVIAL(debug) << "receive close frame";
        if (status_ == CONNECT) {
          close(NORMAL_CLOSE);
          status_ = CLOSED;
          conn_->close();
        }
        break;
      }
      case 0x09:
        // PING
        // NOTE: must not be fragment
        if (!isFin_) {
          close(PROTOCOL_ERROR);
          return;
        }
        BOOST_LOG_TRIVIAL(debug) << "receive ping frame with length " << payloadLength_;
        // ping frame may have application data and can be injected to fragmented frame
        if (pingCallback_ != nullptr) {
          pingCallback_(String(decodeBuf_.peek() + originSize, payloadLength_));
        }
        pong(String(decodeBuf_.peek() + originSize, payloadLength_));
        if (originSize != 0) {
          decodeBuf_.unwrite(payloadLength_);
        }
        clearDecodeStatus();
        break;
      case 0x0a:
        // pong
        if (!isFin_) {
          close(PROTOCOL_ERROR);
          return;
        }
        BOOST_LOG_TRIVIAL(info) << "receive pong frame with length " << payloadLength_;
        if (pongCallback_ != nullptr) {
          pongCallback_(String(decodeBuf_.peek() + originSize, payloadLength_));
        }
        if (originSize != 0) {
          decodeBuf_.unwrite(payloadLength_);
        }
        clearDecodeStatus();
        break;
      default:
        close(PROTOCOL_ERROR);
        return;
    }
    if (inputBuffer.readableBytes() > 0) {
      decode(inputBuffer);
    }
  }
  
  void WebSocketConnection::parse(Buffer &inputBuffer) {
    if (inputBuffer.readableBytes() < wanted_) {
      BOOST_LOG_TRIVIAL(debug) << "wanted_ size: " << wanted_;
      // NOTE: 保证header先parse完毕
      // NOTE: 暂时不消费, 等攒够了再消费
      return;
    }
    if (status_ == INITIAL) {
      size_t nparsed = http_parser_execute(httpParserPtr.get(), &httpParserSettings_, inputBuffer.peek(), inputBuffer.readableBytes());
      inputBuffer.retrieve(nparsed);
    } else if (status_ == CONNECT || status_ == CLOSING) {
      decode(inputBuffer);
    }
  }
}
