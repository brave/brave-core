/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/counters/shields_settings_counter.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/test/simple_test_clock.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/browsing_data/core/browsing_data_utils.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {
const GURL kBraveURL("https://www.brave.com");
const GURL kBatURL("https://basicattentiontoken.org");
const GURL kGoogleURL("https://www.google.com");
const GURL kAbcURL("https://www.abc.com");
}  // namespace

class ShieldsSettingsCounterTest : public testing::Test {
 public:
  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    map_ = static_cast<BraveHostContentSettingsMap*>(
        HostContentSettingsMapFactory::GetForProfile(profile()));

    counter_ = std::make_unique<ShieldsSettingsCounter>(map());
    counter_->Init(profile()->GetPrefs(),
                   browsing_data::ClearBrowsingDataTab::ADVANCED,
                   base::BindRepeating(&ShieldsSettingsCounterTest::Callback,
                                       base::Unretained(this)));
  }

  Profile* profile() { return profile_.get(); }

  BraveHostContentSettingsMap* map() { return map_.get(); }

  ShieldsSettingsCounter* counter() { return counter_.get(); }

  browsing_data::BrowsingDataCounter::ResultInt GetResult() {
    DCHECK(finished_);
    return result_;
  }

  void Callback(
      std::unique_ptr<browsing_data::BrowsingDataCounter::Result> result) {
    DCHECK(result->Finished());
    finished_ = result->Finished();

    result_ = static_cast<browsing_data::BrowsingDataCounter::FinishedResult*>(
                  result.get())
                  ->Value();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;

  scoped_refptr<BraveHostContentSettingsMap> map_;
  std::unique_ptr<ShieldsSettingsCounter> counter_;
  bool finished_;
  browsing_data::BrowsingDataCounter::ResultInt result_;
};

// Tests that the counter correctly counts each setting.
TEST_F(ShieldsSettingsCounterTest, Count) {
  // Below three settings for different host are counted.
  map()->SetContentSettingDefaultScope(
      kBraveURL, GURL(), ContentSettingsType::PLUGINS,
      brave_shields::kHTTPUpgradableResources, CONTENT_SETTING_ALLOW);
  map()->SetContentSettingDefaultScope(
      kBatURL, GURL(), ContentSettingsType::PLUGINS,
      brave_shields::kFingerprinting, CONTENT_SETTING_ALLOW);
  map()->SetContentSettingCustomScope(
      brave_shields::GetPatternFromURL(kGoogleURL, true),
      ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, "", CONTENT_SETTING_BLOCK);

  counter()->Restart();
  EXPECT_EQ(3, GetResult());

  // flash setting is not counted by ShieldsSettingsCounter.
  map()->SetContentSettingDefaultScope(kAbcURL, GURL(), ContentSettingsType::PLUGINS,
                                       "", CONTENT_SETTING_ALLOW);

  counter()->Restart();
  EXPECT_EQ(3, GetResult());
}
