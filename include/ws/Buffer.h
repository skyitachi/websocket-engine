//
// Created by skyitachi on 2019-07-29.
//

#ifndef WEBSOCKET_ENGINE_BUFFER_H
#define WEBSOCKET_ENGINE_BUFFER_H

#include <vector>
#include <boost/log/trivial.hpp>
#include <arpa/inet.h>
#include "String.h"

namespace ws {
  
  typedef uint8_t byte;
  
  class Buffer {
  public:
    static const int kInitialSize = 1024;
    static const int kExpandThreshold = 3;
    static const size_t kExpandSize = 8192;
    
    Buffer() {
      buf_.resize(kInitialSize);
    }
    
    Buffer(size_t len) {
      buf_.resize(len);
    }
    
    size_t remaining() const {
      return buf_.size() - writeIndex_;
    }

    size_t available() const {
      return buf_.size() - writeIndex_ + readIndex_;
    }
    
    size_t size() const {
      return buf_.size();
    }
    
    size_t readableBytes() const {
      return size_t(writeIndex_ - readIndex_);
    }
    
    const char *begin() const {
      return &*buf_.begin();
    }
    
    // NOTE: 要复制吗
    void read(char *dst, size_t len) {
      assert(readableBytes() >= len);
      memcpy(dst, peek(), len);
      retrieve(len);
    }
    
    // 单纯更新readIndex_
    void read(size_t len) {
      assert(readIndex_ + len <= writeIndex_);
      readIndex_ += len;
    }
    
    void unread(size_t len) {
      assert(readIndex_ - len >= 0 && readIndex_ - len <= writeIndex_);
      readIndex_ -= len;
    }
    
    String readString() {
      auto ret = String(peek(), readableBytes());
      retrieve(readableBytes());
      return ret;
    }
    
    const char *peek() const {
      return begin() + readIndex_;
    }
    
    char *writeStart(){
      return const_cast<char *>(begin()) + writeIndex_;
    }
    
    size_t ensureSpace(size_t len) {
      if (remaining() >= len) {
        return remaining();
      }
      // 此时需要移动一下内存，保证一下空间
      if (available() >= len) {
        std::copy(buf_.begin() + readIndex_, buf_.begin() + writeIndex_, buf_.begin());
        writeIndex_ -= readIndex_;
        readIndex_ = 0;
        return len;
      }
      ssize_t left = len - remaining();
      if (left > 0) {
        lastExpand_++;
      }
      if (lastExpand_ > kExpandThreshold) {
        buf_.resize(buf_.size() + left + kExpandSize);
        lastExpand_ = 0;
        return remaining();
      }
      buf_.resize(buf_.size() + left);
      return len;
    }
    
    // NOTE: 自动扩容
    void write(const char *buf, size_t len) {
      ensureSpace(len);
      std::copy(buf, buf + len, writeStart());
      writeIndex_ += len;
    }
    
    void write(Buffer& src) {
      auto len = src.readableBytes();
      write(src, len);
    }
    
    void write(Buffer& src, size_t len) {
      ensureSpace(len);
      std::copy(src.peek(), src.peek() + len, writeStart());
      writeIndex_ += len;
      src.retrieve(len);
    }
    
    void unwrite(size_t len) {
      if (readableBytes() < len) {
        updateWriteIndex(-readableBytes());
      } else {
        updateWriteIndex(-len);
      }
    }
    
    void writeString(const std::string& input) {
      write(input.c_str(), input.size());
    }
    
    void writeString(const String& input) {
      write(input.c_str(), input.size());
    }
    
    void retrieve(ssize_t len) {
      assert(readIndex_ + len <= writeIndex_);
      readIndex_ += len;
    }
    
    // ssize_t 不能和size_t混合计算
    void updateWriteIndex(ssize_t delta) {
      if (delta < 0) {
        writeIndex_ -= (size_t )(-delta);
        if (writeIndex_ < readIndex_) {
          writeIndex_ = readIndex_;
        }
        return;
      }
      assert(delta <= available() && (writeIndex_ + delta) >= readIndex_);
      if (remaining() >= delta) {
        writeIndex_ += delta;
        return;
      }
      // 需要移动一下内存
      std::copy(buf_.begin() + readIndex_, buf_.begin() + writeIndex_, buf_.begin());
      writeIndex_ -= readIndex_;
      readIndex_ = 0;
      writeIndex_ += delta;
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
    
    bool empty() const {
      return readableBytes() == 0;
    }
    
    template <typename T>
    T readTypedNumber() {
      size_t size = sizeof(T);
      assert(readableBytes() >= size);
      T ret;
      switch (size) {
        case 1:
          ret = *(T*)peek();
          break;
        case 2:
          ret = ntohs(*(T*)peek());
          break;
        case 4:
          ret = ntohl(*(T*)(peek()));
          break;
        case 8:
          T high = (T) ntohl(*(uint32_t* )(peek()));
          T low = (T) ntohl(*(uint32_t* )(peek() + 4));
          ret = (uint64_t)high << 32;
          ret |= low;
          break;
      }
      retrieve(size);
      return ret;
    }
  
  private:
    std::vector<char> buf_;
    int readIndex_ = 0;
    int writeIndex_ = 0;
    // NOTE: 如果连续多次扩容的话就大胆加上一些buffer
    int lastExpand_ = 0;
  };
}

#endif //WEBSOCKET_ENGINE_BUFFER_H
