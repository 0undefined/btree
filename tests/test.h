#ifndef TEST_H
#define TEST_H

#include <stdio.h>

#define TEST_CASE(__case_name__, __case__) int __case_name__(){ \
  const char* testname = #__case_name__;\
  int fail_counter = 0;\
  __case__\
  return fail_counter;\
}

#define CHECK(__assertion__) {\
  total_assertions += 1; \
  if (!(__assertion__)) {\
  printf("Test failed: %s in %s:%d\n> " #__assertion__ "\n", testname, __FILE__, __LINE__);\
  fail_counter += 1;\
}}

extern int total_assertions;

#endif
