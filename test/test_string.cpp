//
// Created by skyitachi on 2019-08-20.
//
#include <ws/String.h>
#include <vector>
//#include <boost/log/trivial.hpp>

using namespace ws;

void foo(String x)
{
//  BOOST_LOG_TRIVIAL(info) << "in the foo";
}

void foobar(const String& s) {
//  BOOST_LOG_TRIVIAL(info) << "in the foobar";
}

void bar(const String& x)
{
//  BOOST_LOG_TRIVIAL(info) << "before allocated";
  String local = x;
}

String baz()
{
  String ret("world");
  return ret;
}

void f2(String &&s2) {}

void f1(String &&s1) {
  f2(std::move(s1));
}


int main()
{
//  foo(String("hello"));
//  bar(s0);
  // 左值会触发拷贝构造函数
  String s0;
//  foo(std::move(s0));
//  f1(s0);
//  f1(String("hello"));
  f1("hello");
//  foo(s0);
  // 不需要用到形参的时候, 根本不会触发copy constructor
//  String s0;
//  foobar(s0);
}

