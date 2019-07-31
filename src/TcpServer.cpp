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
    uv_loop_t* mainLoop = tcpServer->getMainLoop();
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(mainLoop, tcpServer->getNextId());
    tcpServer->addTcpConnection(conn);
    
    // NOTE: uv_accept放到一个线程里做
    if (!uv_accept(server, conn->stream())) {
      conn->setMessageCallback(tcpServer->messageCallback);
      conn->setConnectionCallback(tcpServer->connectionCallback);
      conn->setCloseCallback(std::bind(closeConnection, tcpServer, _1));
      
      if (!tcpServer->isSingleThread()) {
        uv_loop_t* workerLoop = tcpServer->getWorkerLoop();
        conn->attachToLoop(workerLoop);
        uv_async_t *task = (uv_async_t *)malloc(sizeof(uv_async_t));
        task->data = &conn;
        uv_async_init(workerLoop, task, onAsyncCb);
        uv_async_send(task);
      } else {
        conn->connectionEstablished();
      }
      conn->readStart();
    }

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
    return uv_run(loop_, UV_RUN_DEFAULT);
  }
}
