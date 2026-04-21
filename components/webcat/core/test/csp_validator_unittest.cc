/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/core/csp_validator.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace webcat {

TEST(CspValidatorTest, ValidMinimalCsp) {
  auto result = ValidateCsp("default-src 'none'");
  EXPECT_TRUE(result.is_valid);
}

TEST(CspValidatorTest, ValidDefaultSelfObjectNone) {
  auto result = ValidateCsp("default-src 'self'; object-src 'none'");
  EXPECT_TRUE(result.is_valid);
}

TEST(CspValidatorTest, ValidDefaultSelfScriptSelf) {
  auto result = ValidateCsp(
      "default-src 'self'; object-src 'none'; script-src 'self'");
  EXPECT_TRUE(result.is_valid);
}

TEST(CspValidatorTest, ValidDefaultSelfScriptWasm) {
  auto result = ValidateCsp(
      "default-src 'self'; object-src 'none'; script-src 'wasm-unsafe-eval'");
  EXPECT_TRUE(result.is_valid);
}

TEST(CspValidatorTest, ValidDefaultSelfScriptHash) {
  auto result = ValidateCsp(
      "default-src 'self'; object-src 'none'; script-src 'sha256-abc123'");
  EXPECT_TRUE(result.is_valid);
}

TEST(CspValidatorTest, ValidStyleInline) {
  auto result = ValidateCsp(
      "default-src 'self'; object-src 'none'; style-src 'unsafe-inline'");
  EXPECT_TRUE(result.is_valid);
}

TEST(CspValidatorTest, ValidFrameSrc) {
  auto result = ValidateCsp(
      "default-src 'self'; object-src 'none'; frame-src 'self' blob: data:");
  EXPECT_TRUE(result.is_valid);
}

TEST(CspValidatorTest, ValidWorkerSrc) {
  auto result = ValidateCsp(
      "default-src 'self'; object-src 'none'; worker-src 'self'");
  EXPECT_TRUE(result.is_valid);
}

TEST(CspValidatorTest, RejectsCommaInCsp) {
  auto result = ValidateCsp(
      "default-src 'self', script-src 'self'");
  EXPECT_FALSE(result.is_valid);
  EXPECT_EQ(result.error_detail,
            "CSP must not contain commas (multiple policies)");
}

TEST(CspValidatorTest, RejectsMissingDefaultSrc) {
  auto result = ValidateCsp("script-src 'self'");
  EXPECT_FALSE(result.is_valid);
  EXPECT_EQ(result.error_detail,
            "CSP must include default-src directive");
}

TEST(CspValidatorTest, RejectsObjectSrcNotNone) {
  auto result = ValidateCsp("default-src 'self'; object-src 'self'");
  EXPECT_FALSE(result.is_valid);
}

TEST(CspValidatorTest, RejectsMissingObjectSrcWhenDefaultNotNone) {
  auto result = ValidateCsp("default-src 'self'; script-src 'self'");
  EXPECT_FALSE(result.is_valid);
}

TEST(CspValidatorTest, AllowsObjectSrcNoneWithDefaultNone) {
  auto result = ValidateCsp("default-src 'none'; object-src 'none'");
  EXPECT_TRUE(result.is_valid);
}

TEST(CspValidatorTest, RejectsScriptUnsafeEval) {
  auto result = ValidateCsp(
      "default-src 'self'; object-src 'none'; script-src 'unsafe-eval'");
  EXPECT_FALSE(result.is_valid);
}

TEST(CspValidatorTest, RejectsScriptUnsafeInline) {
  auto result = ValidateCsp(
      "default-src 'self'; object-src 'none'; script-src 'unsafe-inline'");
  EXPECT_FALSE(result.is_valid);
}

TEST(CspValidatorTest, RejectsFrameSrcHttp) {
  auto result = ValidateCsp(
      "default-src 'self'; object-src 'none'; frame-src https://evil.com");
  EXPECT_FALSE(result.is_valid);
}

TEST(CspValidatorTest, RejectsWorkerSrcNoneSelf) {
  auto result = ValidateCsp(
      "default-src 'self'; object-src 'none'; worker-src blob:");
  EXPECT_FALSE(result.is_valid);
}

TEST(CspValidatorTest, EmptyCsp) {
  auto result = ValidateCsp("");
  EXPECT_FALSE(result.is_valid);
  EXPECT_EQ(result.error_detail, "CSP is empty");
}

TEST(CspValidatorTest, GetEffectiveCspDefault) {
  std::map<std::string, std::string> extra_csp;
  std::string result = GetEffectiveCspForPath("/style.css", "default-src 'self'", extra_csp);
  EXPECT_EQ(result, "default-src 'self'");
}

TEST(CspValidatorTest, GetEffectiveCspExactMatch) {
  std::map<std::string, std::string> extra_csp = {
      {"/admin/", "default-src 'self'; script-src 'none'"},
  };
  std::string result = GetEffectiveCspForPath("/admin/dashboard.html",
                                               "default-src 'self'", extra_csp);
  EXPECT_EQ(result, "default-src 'self'; script-src 'none'");
}

TEST(CspValidatorTest, GetEffectiveCspLongestPrefix) {
  std::map<std::string, std::string> extra_csp = {
      {"/", "default-src 'self'"},
      {"/admin/", "default-src 'self'; script-src 'none'"},
  };
  std::string result = GetEffectiveCspForPath("/admin/panel.html",
                                               "default-src 'self'", extra_csp);
  EXPECT_EQ(result, "default-src 'self'; script-src 'none'");
}

TEST(CspValidatorTest, GetEffectiveCspTrailingSlash) {
  std::map<std::string, std::string> extra_csp;
  std::string result = GetEffectiveCspForPath(
      "/", "default-src 'self'", extra_csp);
  EXPECT_EQ(result, "default-src 'self'");
}

}  // namespace webcat