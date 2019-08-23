//
// Created by skyitachi on 2019-08-20.
//

#ifndef WEBSOCKET_ENGINE_STRING_H
#define WEBSOCKET_ENGINE_STRING_H
#include <cstring>
#include <cstdio>
#include <iostream>

namespace ws {
  class String {
  public:
    String(): data_(new char[1]), size_(0) {
      *data_ = '\0';
    }
    // 允许const char* 到 String的转化就不需要使用explicit了
    String(const char* src): data_(new char[strlen(src) + 1]), size_(strlen(src)) {
      ::strcpy(data_, src);
      data_[size_] = '\0';
    }
    
    String(const char* src, size_t size): data_(new char[size + 1]), size_(size) {
      memcpy(data_, src, size_);
      data_[size] = 0;
    }
    
    String(const String& lhs): data_(new char[lhs.size() + 1]), size_(lhs.size()) {
      ::strcpy(data_, lhs.data());
      data_[size_] = 0;
    }
  
    String &operator=(const String& lhs) {
      if (data_ != nullptr) {
        delete[] data_;
      }
      size_ = lhs.size();
      data_ = new char[size_ + 1];
      strcpy(data_, lhs.data());
      data_[size_] = 0;
      return *this;
    }
    // move and noexcept
    String(String &&rhs) noexcept: data_(rhs.data_), size_(rhs.size_) {
      rhs.data_ = nullptr;
      rhs.size_ = 0;
    }
    
    String& operator=(String &&rhs) noexcept {
      swap(rhs);
      rhs.data_ = nullptr;
      return *this;
    }
    
    const char* data() const {
      return data_;
    }
    
    const char* c_str() const {
      return data_;
    }
    
    bool empty() const {
      return data_ == nullptr || size_ == 0;
    }
    
    size_t size() const {
      return size_;
    }
    
    void swap(String& rhs) {
      std::swap(data_, rhs.data_);
      std::swap(size_, rhs.size_);
    }
    
    
    ~String() {
      delete[] data_;
    }
    
    
  private:
    char *data_;
    size_t size_;
  };
  
  static std::ostream& operator<< (std::ostream& out, const String& s) {
    if (s.empty()) {
      out << "";
    } else {
      out << s.c_str();
    }
    return out;
  }
  
  // TODO:
//  static String& operator+(const String& lhs, const String&rhs) {
//    String s1;
//    return s1;
//  }
  
}
#endif //WEBSOCKET_ENGINE_STRING_H
