// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"

#include <memory>
#include <tuple>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/random/random.h"
#include "url/gurl.h"

namespace {
const uint64_t kTestSessionToken = 123456789;
const uint64_t kAnotherTestSessionToken = 45678;
}  // namespace

class BraveFarblingServiceTest : public testing::Test {
 public:
  BraveFarblingServiceTest() = default;
  BraveFarblingServiceTest(const BraveFarblingServiceTest&) = delete;
  BraveFarblingServiceTest& operator=(const BraveFarblingServiceTest&) = delete;
  ~BraveFarblingServiceTest() override = default;

  void SetUp() override {
    farbling_service_ = std::make_unique<brave::BraveFarblingService>();
    farbling_service_->set_session_tokens_for_testing(kTestSessionToken);
  }

  brave::BraveFarblingService* farbling_service() {
    return farbling_service_.get();
  }

 private:
  std::unique_ptr<brave::BraveFarblingService> farbling_service_;
};

TEST_F(BraveFarblingServiceTest, SessionTokens) {
  EXPECT_EQ(farbling_service()->session_token(), kTestSessionToken);
}

TEST_F(BraveFarblingServiceTest, PRNGKnownValues) {
  const std::array<std::tuple<GURL, uint64_t>, 2> test_cases = {
      std::make_tuple<>(GURL("http://a.com"), 16188622623906601575UL),
      std::make_tuple<>(GURL("http://b.com"), 10059331952077172763UL),
  };
  for (const auto& c : test_cases) {
    brave::FarblingPRNG prng;
    ASSERT_TRUE(farbling_service()->MakePseudoRandomGeneratorForURL(
        std::get<0>(c), &prng));
    EXPECT_EQ(prng(), std::get<1>(c));
  }
}

TEST_F(BraveFarblingServiceTest, PRNGKnownValuesDifferentSeeds) {
  farbling_service()->set_session_tokens_for_testing(kAnotherTestSessionToken);
  const std::array<std::tuple<GURL, uint64_t>, 2> test_cases = {
      std::make_tuple<>(GURL("http://a.com"), 6565599272117158152UL),
      std::make_tuple<>(GURL("http://b.com"), 10499595974068024348UL),
  };
  for (const auto& c : test_cases) {
    brave::FarblingPRNG prng;
    ASSERT_TRUE(farbling_service()->MakePseudoRandomGeneratorForURL(
        std::get<0>(c), &prng));
    EXPECT_EQ(prng(), std::get<1>(c));
  }
}

TEST_F(BraveFarblingServiceTest, InvalidDomains) {
  const std::array<GURL, 8> test_cases = {
      GURL("about:blank"),
      GURL("brave://settings"),
      GURL("chrome://version"),
      GURL("file:///etc/passwd"),
      GURL("javascript:alert(1)"),
      GURL("data:text/plain;base64,"),
      GURL(""),
  };
  for (const auto& url : test_cases) {
    brave::FarblingPRNG prng;
    EXPECT_FALSE(
        farbling_service()->MakePseudoRandomGeneratorForURL(url, &prng));
    EXPECT_FALSE(
        farbling_service()->MakePseudoRandomGeneratorForURL(url, &prng));
  }
}
