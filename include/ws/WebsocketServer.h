//
// Created by skyitachi on 2019-07-29.
//

#ifndef WEBSOCKET_ENGINE_WEBSOCKETSERVER_H
#define WEBSOCKET_ENGINE_WEBSOCKETSERVER_H

#include "TcpServer.h"
#include "../http_parser/http_parser.h"

#include <uv.h>


namespace ws {
  using namespace std::placeholders;
  
  class WebSocketServer: public TcpServer {
  public:
    enum Status {
      INITIAL,
      HANDSHAKE,
      CONNECT,
      CLOSE
    };
    
    WebSocketServer(uv_loop_t* loop): TcpServer(loop), status_(INITIAL) {
      httpParserPtr = std::make_unique<http_parser>();
      http_parser_init(httpParserPtr.get(), HTTP_REQUEST);
  
      initHttpParser();
      setMessageCallback(std::bind(&WebSocketServer::handleMessage, this, _1, _2));
    }
    
    void setHeaderValue(std::string value) {
      assert(!lastHeaderField_.empty());
      headers_[lastHeaderField_] = value;
      lastHeaderField_ = "";
    }
    
    void setLastHeaderField(std::string field) {
      lastHeaderField_ = field;
    }
    
    void onHeaderComplete();

  private:
    void handleMessage(const TcpConnectionPtr&, Buffer& );
    void initHttpParser();
    std::unique_ptr<http_parser> httpParserPtr;
    http_parser_settings httpParserSettings_;
    Status status_;
    std::unordered_map<std::string, std::string> headers_;
    std::string lastHeaderField_;
  };
  
}

#endif //WEBSOCKET_ENGINE_WEBSOCKETSERVER_H
