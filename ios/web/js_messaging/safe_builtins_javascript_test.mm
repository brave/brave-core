// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>
#include <utility>

#include "ios/web/public/test/javascript_test.h"
#include "ios/web/public/test/js_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"

namespace {
constexpr char kContainer[] = "window.__gSafeBuiltins";
constexpr char kObject[] = "Object";
constexpr char kArray[] = "Array";
constexpr char kFunction[] = "Function";
constexpr char kSafeObject[] = "window.__gSafeBuiltins.$Object";
constexpr char kSafeArray[] = "window.__gSafeBuiltins.$Array";
constexpr char kSafeFunction[] = "window.__gSafeBuiltins.$Function";
}  // namespace

class SafeBuiltinsContainerJavaScriptTest : public web::JavascriptTest {
 protected:
  void SetUp() override {
    JavascriptTest::SetUp();
    AddUserScript(@"safe_builtins");

    ASSERT_TRUE(LoadHtml(@"<p>"));
  }
};

// Tests that the safe builtins container (__gSafeBuiltins) is frozen and cannot
// be altered
TEST_F(SafeBuiltinsContainerJavaScriptTest, TestContainerIsFrozen) {
  // The container has no prototype and should remain that way
  id result = web::test::ExecuteJavaScript(
      web_view(), [NSString stringWithFormat:@"%s.prototype", kContainer]);
  EXPECT_NSEQ(nil, result);

  web::test::ExecuteJavaScriptInWebView(
      web_view(),
      [NSString stringWithFormat:@"%s.prototype = {};", kContainer]);

  id protectedResult = web::test::ExecuteJavaScript(
      web_view(), [NSString stringWithFormat:@"%s.prototype", kContainer]);
  EXPECT_NSEQ(nil, protectedResult);
}

class SafeBuiltinsJavaScriptTest
    : public web::JavascriptTest,
      public testing::WithParamInterface<std::pair<std::string, std::string>> {
 protected:
  void SetUp() override {
    JavascriptTest::SetUp();
    AddUserScript(@"safe_builtins");

    ASSERT_TRUE(LoadHtml(@"<p>"));
  }
};

INSTANTIATE_TEST_SUITE_P(AllSafeBuiltins,
                         SafeBuiltinsJavaScriptTest,
                         testing::Values(std::make_pair(kObject, kSafeObject),
                                         std::make_pair(kArray, kSafeArray),
                                         std::make_pair(kFunction,
                                                        kSafeFunction)));

// Tests that a value added to the prototype of a builtin is not visible in
// the equivalent safe builtin prototype
TEST_P(SafeBuiltinsJavaScriptTest, TestBuiltinPrototypeAssignment) {
  std::string builtin = GetParam().first;
  std::string safe_builtin = GetParam().second;

  id result = web::test::ExecuteJavaScript(
      web_view(), [NSString stringWithFormat:@"%s.prototype.testValue = 1;",
                                             builtin.c_str()]);
  EXPECT_NSEQ(@1, result);

  id protectedResult = web::test::ExecuteJavaScript(
      web_view(), [NSString stringWithFormat:@"%s.prototype.testValue",
                                             safe_builtin.c_str()]);
  EXPECT_NSNE(@1, protectedResult);
}

// Tests that the safe builtins prototype is frozen and cannot be altered
TEST_P(SafeBuiltinsJavaScriptTest, TestSafeBuiltinPrototypeIsFrozen) {
  std::string safe_builtin = GetParam().second;

  web::test::ExecuteJavaScriptInWebView(
      web_view(), [NSString stringWithFormat:@"%s.prototype.testValue = 1;",
                                             safe_builtin.c_str()]);

  id protectedResult = web::test::ExecuteJavaScript(
      web_view(), [NSString stringWithFormat:@"%s.prototype.testValue",
                                             safe_builtin.c_str()]);
  EXPECT_NSNE(@1, protectedResult);
}

// Tests that editing a value on the builtin is does not alter it on the
// equivalent safe builtin
TEST_P(SafeBuiltinsJavaScriptTest, TestBuiltinAssignment) {
  std::string builtin = GetParam().first;
  std::string safe_builtin = GetParam().second;

  web::test::ExecuteJavaScriptInWebView(
      web_view(),
      [NSString stringWithFormat:@"%s.name = function() { return 1; }",
                                 builtin.c_str()]);

  id protectedResult = web::test::ExecuteJavaScript(
      web_view(), [NSString stringWithFormat:@"%s.name", safe_builtin.c_str()]);
  EXPECT_NSNE(@1, protectedResult);
}

// Tests that the safe builtin is frozen and cannot be altered
TEST_P(SafeBuiltinsJavaScriptTest, TestSafeBuiltinIsFrozen) {
  std::string builtin = GetParam().first;
  std::string safe_builtin = GetParam().second;

  web::test::ExecuteJavaScriptInWebView(
      web_view(),
      [NSString stringWithFormat:@"%s.name = function() { return 1; }",
                                 safe_builtin.c_str()]);

  id protectedResult = web::test::ExecuteJavaScript(
      web_view(), [NSString stringWithFormat:@"%s.name", safe_builtin.c_str()]);
  EXPECT_NSNE(@1, protectedResult);
}
