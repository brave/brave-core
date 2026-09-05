/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/external_protocol/external_protocol_handler.h"

#include <utility>

#include "base/values.h"
#include "chrome/browser/external_protocol/constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

// Brave comments out upstream's mailto fast path in GetBlockState(), which
// returned DONT_BLOCK before the policy and per-origin preference checks ran.
// See brave/rewrite/chrome/browser/external_protocol/
// external_protocol_handler.cc.yaml.
class BraveExternalProtocolHandlerTest : public testing::Test {
 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
};

TEST_F(BraveExternalProtocolHandlerTest, MailtoIsNotAllowedByDefault) {
  EXPECT_EQ(
      ExternalProtocolHandler::UNKNOWN,
      ExternalProtocolHandler::GetBlockState("mailto", nullptr, &profile_));
}

TEST_F(BraveExternalProtocolHandlerTest, MailtoAllowedByEnterprisePolicy) {
  const url::Origin origin = url::Origin::Create(GURL("https://example.test"));

  base::ListValue origins;
  origins.Append("https://example.test");
  base::DictValue entry;
  entry.Set(policy::external_protocol::kProtocolNameKey, "mailto");
  entry.Set(policy::external_protocol::kOriginListKey, std::move(origins));
  base::ListValue policy;
  policy.Append(std::move(entry));
  profile_.GetPrefs()->SetList(prefs::kAutoLaunchProtocolsFromOrigins,
                               std::move(policy));

  EXPECT_EQ(
      ExternalProtocolHandler::DONT_BLOCK,
      ExternalProtocolHandler::GetBlockState("mailto", &origin, &profile_));
}

TEST_F(BraveExternalProtocolHandlerTest, MailtoAllowedByRememberedPreference) {
  const url::Origin origin = url::Origin::Create(GURL("https://example.test"));

  ExternalProtocolHandler::SetBlockState(
      "mailto", origin, ExternalProtocolHandler::DONT_BLOCK, &profile_);

  EXPECT_EQ(
      ExternalProtocolHandler::DONT_BLOCK,
      ExternalProtocolHandler::GetBlockState("mailto", &origin, &profile_));
}
