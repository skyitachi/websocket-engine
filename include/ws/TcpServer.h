//
// Created by skyitachi on 2019/5/28.
//

#ifndef SOCKSPROXY_TCPSERVER_H
#define SOCKSPROXY_TCPSERVER_H

#include <uv.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <boost/log/trivial.hpp>
#include "TcpConnection.h"
#include "EventLoopThreadPool.h"

namespace ws {
  class TcpServer {
  public:
    
    TcpServer(uv_loop_t* loop): loop_(loop), tcp_(std::make_unique<uv_tcp_t>()), pool_(loop) {
      tcp_->data = this;
      uv_tcp_init(loop_, tcp_.get());
      id_ = 0;
      listened_ = false;
      fetched_ = false;
      // 默认使用baseLoop
      pool_.setThreadNums(0);
    }
    
    int listen(const std::string, int);
    
    void setMessageCallback(const MessageCallback& cb) {
      messageCallback = cb;
    }
    
    void setMessageCallback(MessageCallback&& cb) {
      messageCallback = std::move(cb);
    }
    
    void setConnectionCallback(const ConnectionCallback& cb) {
      connectionCallback = cb;
    }
    
    void setConnectionCallback(ConnectionCallback&& cb) {
      connectionCallback = std::move(cb);
    }
    
    int getNextId() {
      return id_++;
    }
    
    uv_stream_t* stream() {
      return (uv_stream_t* )tcp_.get();
    }

    int get_native_fd() {
      assert(listened_);
      if (fetched_) {
        return fd_;
      }
      uv_fileno((uv_handle_t *) tcp_.get(), static_cast<uv_os_fd_t* >(&fd_));
      fetched_ = true;
      return fd_;
    }

    int native_accept() {
      sockaddr_in peer_addr;
      socklen_t peer_addr_size;
      return accept(get_native_fd(), (sockaddr*)&peer_addr, &peer_addr_size);
    }
    
    uv_loop_t* getWorkerLoop() {
      return pool_.getNextLoop();
    }
    
    uv_loop_t* getMainLoop() {
      return loop_;
    }
    // 此处应该传入TcpConnectionPtr&
    void addTcpConnection(const TcpConnectionPtr& conn) {
      connectionMap_[conn->id()] = conn;
    }
    
    ~TcpServer() {
      auto uv_tcp = tcp_.release();
      uv_close((uv_handle_t* )uv_tcp, [](uv_handle_t* handle) {
        delete handle;
      });
    }
    
    void setNumberThreads(int nums) {
      pool_.setThreadNums(nums);
    }
    
    bool isSingleThread() {
      return pool_.isSingleThread();
    }
    
    
    MessageCallback messageCallback;
    ConnectionCallback connectionCallback;
    
    // public ?
    std::unordered_map<int, TcpConnectionPtr> connectionMap_;
  
  private:
    int id_;
    uv_loop_t* loop_;
    std::unique_ptr<uv_tcp_t> tcp_;
    EventLoopThreadPool pool_;
    bool listened_;
    int fd_;
    bool fetched_;

  };
  
}


#endif //SOCKSPROXY_TCPSERVER_H
