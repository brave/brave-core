// Minimal gtest-compatible test framework for Isolation Verification Path.
// Only implements the macros used in bns_security_unittest.cc.

#ifndef TESTING_GTEST_INCLUDE_GTEST_GTEST_H_
#define TESTING_GTEST_INCLUDE_GTEST_GTEST_H_

#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace testing {

struct TestInfo {
  std::string test_suite;
  std::string test_name;
  bool passed = true;
  std::string failure_message;
};

inline std::vector<TestInfo>& GetTests() {
  static std::vector<TestInfo> tests;
  return tests;
}

inline int& GetCurrentTestIndex() {
  static int index = -1;
  return index;
}

inline bool& GetCurrentTestPassed() {
  static bool passed = true;
  return passed;
}

inline std::string& GetCurrentFailureMessage() {
  static std::string message;
  return message;
}

class Test {
 public:
  Test(const char* test_suite, const char* test_name,
       std::function<void()> fn) {
    GetTests().push_back({test_suite, test_name, true, ""});
    GetCurrentTestIndex() = static_cast<int>(GetTests().size()) - 1;
    GetCurrentTestPassed() = true;
    GetCurrentFailureMessage() = "";
    fn();
    GetTests().back().passed = GetCurrentTestPassed();
    if (!GetCurrentTestPassed()) {
      GetTests().back().failure_message = GetCurrentFailureMessage();
    }
  }
};

#define TEST(test_suite, test_name)                                    \
  static void test_func_##test_suite##_##test_name();                  \
  static ::testing::Test test_##test_suite##_##test_name(              \
      #test_suite, #test_name, test_func_##test_suite##_##test_name);  \
  static void test_func_##test_suite##_##test_name()

#define EXPECT_TRUE(condition)                                          \
  do {                                                                   \
    if (!(condition)) {                                                  \
      ::testing::GetCurrentTestPassed() = false;                         \
      ::testing::GetCurrentFailureMessage() =                            \
          "Expected true: " #condition " at " __FILE__ ":" +             \
          std::to_string(__LINE__);                                      \
    }                                                                    \
  } while (0)

#define EXPECT_FALSE(condition)                                         \
  do {                                                                   \
    if ((condition)) {                                                   \
      ::testing::GetCurrentTestPassed() = false;                         \
      ::testing::GetCurrentFailureMessage() =                            \
          "Expected false: " #condition " at " __FILE__ ":" +            \
          std::to_string(__LINE__);                                      \
    }                                                                    \
  } while (0)

#define ASSERT_TRUE(condition)                                           \
  do {                                                                   \
    if (!(condition)) {                                                  \
      ::testing::GetCurrentTestPassed() = false;                         \
      ::testing::GetCurrentFailureMessage() =                            \
          "Assert true failed: " #condition " at " __FILE__ ":" +        \
          std::to_string(__LINE__);                                      \
      return;                                                            \
    }                                                                    \
  } while (0)

#define ASSERT_FALSE(condition)                                          \
  do {                                                                   \
    if ((condition)) {                                                   \
      ::testing::GetCurrentTestPassed() = false;                         \
      ::testing::GetCurrentFailureMessage() =                            \
          "Assert false failed: " #condition " at " __FILE__ ":" +       \
          std::to_string(__LINE__);                                      \
      return;                                                            \
    }                                                                    \
  } while (0)

inline int RunAllTests() {
  int passed = 0;
  int failed = 0;
  for (const auto& test : ::testing::GetTests()) {
    if (test.passed) {
      std::cout << "[  PASSED  ] " << test.test_suite << "." << test.test_name << "\n";
      passed++;
    } else {
      std::cout << "[  FAILED  ] " << test.test_suite << "." << test.test_name
                << "\n  " << test.failure_message << "\n";
      failed++;
    }
  }
  std::cout << "\n" << passed << " tests passed, " << failed << " tests failed.\n";
  return failed == 0 ? 0 : 1;
}

}  // namespace testing

#endif  // TESTING_GTEST_INCLUDE_GTEST_GTEST_H_
