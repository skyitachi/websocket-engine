//
// Created by skyitachi on 2019-07-31.
//

#ifndef WEBSOCKET_ENGINE_WEBSOCKETCONNECTION_H
#define WEBSOCKET_ENGINE_WEBSOCKETCONNECTION_H

#include <ws/util.h>
#include <ws/TcpConnection.h>
#include <ws/String.h>
#include <unordered_map>
#include <http_parser/http_parser.h>
#include <memory>

namespace ws {
  enum StatusCode {
    NORMAL_CLOSE = 1000,
    GOING_AWAY,
    PROTOCOL_ERROR,
    UNEXPECT_PAYLOAD
  };
  class WebSocketConnection:
    util::NoCopyable,
    public std::enable_shared_from_this<WebSocketConnection> {
    
  public:
    const size_t kInitialDecodeBufSize = 64;
    const size_t kMinPacketSize = 2; // control frame and payload size
    typedef std::shared_ptr<WebSocketConnection> WebSocketConnectionPtr;
    typedef std::function<void (const WebSocketConnectionPtr&)> WSConnectionCallback;
    typedef std::function<void()> WSCloseCallback;
    typedef std::function<void (String&&, bool)> WSMessageCallback;
    typedef std::function<void (String&&)> WSPingCallback;
    typedef std::function<void (String&& )> WSPongCallback;
    
    enum Status {
      INITIAL,
      HANDSHAKE,
      CONNECT,
      CLOSING,
      CLOSED
    };
    
    WebSocketConnection(const TcpConnectionPtr& ptr): conn_(ptr), status_(INITIAL), decodeBuf_(kInitialDecodeBufSize) {
      httpParserPtr = std::make_unique<http_parser>();
      http_parser_init(httpParserPtr.get(), HTTP_REQUEST);
      initHttpParser();
    }
    
    void parse(Buffer &inputBuffer);
   
    // 这里是可以使用右值引用的参数的
    void setHeaderValue(std::string&& value) {
      assert(!lastHeaderField_.empty());
      headers_.insert(std::make_pair(lastHeaderField_, std::move(value)));
      lastHeaderField_ = "";
    }
  
    void setLastHeaderField(std::string&& field) {
      lastHeaderField_ = std::move(field);
    }
    
    void onHeaderComplete();
   
    // NOTE: 因为这里的callback是有server那里传入的，要负责多个连接的使用
    // 所以不能使用move
    void onConnection(const WSConnectionCallback& cb) {
      // NOTE: 这里不能move
      connCb_ = cb;
    }
    
    void onClose(const WSCloseCallback& cb) {
      // NOTE: 这里不能move
      closeCb_ = cb;
      conn_->setCloseCallback(std::bind(&WebSocketConnection::handleClose, this));
    }
    
    void onClose(WSCloseCallback&& cb) {
      closeCb_ = std::move(cb);
      conn_->setCloseCallback(std::bind(&WebSocketConnection::handleClose, this));
    }
    
    void onMessage(const WSMessageCallback& cb) {
      // 不能用move
      wsMessageCallback_ = cb;
    }
    
    void onMessage(WSMessageCallback&& cb) {
      wsMessageCallback_ = std::move(cb);
    }
    
    // 如果要使用T&&版本的话至少要提供一个const T&的重载
    void onPing(WSPingCallback&& cb) {
      pingCallback_ = std::move(cb);
    }

    void onPing(const WSPingCallback& cb) {
      pingCallback_ = cb;
    }
    
    void onPong(WSPongCallback&& cb) {
      pongCallback_ = std::move(cb);
    }

    void onPong(const WSPongCallback&& cb) {
      pongCallback_ = cb;
    }
    
    // send websocket frame
    int sendMessage(String &&, bool);
    int sendMessage(const String&, bool);
    int ping(const String& message);
    int pong(const String& message);
    int close(StatusCode code);

  private:
    const std::string computeAcceptKey(const std::string&);
    const TcpConnectionPtr& conn_;
    std::unique_ptr<http_parser> httpParserPtr;
    http_parser_settings httpParserSettings_;
    Status status_;
    std::unordered_map<std::string, std::string> headers_;
    std::string lastHeaderField_;
    WSConnectionCallback connCb_;
    WSCloseCallback closeCb_;
    WSMessageCallback wsMessageCallback_;
    WSPingCallback pingCallback_;
    WSPongCallback pongCallback_;
   
    Buffer outputBuf_;
    Buffer decodeBuf_;
    byte fragmentedOpCode_ = 0;
    // 是否把整个websocket的header读完
    bool headerRead_ = false;
    // NOTE: 希望至少有多少个字节
    uint64_t wanted_ = 0;
    // 处理tcp分片
    bool split_;
    bool masked_;
    uint64_t payloadLength_;
    byte opcode_;
    bool isFin_;
    char maskKey_[4];
    
    void writeHeader(uint64_t payLoadLength, byte opcode);
    void handleConnection() {
      status_ = CONNECT;
      if (connCb_ != nullptr) {
        connCb_(shared_from_this());
      }
    }
    
    void handleClose() {
      status_ = CLOSED;
      if (closeCb_ != nullptr) {
        closeCb_();
      }
    }
    
    void clearDecodeStatus() {
      wanted_ = 0;
      headerRead_ = false;
    }
    
    void initHttpParser();
    void decode(Buffer &inputBuffer);
  };
  
  typedef std::shared_ptr<WebSocketConnection> WebSocketConnectionPtr;
  
}
#endif //WEBSOCKET_ENGINE_WEBSOCKETCONNECTION_H
