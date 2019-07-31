//
// Created by skyitachi on 2019-06-20.
//

#ifndef SOCKSPROXY_EVENTLOOPTHREADPOOL_H
#define SOCKSPROXY_EVENTLOOPTHREADPOOL_H

#include "util.h"
#include "EventLoopThread.h"
#include <uv.h>
#include <vector>
#include <memory>
#include <boost/log/trivial.hpp>

namespace ws {
  class EventLoopThreadPool: ws::util::NoCopyable {
  public:
    typedef std::function<void (uv_loop_t*)> ThreadLoopThreadCallback;
    EventLoopThreadPool(uv_loop_t* loop): baseLoop_(loop) {}
    void setThreadNums(int threadNums);
    
    bool isSingleThread() {
      return threadList_.size() == 0;
    }
    
    uv_loop_t* getNextLoop();
    
    ~EventLoopThreadPool() {
      BOOST_LOG_TRIVIAL(trace) << "EventLoopThreadPool destructor called";
    }
  private:
    std::vector<std::unique_ptr<EventLoopThread>> threadList_;
    std::vector<uv_loop_t*> loopList_;
    int next_ = 0;
    uv_loop_t* baseLoop_;
  };
  
}


#endif //SOCKSPROXY_EVENTLOOPTHREADPOOL_H
