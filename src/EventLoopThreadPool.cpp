//
// Created by skyitachi on 2019-06-20.
//

#include <ws/EventLoopThreadPool.h>

namespace ws {
  void EventLoopThreadPool::setThreadNums(int threadNums) {
    for(int i = 0; i < threadNums; i++) {
      threadList_.push_back(std::make_unique<EventLoopThread>());
      loopList_.push_back(threadList_[i]->startLoop());
    }
  }
  
  uv_loop_t* EventLoopThreadPool::getNextLoop() {
    if (threadList_.empty()) {
      return baseLoop_;
    }
    uv_loop_t *loop = loopList_[next_];
    next_++;
    if (next_ == loopList_.size()) {
      next_ = 0;
    }
    return loop;
  }
}
