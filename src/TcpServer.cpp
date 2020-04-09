//
// Created by skyitachi on 2019/5/28.
//

#include <ws/TcpServer.h>

namespace ws {

  using namespace std::placeholders;
  
  static void onAsyncCb(uv_async_t* asyncHandle) {
    auto conn = *static_cast<TcpConnectionPtr*> (asyncHandle->data);
    assert(conn);
    conn->connectionEstablished();
    free(asyncHandle);
  }
  
  static void closeConnection(TcpServer* server, const TcpConnectionPtr& conn) {
    BOOST_LOG_TRIVIAL(info) << "delete conn " << conn->id() << " from connectionMap";
    assert(server->connectionMap_.find(conn->id()) != server->connectionMap_.end());
    server->connectionMap_.erase(conn->id());
  }
  
  static void on_uv_connection(uv_stream_t* server, int status) {
    auto tcpServer = (TcpServer*) server->data;
    assert(tcpServer);
    if (status < 0) {
      // TODO: 错误处理
      return;
    }
    TcpConnectionPtr conn;
    // NOTE: uv_accept放到一个线程里做
    // TODO: 改用accept, 使用uv_tcp_open
    if (tcpServer->isSingleThread()) {
      uv_loop_t* mainLoop = tcpServer->getMainLoop();
      conn = std::make_shared<TcpConnection>(mainLoop, tcpServer->getNextId());
      tcpServer->addTcpConnection(conn);
      if (!uv_accept(server, conn->stream())) {}
    } else {
      int connFd = tcpServer->native_accept();
      conn = std::make_shared<TcpConnection>(tcpServer->getWorkerLoop(), connFd, tcpServer->getNextId());
      tcpServer->addTcpConnection(conn);
    }
    conn->setMessageCallback(tcpServer->messageCallback);
    conn->setConnectionCallback(tcpServer->connectionCallback);
    conn->setCloseCallback(std::bind(closeConnection, tcpServer, _1));
    conn->connectionEstablished();
    conn->startRead();
  }

  int TcpServer::listen(const std::string host, int port) {
    sockaddr_in sockaddrIn;
    uv_ip4_addr(host.c_str(), port, &sockaddrIn);
    // TODO 错误处理
    uv_tcp_bind(tcp_.get(), (const sockaddr*)&sockaddrIn, 0);
    int ret = uv_listen(stream(), 1024, on_uv_connection);
    if (ret) {
      return ret;
    }
    listened_ = true;
    return uv_run(loop_, UV_RUN_DEFAULT);
  }
}
