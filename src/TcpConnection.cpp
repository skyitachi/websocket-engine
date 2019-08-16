//
// Created by skyitachi on 2019-05-27.
//

#include <ws/TcpConnection.h>

namespace ws {
  static const int kMinWritableSize = 512;
  
  // NOTE: alloc 这里不能使用内置的buffer
  static void on_uv_alloc(uv_handle_t* handle, size_t suggest_size, uv_buf_t* buf) {
    auto conn = (TcpConnection* ) handle->data;
    assert(conn);
    buf->len = conn->buf.ensureSpace(kMinWritableSize);
    buf->base = conn->buf.writeStart();
  }
  
  // NOTE: buf是临时变量，必须先拷贝然后再使用
  static void on_uv_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    auto conn = (TcpConnection* ) stream->data;
    assert(conn);
    if (nread < 0) {
      BOOST_LOG_TRIVIAL(info) << "close connection " << conn->id() << " passively";
      conn->handleClose();
    } else if (nread == 0) {
      conn->close();
    } else {
      BOOST_LOG_TRIVIAL(debug) << "conn id " << conn->id() << " received " << nread << " bytes data";
      conn->buf.updateWriteIndex(nread);
      conn->handleMessage(nread);
    }
  }
  
  int TcpConnection::readStart() {
    return uv_read_start(stream(), on_uv_alloc, on_uv_read);
  }
  
  int TcpConnection::readStop() {
    BOOST_LOG_TRIVIAL(debug) << "conn " << id() << " stop read";
    BOOST_LOG_TRIVIAL(debug) << "connection bufs size: " << buf.readableBytes();
    return uv_read_stop(stream());
  }
  
  int TcpConnection::send(const char *sendBuf, size_t len) {
    // write_req 这类的对象不需要传递data，使用handle->data即可
    if (state_ == kWritting) {
      outputBuf.write(sendBuf, len);
      return 0;
    }
    assert(state_ == kConnected);
    lastWrite_ = len;
    
    uv_buf_t uv_buf = uv_buf_init(const_cast<char *>(sendBuf), len);
    setState(kWritting);
    
    // NOTE: 如果使用uv_try_write就需要自己管理output buffer了, try_write不会queue write request
    return uv_write(new uv_write_t, stream(), &uv_buf, 1, [](uv_write_t* req, int status) {
      // 留给unique_ptr自动管理
      std::unique_ptr<uv_write_t> reqHolder(req);
      auto conn = (TcpConnection *)req->handle->data;
      assert(conn);
      if (status < 0) {
        BOOST_LOG_TRIVIAL(error) << "connection write error " << uv_strerror(status);
        conn->handleClose();
        return;
      }
      conn->handleWrite(conn->lastWrite());
      conn->resetLastWrite();
      if (conn->isLastWrite) {
        conn->setState(kConnected);
        conn->handleClose();
        conn->isLastWrite = false;
        return;
      }
      if (conn->state_ == kDisconnecting) {
        // 仍然要将outputBuf中的数据发送完
        conn->setState(kConnected);
        if (conn->outputBuf.readableBytes() > 0) {
          conn->isLastWrite = true;
        } else {
          conn->handleClose();
          return;
        }
      }
      conn->setState(kConnected);
      if (conn->outputBuf.readableBytes() > 0) {
        conn->send(conn->outputBuf.peek(), conn->outputBuf.readableBytes());
        conn->outputBuf.retrieve(conn->outputBuf.readableBytes());
      }
    });
  }
  
  // stop write
  int TcpConnection::shutdown() {
    return uv_shutdown(new uv_shutdown_t, stream(), [](uv_shutdown_t* req, int status) {
      std::unique_ptr<uv_shutdown_t> reqHolder(req);
      auto conn = (TcpConnection *)req->handle->data;
      assert(conn);
      if (status < 0) {
        // TODO: 错误处理
        return;
      }
      BOOST_LOG_TRIVIAL(info) << "connection " << conn->id() << " shutdown write";
    });
  }
  
  // when read 0 size then close
  void TcpConnection::close() {
    handleClose();
  }
  
  void TcpConnection::handleClose() {
    if (state_ == kWritting) {
      BOOST_LOG_TRIVIAL(debug) << "conn id " << id() << " disconnect while writting";
      setState(kDisconnecting);
      return;
    }
    assert(state_ == kConnected || state_ == kConnecting);
    setState(kDisconnected);
    readStop();
    connectionCallback_(shared_from_this());
    closeCallback_(shared_from_this());
  }
  
  void TcpConnection::attachToLoop(uv_loop_t *newLoop) {
    if (newLoop != loop_) {
      uv_os_fd_t fd;
      uv_fileno((uv_handle_t*)tcp_.get(), &fd);
      BOOST_LOG_TRIVIAL(debug) << "get system fd: " << fd;
      tcp_.reset(new uv_tcp_t);
      uv_tcp_init(newLoop, tcp_.get());
      assert(tcp_->loop == newLoop);
      uv_tcp_open(tcp_.get(), fd);
      tcp_->data = this;
      loop_ = newLoop;
    }
  }
}
