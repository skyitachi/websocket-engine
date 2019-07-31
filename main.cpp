//
// Created by skyitachi on 2019-07-29.
//

#include <uv.h>
#include <ws/WebsocketServer.h>
#include <boost/log/trivial.hpp>
#include <iostream>

using namespace ws;

int main() {
  
  WebSocketServer server(uv_default_loop());
  
  int ret = server.Listen("0.0.0.0", 3000);
  
  if (ret) {
    BOOST_LOG_TRIVIAL(error) << "server listen error " << uv_strerror(ret);
  }
}