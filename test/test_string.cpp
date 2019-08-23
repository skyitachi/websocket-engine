//
// Created by skyitachi on 2019-08-20.
//
#include <ws/String.h>
#include <vector>
#include <iostream>
//#include <boost/log/trivial.hpp>

using namespace ws;

void foo(String x)
{
  
  std::cout << x << std::endl;
}

void foobar(const String& s) {
  std::cout << s << std::endl;
}

void f2(String &&s2) {
  std::cout << "f2: " << s2 << std::endl;
}

void bar(const String& x)
{
  String local = x;
}

String baz()
{
  String ret("world");
  return ret;
}

void f1(String &&s1) {
  String s = s1;
  std::cout << "f1: " << s << std::endl;
}

void fmove(String &&s1) {
  String s = std::move(s1);
  std::cout << "fmove: " << s << std::endl;
}

//template <typename T> T ft(T a) {}
//template <typename T, typename U, typename V> void ft(T,U,V);

void test_string_move(std::string&& s) {
  std::cout <<"test_string_move: "  << s << std::endl;
}

int main()
{
//  foo(String("hello"));
//  bar(s0);
  String s0("hello");
//  foobar(s0);
  f1(std::move(s0));
  std::cout << "after fake move: " << s0 << std::endl;
  
  String m("move");
  fmove(std::move(m));
  std::cout << "after really move: " << m << std::endl;
  
//  std::string s2("hello");
//  test_string_move(std::move(s2));
//  std::cout << "after move: " << s2 << std::endl;
//  foo(sd::move(s0));
//  f1(s0);
//  f1(String("hello"));
//  f1("hello");
//  std::cout << s0 << std::endl;
//  foo(s0);
  // 不需要用到形参的时候, 根本不会触发copy constructor
//  String s0;
//  foobar(s0);
}

