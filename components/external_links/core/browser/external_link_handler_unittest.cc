/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/external_links/core/browser/external_link_handler.h"

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace external_links {

class ExternalLinkHandlerTest : public testing::Test {
 protected:
  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());
    ExternalLinkHandler::RegisterProfilePrefs(prefs_.registry());
    settings_map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &prefs_, /*is_off_the_record=*/false, /*store_last_modified=*/false,
        /*restore_session=*/false, /*should_record_metrics=*/false);
    handler_ =
        std::make_unique<ExternalLinkHandler>(settings_map_.get(), &prefs_);
  }

  void TearDown() override {
    handler_.reset();
    settings_map_->ShutdownOnUIThread();
  }

  ExternalLinkHandler& handler() { return *handler_; }

 private:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> settings_map_;
  std::unique_ptr<ExternalLinkHandler> handler_;
};

TEST_F(ExternalLinkHandlerTest, DefaultModeRoundTrips) {
  EXPECT_EQ(handler().GetDefaultMode(), ExternalLinkMode::kCurrent);
  handler().SetDefaultMode(ExternalLinkMode::kModal);
  EXPECT_EQ(handler().GetDefaultMode(), ExternalLinkMode::kModal);
}

TEST_F(ExternalLinkHandlerTest, GetModeFallsBackToDefault) {
  handler().SetDefaultMode(ExternalLinkMode::kPrivateBrowsing);
  EXPECT_EQ(handler().GetMode(GURL("https://example.com")),
            ExternalLinkMode::kPrivateBrowsing);
  EXPECT_EQ(handler().GetMode(GURL()), ExternalLinkMode::kPrivateBrowsing);
}

TEST_F(ExternalLinkHandlerTest, SiteModeAppliesToSubdomains) {
  handler().SetDefaultMode(ExternalLinkMode::kAsk);
  handler().SetSiteMode(GURL("https://www.example.com/path"),
                        ExternalLinkMode::kRegular);
  EXPECT_EQ(handler().GetMode(GURL("https://news.example.com")),
            ExternalLinkMode::kRegular);
  EXPECT_EQ(handler().GetMode(GURL("https://example.org")),
            ExternalLinkMode::kAsk);
}

TEST_F(ExternalLinkHandlerTest, ClearSiteModesKeepsDefault) {
  handler().SetDefaultMode(ExternalLinkMode::kAsk);
  handler().SetSiteMode(GURL("https://example.com"), ExternalLinkMode::kModal);
  handler().ClearSiteModes();
  EXPECT_EQ(handler().GetMode(GURL("https://example.com")),
            ExternalLinkMode::kAsk);
  EXPECT_EQ(handler().GetDefaultMode(), ExternalLinkMode::kAsk);
}

TEST_F(ExternalLinkHandlerTest, SiteModeTakesPrecedenceOverDefault) {
  handler().SetDefaultMode(ExternalLinkMode::kPrivateBrowsing);
  handler().SetSiteMode(GURL("https://example.com"),
                        ExternalLinkMode::kRegular);
  EXPECT_EQ(handler().Resolve(GURL("https://example.com")),
            ExternalLinkAction::kOpenRegular);
  EXPECT_EQ(handler().Resolve(GURL("https://example.org")),
            ExternalLinkAction::kOpenPrivate);
}

TEST_F(ExternalLinkHandlerTest, AskPromptsWithoutSiteMode) {
  handler().SetDefaultMode(ExternalLinkMode::kAsk);
  EXPECT_EQ(handler().Resolve(GURL("https://example.com")),
            ExternalLinkAction::kShowPrompt);
  handler().SetSiteMode(GURL("https://example.com"), ExternalLinkMode::kModal);
  EXPECT_EQ(handler().Resolve(GURL("https://example.com")),
            ExternalLinkAction::kOpenModal);
}

TEST_F(ExternalLinkHandlerTest, OnboardingShownOnce) {
  EXPECT_EQ(handler().Resolve(GURL("https://example.com")),
            ExternalLinkAction::kShowOnboarding);
  EXPECT_TRUE(handler().IsOnboardingShown());
  EXPECT_EQ(handler().Resolve(GURL("https://example.com")),
            ExternalLinkAction::kOpenCurrent);
}

TEST_F(ExternalLinkHandlerTest, NonWebUrlOpensCurrent) {
  handler().SetDefaultMode(ExternalLinkMode::kAsk);
  EXPECT_EQ(handler().Resolve(GURL("file:///tmp/a.html")),
            ExternalLinkAction::kOpenCurrent);
  EXPECT_EQ(handler().Resolve(GURL()), ExternalLinkAction::kOpenCurrent);
  EXPECT_FALSE(handler().IsOnboardingShown());
}

}  // namespace external_links
