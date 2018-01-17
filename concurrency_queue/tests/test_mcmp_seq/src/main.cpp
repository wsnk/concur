#define BOOST_TEST_MODULE MCMPTest
#include <chrono>
#include <boost/test/unit_test.hpp>
#include <future>
#include <iostream>
#include "mcmp_seq.h"
#include <vector>
//------------------------------------------------------------------------------
int64_t const TOTAL_HIT = 1000000;
int64_t const WRITERS_COUNT = std::thread::hardware_concurrency();
int64_t const READERS_COUNT = std::thread::hardware_concurrency();
MCMPSeq< int64_t > queue(TOTAL_HIT);
std::vector< int64_t > misses(READERS_COUNT);
//------------------------------------------------------------------------------
void writer() {
  for (int64_t i = 0; i < TOTAL_HIT / WRITERS_COUNT; ++i) {
    queue.produce(i);
  }
}
//------------------------------------------------------------------------------
void reader(int64_t const aIndex) {   
  int64_t hit = 0;
  int64_t &miss = misses[aIndex];
  miss = -1;
  while (hit != TOTAL_HIT / READERS_COUNT) {
    ++miss;
    int64_t val = 0;
    while (hit != TOTAL_HIT / READERS_COUNT && queue.consume(val) ) {
      ++hit;
    }   
  }
}
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(mcmp_concurrency_push_pop) {
  int64_t const testValues[] = {1024, -45345, 0, 234, 34, -32768};
  
  MCMPSeq< int64_t > queue(6);

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
BOOST_AUTO_TEST_CASE(scsp_concurrency) {
  auto const beg = std::chrono::high_resolution_clock::now();

  std::vector< std::future< void > > futuresRead;
  for (int64_t i = 0; i < READERS_COUNT; ++i) {
    futuresRead.push_back( std::async(reader, i) );
  }

  std::vector< std::future< void > > futuresWrite;
  for (int64_t i = 0; i < WRITERS_COUNT; ++i) {
    futuresWrite.push_back( std::async(writer) );
  }
 
  for (auto const &futureWrite : futuresWrite) {
    futureWrite.wait();
  }

  for (auto const &futureRead : futuresRead) {
    futureRead.wait();
  }

  auto const end = std::chrono::high_resolution_clock::now();
  auto const duration = end - beg;

  int64_t totalMiss = 0;
  for (int64_t i = 0; i < READERS_COUNT; ++i) {
    totalMiss += misses[i];
  }

  std::cout << "hit " << TOTAL_HIT << ", miss " << totalMiss << ", duration "
    << std::chrono::duration_cast< std::chrono::microseconds >(duration).count()
    << " microseconds" << std::endl;
}
//------------------------------------------------------------------------------
