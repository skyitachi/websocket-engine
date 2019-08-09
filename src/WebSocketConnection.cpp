//
// Created by skyitachi on 2019-07-31.
//
#include <ws/WebSocketConnection.h>
#include <boost/compute/detail/sha1.hpp>
#include <boost/beast/core/detail/base64.hpp>

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
    // TODO: handshake logic
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
    if (headers_.find("Upgrade") == headers_.end() || headers_["Upgrade"] != "websocket") {
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
    BOOST_LOG_TRIVIAL(debug) << " maybe websocket request come";
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
  
  // text, 默认不分片
  int WebSocketConnection::sendMessage(const std::string &data) {
    uint8_t control = 0xff;
    // set FIN
    control = control & 0x8f;
    // set opcode txt
    control = control & 0xf1;
    buf_.putByte(control);
    if (data.size() < 126) {
      buf_.putByte((uint8_t) data.size());
    } else if (data.size() <= 0xffff) {
      buf_.putByte(126);
      buf_.putUInt16((uint16_t)data.size());
    } else if ((uint64_t) data.size() > 0xfff) {
      buf_.putByte(127);
      buf_.putUInt64((uint64_t)data.size());
    }
    buf_.write(data.c_str(), data.size());
    size_t readable = buf_.readableBytes();
    int ret = conn_->send(buf_.peek(), readable);
    buf_.retrieve(readable);
    return ret;
  }
  
  // NOTE: 解析websocket data frame
  void WebSocketConnection::decode(Buffer& inputBuffer) {
  }
  
  void WebSocketConnection::parse(Buffer& inputBuffer) {
    if (status_ == INITIAL) {
      size_t nparsed = http_parser_execute(httpParserPtr.get(), &httpParserSettings_, inputBuffer.peek(), inputBuffer.readableBytes());
      inputBuffer.retrieve(nparsed);
    } else if (status_ == CONNECT) {
      decode(inputBuffer);
    }
  }
}