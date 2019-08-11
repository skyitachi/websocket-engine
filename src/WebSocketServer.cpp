//
// Created by skyitachi on 2019-07-29.
//

#include <ws/WebSocketServer.h>
#include <boost/log/trivial.hpp>

namespace ws {
  
  void WebSocketServer::handleMessage(const TcpConnectionPtr& ptr, Buffer& buf) {
    BOOST_LOG_TRIVIAL(debug) << "receive client message";
    int id = ptr->id();
    if (wsConns_.find(id) == wsConns_.end()) {
      wsConns_[id] = std::make_shared<WebSocketConnection>(ptr);
      if (connCb_ != nullptr) {
        wsConns_[id]->onConnection(connCb_);
      }
      if (closeCb_ != nullptr) {
        wsConns_[id]->onClose(closeCb_);
      }
    }
    BOOST_LOG_TRIVIAL(debug) << "conn id " << id << " handle message: " << buf.readableBytes();
    auto connPtr = wsConns_[id];
    connPtr->parse(buf);
  }
  
  void WebSocketServer::handleConnection(const ws::TcpConnectionPtr &conn) {
    if (conn->disconnected()) {
      BOOST_LOG_TRIVIAL(debug) << "remove conn id " << conn->id() << " from websocket connections";
      wsConns_.erase(conn->id());
    }
  }
}