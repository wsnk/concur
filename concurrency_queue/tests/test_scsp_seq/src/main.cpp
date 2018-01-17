#define BOOST_TEST_MODULE SCSPTest
#include <chrono>
#include <boost/test/unit_test.hpp>
#include <future>
#include <iostream>
#include "scsp_seq.h"
//------------------------------------------------------------------------------
int64_t const TOTAL_HIT = 1000000;
int64_t miss = -1;
SCSPSeq< int64_t > queue(TOTAL_HIT);
//------------------------------------------------------------------------------
void writer() {
  for (int64_t i = 0; i < TOTAL_HIT; ++i) {
    queue.produce(i);
  }
}
//------------------------------------------------------------------------------
void reader() {
  int64_t hit = 0;
  while (hit != TOTAL_HIT) {
    ++miss;
    int64_t val = 0;
    while ( queue.consume(val) ) {
      ++hit;
    }   
  }
}
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(scsp_concurrency_push_pop) {
  int64_t const testValues[] = {1024, -45345, 0, 234, 34, -32768};
  
  SCSPSeq< int64_t > queue(6);

  for (auto &testValue : testValues) {
    queue.produce(testValue);
  }
  
  for (size_t i = 0, n = sizeof(testValues) / sizeof(testValues[0]); i < n;
    ++i)
  {
    int64_t value = 0;
    BOOST_CHECK_EQUAL( true, queue.consume(value) );
    BOOST_CHECK_EQUAL(testValues[i], value);
  }  
}
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(scsp_concurrency_perfomance) {
  auto const beg = std::chrono::high_resolution_clock::now();

  auto futureRead = std::async(reader);
  auto futureWrite = std::async(writer);
  futureWrite.wait();
  futureRead.wait();

  auto const end = std::chrono::high_resolution_clock::now();
  auto const duration = end - beg;

  std::cout << "hit " << TOTAL_HIT << ", miss " << miss << ", duration "
    << std::chrono::duration_cast< std::chrono::microseconds >(duration).count()
    << " microseconds" << std::endl;
}
//------------------------------------------------------------------------------
