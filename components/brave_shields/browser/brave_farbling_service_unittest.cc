/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <random>
#include <tuple>

#include "brave/components/brave_shields/browser/brave_farbling_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {
const uint64_t kTestSessionToken = 123456789;
const uint64_t kTestIncognitoSessionToken = 234567890;
}  // namespace

class BraveFarblingServiceTest : public testing::Test {
 public:
  BraveFarblingServiceTest() = default;
  BraveFarblingServiceTest(const BraveFarblingServiceTest&) = delete;
  BraveFarblingServiceTest& operator=(const BraveFarblingServiceTest&) = delete;
  ~BraveFarblingServiceTest() override = default;

  void SetUp() override {
    farbling_service_ = std::make_unique<brave::BraveFarblingService>();
    farbling_service_->set_session_tokens_for_testing(
        kTestSessionToken, kTestIncognitoSessionToken);
  }

  brave::BraveFarblingService* farbling_service() {
    return farbling_service_.get();
  }

 private:
  std::unique_ptr<brave::BraveFarblingService> farbling_service_;
};

TEST_F(BraveFarblingServiceTest, SessionTokens) {
  EXPECT_EQ(farbling_service()->session_token(false /* is_off_the_record */),
            kTestSessionToken);
  EXPECT_EQ(farbling_service()->session_token(true /* is_off_the_record */),
            kTestIncognitoSessionToken);
}

TEST_F(BraveFarblingServiceTest, PRNGKnownValues) {
  const std::array<std::tuple<GURL, bool, uint64_t>, 4> test_cases = {
      std::make_tuple<>(GURL("http://a.com"), false, 2028179745801958218UL),
      std::make_tuple<>(GURL("http://a.com"), true, 12718887648122721672UL),
      std::make_tuple<>(GURL("http://b.com"), false, 12627633265124970774UL),
      std::make_tuple<>(GURL("http://b.com"), true, 1158566490132461033UL),
  };
  for (const auto& c : test_cases) {
    std::mt19937_64 prng;
    ASSERT_TRUE(farbling_service()->MakePseudoRandomGeneratorForURL(
        std::get<0>(c), std::get<1>(c), &prng));
    EXPECT_EQ(prng(), std::get<2>(c));
  }
}

TEST_F(BraveFarblingServiceTest, InvalidDomains) {
  const std::array<GURL, 8> test_cases = {
      GURL("about:blank"),
      GURL("brave://settings"),
      GURL("chrome://version"),
      GURL("gopher://brave.com"),
      GURL("file:///etc/passwd"),
      GURL("javascript:alert(1)"),
      GURL("data:text/plain;base64,"),
      GURL(""),
  };
  for (const auto& url : test_cases) {
    std::mt19937_64 prng;
    EXPECT_FALSE(
        farbling_service()->MakePseudoRandomGeneratorForURL(url, false, &prng));
    EXPECT_FALSE(
        farbling_service()->MakePseudoRandomGeneratorForURL(url, true, &prng));
  }
}
