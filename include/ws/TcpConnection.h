//
// Created by skyitachi on 2019-05-27.
//

#ifndef SOCKSPROXY_TCPCONNECTION_H
#define SOCKSPROXY_TCPCONNECTION_H

#include <uv.h>
#include <memory>
#include <functional>
#include <boost/log/trivial.hpp>
#include <string>
#include "util.h"
#include "Buffer.h"
#include "String.h"

namespace ws {
class TcpConnection:
    ws::util::NoCopyable,
    public std::enable_shared_from_this<TcpConnection> {
    static const int kBufferLen = 4096;
    
  public:
    typedef std::unique_ptr<uv_tcp_t> TcpUniquePtr;
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    typedef std::function<void (const TcpConnectionPtr& )> ConnectionCallback;
    typedef std::function<void (const TcpConnectionPtr&, Buffer&)> MessageCallback;
    typedef std::function<void (const TcpConnectionPtr&) > CloseCallback;
    typedef std::function<void (const TcpConnectionPtr&) > WriteCompleteCallback;
    
    enum StateE {
      kConnecting, kConnected, kDisconnected, kDisconnecting, kWritting
    };
    
    TcpConnection(uv_loop_t *loop, int id) : loop_(loop), id_(id), tcp_(std::make_unique<uv_tcp_t>()), buf(kBufferLen) {
      tcp_->data = this;
      uv_tcp_init(loop_, tcp_.get());
      setState(kConnecting);
    }

    TcpConnection(uv_loop_t* loop, int fd, int id): TcpConnection(loop, id) {
      uv_tcp_open(tcp_.get(), fd);
    }
    
    explicit TcpConnection(uv_loop_t* loop): TcpConnection(loop, 0) {}
    
    TcpConnection(uv_loop_t* loop, TcpUniquePtr && ptr, int id):
      loop_(loop), id_(id) {
      // NOTE: 需要用move
      tcp_ = std::move(ptr);
      // 由于是 move 过来的默认已经初始化过了
      // NOTE: change context to this
      tcp_->data = this;
      setState(kConnecting);
    }
    
    TcpConnection(uv_loop_t* loop, TcpUniquePtr&& ptr):
      TcpConnection(loop, std::move(ptr), 0) {}
  
    // NOTE: TcpConnection的callback可以有右值版本
    void setCloseCallback(const CloseCallback& cb) {
      closeCallback_ = cb;
    }
    
    void setCloseCallback(CloseCallback&& cb) {
      closeCallback_ = std::move(cb);
    }
    
    // NOTE: 由于connection callback是由TcpServer绑定的，不需要有右值版本
    void setConnectionCallback(const ConnectionCallback& cb) {
      connectionCallback_ = cb;
    }
    
    void setMessageCallback(const MessageCallback& cb) {
      messageCallback_ = cb;
    }
    
    void setMessageCallback(MessageCallback&& cb) {
      messageCallback_ = std::move(cb);
    }
    
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
      writeCompleteCallback_ = cb;
    }
    
    void setWriteCompleteCallback(WriteCompleteCallback&& cb) {
      writeCompleteCallback_ = std::move(cb);
    }
    
    std::string getPeerAddress() {
      assert(state_ == kConnected);
      sockaddr_storage storage;
      int len;
      uv_tcp_getpeername(tcp_.get(), (sockaddr*)&storage, &len);
      return formatAddr(&storage);
    }
    
    std::string getLocalAddress() {
      assert(state_ == kConnected);
      sockaddr_storage storage;
      int len;
      uv_tcp_getsockname(tcp_.get(), (sockaddr*)&storage, &len);
      return formatAddr(&storage);
    }
    
    uv_stream_t* stream() {
      return (uv_stream_t *) tcp_.get();
    }
    
    void connectionEstablished() {
      setState(kConnected);
      if (connectionCallback_) {
        connectionCallback_(shared_from_this());
      }
    }
  
    // shutdown
    int shutdown();
    
    // 主动关闭链接
    void close();
    int startRead();
    int stopRead();
    
    void handleMessage(ssize_t len) {
      bytesRead_ += len;
      if (messageCallback_) {
        messageCallback_(shared_from_this(), buf);
      }
    }
    
    void handleWrite(ssize_t len) {
      bytesWritten_ += len;
      if (writeCompleteCallback_) {
        BOOST_LOG_TRIVIAL(debug) << "in the writeCompleteCallback";
        writeCompleteCallback_(shared_from_this());
      }
    }
    
    void handleClose();
    
    ~TcpConnection() {
      auto raw = tcp_.release();
      uv_close((uv_handle_t *) raw, [](uv_handle_t* handle) {
        delete handle;
      });
    }
    
    int id() {
      return id_;
    }
    
    Buffer buf;
    Buffer outputBuf;
    bool isLastWrite = false;
 
    void setState(StateE state) {
      state_ = state;
    }
    
    int send(const char *, size_t);
    
    int send(Buffer& buf) {
      size_t readable = buf.readableBytes();
      int ret = send(buf.peek(), buf.readableBytes());
      buf.retrieve(readable);
      return ret;
    }
    
//    int send(std::vector<Buffer&>);
    
    // Note: 非常丑陋的api
    int send(Buffer&, String&&);
//    int send(const std::string& data) {
//      return send(data.c_str(), (size_t)data.size());
//    }
    int send(const std::string& data) {
      return send(data.c_str(), static_cast<size_t>(data.size()));
    }
//    int send(String&&);
    
    bool connected() const {
      return state_ == kConnected;
    }
    
    bool disconnected() {
      return state_ == kDisconnected;
    }
    
    int bytesRead() {
      return bytesRead_;
    }
    
    int bytesWritten() {
      return bytesWritten_;
    }
    
    size_t lastWrite() {
      return lastWrite_;
    }
    
    void resetLastWrite() {
      lastWrite_ = 0;
    }
    
    void attachToLoop(uv_loop_t* loop);
    
  private:
    uv_loop_t *loop_;
    std::unique_ptr<uv_tcp_t> tcp_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    int id_;
    StateE state_;
    int bytesRead_ = 0;
    int bytesWritten_ = 0;
    size_t lastWrite_;

    std::string formatAddr(const sockaddr_storage* storage) {
      const sockaddr_in *sockaddrIn = (const sockaddr_in*) storage;
      uint16_t port = ntohs(sockaddrIn->sin_port);
      if (storage->ss_family == AF_INET6) {
        // ipv6
        char decoded[46];
        uv_ip6_name((sockaddr_in6*)storage, decoded, sizeof(decoded));
        // port
        return std::string(decoded) + ":" + std::to_string(port);
      }
      char decoded[16];
      uv_ip4_name((sockaddr_in*)storage, decoded, sizeof(decoded));
      return std::string(decoded) + ":" + std::to_string(port);
    }
    
    int writeToUv(const uv_buf_t*, unsigned int);
  };
  
  typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
  typedef std::function<void (const TcpConnectionPtr& )> ConnectionCallback;
  typedef std::function<void (const TcpConnectionPtr&, Buffer&)> MessageCallback;
  typedef std::function<void (const TcpConnectionPtr&) > CloseCallback;
  typedef std::function<void (const TcpConnectionPtr&) > WriteCompleteCallback;
}
#endif //SOCKSPROXY_TCPCONNECTION_H
