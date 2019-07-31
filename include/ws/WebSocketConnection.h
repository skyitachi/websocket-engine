//
// Created by skyitachi on 2019-07-31.
//

#ifndef WEBSOCKET_ENGINE_WEBSOCKETCONNECTION_H
#define WEBSOCKET_ENGINE_WEBSOCKETCONNECTION_H

#include <ws/util.h>
#include <ws/TcpConnection.h>
#include <unordered_map>
#include <http_parser/http_parser.h>
#include <memory>

namespace ws {
  class WebSocketConnection: util::NoCopyable {
  public:
    enum Status {
      INITIAL,
      HANDSHAKE,
      CONNECT,
      CLOSE
    };
    
    WebSocketConnection(const TcpConnectionPtr& ptr): conn_(ptr), status_(INITIAL) {
      httpParserPtr = std::make_unique<http_parser>();
      http_parser_init(httpParserPtr.get(), HTTP_REQUEST);
      initHttpParser();
      
    }
    
    size_t parse(const char *data, size_t len) {
      return http_parser_execute(httpParserPtr.get(), &httpParserSettings_, data, len);
    }
    
    void initHttpParser();
    
    void setHeaderValue(std::string&& value) {
      assert(!lastHeaderField_.empty());
      headers_.insert(std::make_pair(lastHeaderField_, std::move(value)));
      lastHeaderField_ = "";
    }
  
    void setLastHeaderField(std::string&& field) {
      lastHeaderField_ = std::move(field);
    }
    
    void onHeaderComplete();

  private:
    const std::string computeAcceptKey(const std::string&);
    const TcpConnectionPtr& conn_;
    std::unique_ptr<http_parser> httpParserPtr;
    http_parser_settings httpParserSettings_;
    Status status_;
    std::unordered_map<std::string, std::string> headers_;
    std::string lastHeaderField_;
    
  };
  typedef std::shared_ptr<WebSocketConnection> WebSocketConnectionPtr;
  
}
#endif //WEBSOCKET_ENGINE_WEBSOCKETCONNECTION_H
