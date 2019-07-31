//
// Created by skyitachi on 2019-07-29.
//

#ifndef WEBSOCKET_ENGINE_WEBSOCKETSERVER_H
#define WEBSOCKET_ENGINE_WEBSOCKETSERVER_H

#include "TcpServer.h"
#include "../http_parser/http_parser.h"
#include "WebSocketConnection.h"

#include <uv.h>
#include <map>


namespace ws {
  using namespace std::placeholders;
  
  class WebSocketServer: private TcpServer {
  public:
    
    WebSocketServer(uv_loop_t* loop): TcpServer(loop) {
      setMessageCallback(std::bind(&WebSocketServer::handleMessage, this, _1, _2));
      setConnectionCallback(std::bind(&WebSocketServer::handleConnection, this, _1));
    }
    
    int Listen(const std::string& host, int port) {
      return listen(host, port);
    }
    
  private:
    void handleMessage(const TcpConnectionPtr&, Buffer&);
    void handleConnection(const TcpConnectionPtr&);
    std::map<int, WebSocketConnectionPtr> wsConns_;
  };
  
}

#endif //WEBSOCKET_ENGINE_WEBSOCKETSERVER_H
