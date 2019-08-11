//
// Created by skyitachi on 2019-07-29.
//

#ifndef WEBSOCKET_ENGINE_BUFFER_H
#define WEBSOCKET_ENGINE_BUFFER_H

#include <vector>
#include <boost/log/trivial.hpp>
#include <arpa/inet.h>
namespace ws {
  
  typedef uint8_t byte;
  
  class Buffer {
  public:
    static const int kInitialSize = 1024;
    
    Buffer() {
      buf_.resize(kInitialSize);
    }
    
    Buffer(size_t len) {
      buf_.resize(len);
    }
    
    size_t remaining() {
      return buf_.size() - writeIndex_;
    }
    
    size_t size() {
      return buf_.size();
    }
    
    size_t readableBytes() {
      return size_t(writeIndex_ - readIndex_);
    }
    
    char *begin() {
      return &*buf_.begin();
    }
    
    // NOTE: 要复制吗
    void read(char *dst, size_t len) {
      assert(readableBytes() >= len);
      memcpy(dst, peek(), len);
      retrieve(len);
    }
    
    const char *peek() {
      return begin() + readIndex_;
    }
    
    char *writeStart() {
      return begin() + writeIndex_;
    }
    
    size_t ensureSpace(ssize_t len) {
      if (remaining() >= len) {
        return remaining();
      }
      // 此时需要移动一下内存，保证一下空间
      if (readIndex_ + remaining() >= len) {
        std::copy(buf_.begin() + readIndex_, buf_.begin() + writeIndex_, buf_.begin());
        writeIndex_ -= readIndex_;
        readIndex_ = 0;
        return len;
      }
      ssize_t left = len - remaining();
      buf_.resize(buf_.size() + left);
      return len;
    }
    
    // NOTE: 自动扩容
    void write(const char *buf, ssize_t len) {
      ensureSpace(len);
      std::copy(buf, buf + len, begin() + writeIndex_);
      writeIndex_ += len;
    }
    
    void retrieve(ssize_t len) {
      readIndex_ += len;
    }
    
    void updateWriteIndex(int newWriteIndex) {
      assert(newWriteIndex >= 0 && newWriteIndex < buf_.size());
      writeIndex_ = newWriteIndex;
    }
    
    int getWriteIndex() {
      return writeIndex_;
    }
    
    void clear() {
      writeIndex_ = readIndex_ = 0;
    }
    
    void shrinkToFit() {
      shrink(readableBytes());
    }
    
    // 如果
    void shrink(size_t len) {
      if (readIndex_ > 0) {
        std::copy(buf_.begin() + readIndex_, buf_.begin() + writeIndex_, buf_.begin());
        writeIndex_ -= readIndex_;
        readIndex_ = 0;
      }
      // 阶段数据
      if (readableBytes() > len) {
        writeIndex_ = len;
      }
      buf_.resize(len);
    }
    
    void putByte(uint8_t byte) {
      write((const char *) &byte, 1);
    }
    
    // 网络序
    void putUInt16(uint16_t word) {
      uint16_t network = htons(word);
      write((const char *) &network, 2);
    }
    
    void putUInt32(uint32_t integer) {
      uint32_t network = htonl(integer);
      write((const char *) &network, 4);
    }
    
    void putUInt64(uint64_t bigInt) {
      auto high = (uint32_t) (bigInt >> 32);
      auto low = (uint32_t) (bigInt & 0xfffffffff);
      putUInt32(high);
      putUInt32(low);
    }
    
    byte readByte() {
      byte b = (byte)*(peek());
      readIndex_ += 1;
      return b;
    }
    
    uint16_t readUInt16() {
      uint16_t ret = ntohs((uint16_t)*(peek()));
      readIndex_ += 2;
      return ret;
    }
    
    template <typename T>
    T readTypedNumber() {
      size_t size = sizeof(T);
      T ret;
      if (size == 1) {
        ret = (T)*peek();
      } else if (size == 2) {
        ret = ntohs((T)*peek());
      } else if (size == 4) {
        ret = ntohl((T)*(peek()));
      }
      retrieve(size);
      return ret;
    }
  
  private:
    std::vector<char> buf_;
    int readIndex_ = 0;
    int writeIndex_ = 0;
  };
}

#endif //WEBSOCKET_ENGINE_BUFFER_H
