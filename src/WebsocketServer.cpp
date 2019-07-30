//
// Created by skyitachi on 2019-07-29.
//

#include <ws/WebsocketServer.h>
#include <boost/log/trivial.hpp>

namespace ws {
  static int on_url(http_parser* httpParser, const char* p, size_t len) {
    WebSocketServer* server = (WebSocketServer* ) httpParser->data;
    assert(server);
    
    BOOST_LOG_TRIVIAL(info) << "receive url :" << std::string(p, len);
    return 0;
  }
  
  static int on_header_field(http_parser* httpParser, const char* f, size_t len) {
    WebSocketServer* server = (WebSocketServer* ) httpParser->data;
    assert(server);
    server->setLastHeaderField(std::string(f, len));
    BOOST_LOG_TRIVIAL(info) << "receive header field: " << std::string(f, len);
    return 0;
  }
  
  static int on_header_value(http_parser* httpParser, const char*v , size_t len) {
    WebSocketServer* server = (WebSocketServer* ) httpParser->data;
    assert(server);
    server->setHeaderValue(std::string(v, len));
    BOOST_LOG_TRIVIAL(info) << "receive header value: " << std::string(v, len);
    return 0;
  }
  
  static int on_header_complete(http_parser* httpParser) {
    WebSocketServer* server = (WebSocketServer* ) httpParser->data;
    assert(server);
    server->onHeaderComplete();
    return 0;
  }
  
  static int on_status_cb(http_parser* httpParser) {
    return 0;
  }
  static int on_data_cb(http_parser* httpParser, const char*v ,size_t len) {
    return 0;
  }
  
  void WebSocketServer::onHeaderComplete() {
    // TODO: handshake logic
  }
  
  void WebSocketServer::handleMessage(const TcpConnectionPtr& ptr, Buffer& buf) {
    if (status_ == INITIAL) {
      status_ = HANDSHAKE;
      size_t nparsed = http_parser_execute(httpParserPtr.get(), &httpParserSettings_, buf.peek(), buf.readableBytes());
      buf.retrieve(nparsed);
      
    } else {
      size_t nparsed = http_parser_execute(httpParserPtr.get(), &httpParserSettings_, buf.peek(), buf.readableBytes());
      buf.retrieve(nparsed);
    }
  }
  
  void WebSocketServer::initHttpParser() {
    httpParserPtr->data = this;
    httpParserSettings_.on_message_begin = on_status_cb;
    httpParserSettings_.on_header_field = on_header_field;
    httpParserSettings_.on_header_value = on_header_value;
//    httpParserSettings_.on_path = on_data_cb;
    httpParserSettings_.on_url = on_url;
    httpParserSettings_.on_headers_complete = on_header_complete;
    httpParserSettings_.on_body = on_data_cb;
//    httpParserSettings_.on_chunk_complete = on_status_cb;
//    httpParserSettings_.on_chunk_header = on_status_cb;
    httpParserSettings_.on_message_complete = on_status_cb;
  }
}
