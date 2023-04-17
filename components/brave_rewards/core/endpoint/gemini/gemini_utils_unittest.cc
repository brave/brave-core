/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/endpoint/gemini/gemini_utils.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/test/bat_ledger_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiUtilsTest.*

namespace brave_rewards::internal {
namespace endpoint {
namespace gemini {

class GeminiUtilsTest : public testing::Test {};

TEST_F(GeminiUtilsTest, GetApiServerUrlDevelopment) {
  _environment = mojom::Environment::DEVELOPMENT;
  const std::string url = GetApiServerUrl("/test");
  ASSERT_EQ(url, base::StrCat({BUILDFLAG(GEMINI_API_STAGING_URL), "/test"}));
}

TEST_F(GeminiUtilsTest, GetApiServerUrlStaging) {
  _environment = mojom::Environment::STAGING;
  const std::string url = GetApiServerUrl("/test");
  ASSERT_EQ(url, base::StrCat({BUILDFLAG(GEMINI_API_STAGING_URL), "/test"}));
}

TEST_F(GeminiUtilsTest, GetApiServerUrlProduction) {
  _environment = mojom::Environment::PRODUCTION;
  const std::string url = GetApiServerUrl("/test");
  ASSERT_EQ(url, base::StrCat({BUILDFLAG(GEMINI_API_URL), "/test"}));
}

TEST_F(GeminiUtilsTest, GetOauthServerUrlDevelopment) {
  _environment = mojom::Environment::DEVELOPMENT;
  const std::string url = GetOauthServerUrl("/test");
  ASSERT_EQ(url, base::StrCat({BUILDFLAG(GEMINI_OAUTH_STAGING_URL), "/test"}));
}

TEST_F(GeminiUtilsTest, GetOauthServerUrlStaging) {
  _environment = mojom::Environment::STAGING;
  const std::string url = GetOauthServerUrl("/test");
  ASSERT_EQ(url, base::StrCat({BUILDFLAG(GEMINI_OAUTH_STAGING_URL), "/test"}));
}

TEST_F(GeminiUtilsTest, GetOauthServerUrlProduction) {
  _environment = mojom::Environment::PRODUCTION;
  const std::string url = GetOauthServerUrl("/test");
  ASSERT_EQ(url, base::StrCat({BUILDFLAG(GEMINI_OAUTH_URL), "/test"}));
}

TEST_F(GeminiUtilsTest, CheckStatusCodeTest) {
  ASSERT_EQ(CheckStatusCode(net::HTTP_UNAUTHORIZED),
            mojom::Result::EXPIRED_TOKEN);
  ASSERT_EQ(CheckStatusCode(net::HTTP_FORBIDDEN), mojom::Result::EXPIRED_TOKEN);
  ASSERT_EQ(CheckStatusCode(net::HTTP_NOT_FOUND), mojom::Result::NOT_FOUND);
  ASSERT_EQ(CheckStatusCode(net::HTTP_BAD_REQUEST),
            mojom::Result::LEDGER_ERROR);
  ASSERT_EQ(CheckStatusCode(net::HTTP_OK), mojom::Result::LEDGER_OK);
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace brave_rewards::internal
