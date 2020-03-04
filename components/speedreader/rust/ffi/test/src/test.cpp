#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deps/picotest/picotest.h"
#include "tests.h"

extern "C" {

int run_tests() {
  subtest("url readability", test_url_check);
  subtest("rewriter type", test_find_type);
  subtest("opaque config works", test_find_type);
  subtest("deserialization works", test_rewriter_deserialized);
  subtest("rewriter works", test_rewriter);

  subtest("c++ url readability", test_cpp_url_check);
  subtest("c++ rewriter works", test_cpp_rewriter);
  return done_testing();
}

}
