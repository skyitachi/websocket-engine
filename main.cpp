//
// Created by skyitachi on 2019-07-29.
//

#include <uv.h>
#include <ws/WebSocketServer.h>
#include <boost/log/trivial.hpp>
#include <iostream>

using namespace ws;

int main() {
  
  WebSocketServer server(uv_default_loop());
  
  server.onConnection([](const WebSocketConnectionPtr& conn) {
    BOOST_LOG_TRIVIAL(info) << "websocket connection established";
    
    conn->onMessage([conn](String&& message, bool isBinary) {
      BOOST_LOG_TRIVIAL(info) << "receive message from client: " << message.size() << " content is " << message.c_str();
      conn->sendMessage(std::move(message), isBinary);
    });
    
    conn->onPing([](String&& message) {
//      BOOST_LOG_TRIVIAL(info) << "receive ping";
    });
    
    conn->onPong([](String&& message) {
//      BOOST_LOG_TRIVIAL(info) << "receive pong";
    });
  });
  
  int ret = server.Listen("0.0.0.0", 3000);
  
  if (ret) {
    BOOST_LOG_TRIVIAL(error) << "server listen error " << uv_strerror(ret);
  }
}