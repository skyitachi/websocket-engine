//
// Created by skyitachi on 2019-07-31.
//

#ifndef WEBSOCKET_ENGINE_WEBSOCKETCONNECTION_H
#define WEBSOCKET_ENGINE_WEBSOCKETCONNECTION_H

#include <ws/util.h>
#include <ws/TcpConnection.h>
#include <unordered_map>
#include <http_parser/http_parser.h>
#include <memory>

namespace ws {
  class WebSocketConnection:
    util::NoCopyable,
    public std::enable_shared_from_this<WebSocketConnection> {
    
  public:
    const int kInitialDecodeBufSize = 64;
    typedef std::shared_ptr<WebSocketConnection> WebSocketConnectionPtr;
    typedef std::function<void (const WebSocketConnectionPtr&)> WSConnectionCallback;
    typedef std::function<void (const std::string&& )> WSMessageCallback;
    typedef std::function<void (std::string&& )> WSPingCallback;
    typedef std::function<void (std::string&& )> WSPongCallback;
    
    enum Status {
      INITIAL,
      HANDSHAKE,
      CONNECT,
      CLOSING,
      CLOSE
    };
    
    WebSocketConnection(const TcpConnectionPtr& ptr): conn_(ptr), status_(INITIAL), decodeBuf_(kInitialDecodeBufSize) {
      httpParserPtr = std::make_unique<http_parser>();
      http_parser_init(httpParserPtr.get(), HTTP_REQUEST);
      initHttpParser();
    }
    
    void parse(Buffer &inputBuffer);
    
    void setHeaderValue(std::string&& value) {
      assert(!lastHeaderField_.empty());
      headers_.insert(std::make_pair(lastHeaderField_, std::move(value)));
      lastHeaderField_ = "";
    }
  
    void setLastHeaderField(std::string&& field) {
      lastHeaderField_ = std::move(field);
    }
    
    void onHeaderComplete();
    
    // TODO: callback 是如何使用move的
    void onConnection(WSConnectionCallback& cb) {
      connCb_ = std::move(cb);
    }
    
    void onClose(WSConnectionCallback& cb) {
      closeCb_ = std::move(cb);
    }
    
    void onMessage(WSMessageCallback&& cb) {
      wsMessageCallback_ = std::move(cb);
    }
    
    void onPing(WSPingCallback&& cb) {
      pingCallback_ = std::move(cb);
    }
    
    void onPong(WSPongCallback&& cb) {
      pongCallback_ = std::move(cb);
    }
    
    // send websocket frame
    int sendMessage(const std::string&);
    int ping();
    int pong();
    int close();

  private:
    const std::string computeAcceptKey(const std::string&);
    const TcpConnectionPtr& conn_;
    std::unique_ptr<http_parser> httpParserPtr;
    http_parser_settings httpParserSettings_;
    Status status_;
    std::unordered_map<std::string, std::string> headers_;
    std::string lastHeaderField_;
    WSConnectionCallback connCb_;
    WSConnectionCallback closeCb_;
    WSMessageCallback wsMessageCallback_;
    WSPingCallback pingCallback_;
    WSPongCallback pongCallback_;
   
    Buffer outputBuf_;
    Buffer decodeBuf_;
    byte fragmentedOpCode_;
    
    void handleConnection() {
      status_ = CONNECT;
      if (connCb_ != nullptr) {
        connCb_(shared_from_this());
      }
    }
    
    void handleClose() {
      if (closeCb_ != nullptr) {
        closeCb_(shared_from_this());
      }
    }
    
    void initHttpParser();
    void decode(Buffer &inputBuffer);
  };
  
  typedef std::shared_ptr<WebSocketConnection> WebSocketConnectionPtr;
  
}
#endif //WEBSOCKET_ENGINE_WEBSOCKETCONNECTION_H
