#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define COLOR_RED    "\e[31m"
#define COLOR_GREEN  "\e[32m"
#define COLOR_R      "\e[0m"

#define GET_RES(func) {\
  int res = func();\
  if (res > 0) {\
    /*printf(COLOR_RED "Case " #func " failed" COLOR_R "\n");*/\
    failed_cases += 1;\
  }\
  failed_assertions += res;\
}

enum TestCases {
#define CASE(__case__) TestCase_##__case__ ,
#include "cases_list.h"
#undef CASE
  TestCase_MAXCASES,
};

#define CASE(__case__) \
extern unsigned __case__();
#include "cases_list.h"
#undef CASE

unsigned ((*test_cases[TestCase_MAXCASES])(void)) = {
#define CASE(__case__) [TestCase_##__case__] = &(__case__),
#include "cases_list.h"
#undef CASE
};

int total_assertions = 0;

int main () {
  int failed_assertions = 0;
  int failed_cases = 0;

  for (int i = 0; i < TestCase_MAXCASES; i++) {
    GET_RES(test_cases[i]);
  }

  if (!failed_assertions) {
    printf(COLOR_GREEN "All tests passed!\n" COLOR_R);
    exit(EXIT_SUCCESS);
  } else {
    puts("\n=================================== TEST SUMMARY ================================\n");
    printf(COLOR_GREEN "Pass: %d \t" COLOR_RED "Failed: %d\n" COLOR_R, total_assertions - failed_assertions, failed_assertions);
    exit(EXIT_FAILURE);
  }

  return 0;
}
