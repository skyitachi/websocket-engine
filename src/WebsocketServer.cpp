//
// Created by skyitachi on 2019-07-29.
//

#include <ws/WebsocketServer.h>
#include <boost/log/trivial.hpp>

namespace ws {
  
  void WebSocketServer::handleMessage(const TcpConnectionPtr& ptr, Buffer& buf) {
    BOOST_LOG_TRIVIAL(debug) << "receive client message";
    int id = ptr->id();
    if (wsConns_.find(id) == wsConns_.end()) {
      wsConns_[id] = std::make_shared<WebSocketConnection>(ptr);
    }
    BOOST_LOG_TRIVIAL(debug) << "conn id " << id << " send message";
    
    BOOST_LOG_TRIVIAL(debug) << "conn id " << id << " handle message: " << buf.readableBytes();
    auto connPtr = wsConns_[id];
    size_t nParsed = connPtr->parse(buf.peek(), buf.readableBytes());
    buf.retrieve(nParsed);
  }
  
  void WebSocketServer::handleConnection(const ws::TcpConnectionPtr &conn) {
    if (conn->disconnected()) {
      BOOST_LOG_TRIVIAL(debug) << "remove conn id " << conn->id() << " from websocket connections";
      wsConns_.erase(conn->id());
    }
  }
}
