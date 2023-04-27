/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/brave_browsing_data_remover_delegate.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class BraveBrowsingDataRemoverDelegateTest : public testing::Test {
 public:
  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    map_ = HostContentSettingsMapFactory::GetForProfile(profile());
  }

  Profile* profile() { return profile_.get(); }

  HostContentSettingsMap* map() { return map_.get(); }

  BraveBrowsingDataRemoverDelegate* delegate() {
    return static_cast<BraveBrowsingDataRemoverDelegate*>(
        profile()->GetBrowsingDataRemoverDelegate());
  }

  static std::vector<ContentSettingsType> GetExpectedTypesToBeRemoved() {
    std::vector<ContentSettingsType> types_to_remove;
    for (int32_t type = static_cast<int32_t>(ContentSettingsType::BRAVE_START);
         type < static_cast<int32_t>(ContentSettingsType::NUM_TYPES); ++type) {
      types_to_remove.push_back(static_cast<ContentSettingsType>(type));
    }
    return types_to_remove;
  }

  base::flat_map<ContentSettingsType, size_t> GetBraveSettingsCount() {
    base::flat_map<ContentSettingsType, size_t> count_per_type;
    for (const auto& content_type : GetExpectedTypesToBeRemoved()) {
      ContentSettingsForOneType settings;
      map()->GetSettingsForOneType(content_type, &settings);
      count_per_type[content_type] = settings.size();
    }
    return count_per_type;
  }

 private:
  content::BrowserTaskEnvironment task_environment_;

  std::unique_ptr<TestingProfile> profile_;
  scoped_refptr<HostContentSettingsMap> map_;
};

TEST_F(BraveBrowsingDataRemoverDelegateTest, BraveSettingsClearTest) {
  const GURL kBraveURL("https://www.brave.com");
  const GURL kBatURL("https://basicattentiontoken.org");
  const GURL kGoogleURL("https://www.google.com");
  const GURL kAbcURL("https://www.abc.com");

  // defaults
  //size_t start_count = GetBraveSettingsCount();

  const auto initial_count = GetBraveSettingsCount();

  const auto& types_to_be_removed = GetExpectedTypesToBeRemoved();
  for (const ContentSettingsType type : types_to_be_removed) {
    map()->SetContentSettingDefaultScope(kBraveURL, GURL(), type,
                                         CONTENT_SETTING_ALLOW);
  }
  map()->SetContentSettingCustomScope(
      brave_shields::GetPatternFromURL(kGoogleURL),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::JAVASCRIPT,
      CONTENT_SETTING_BLOCK);

  // Check current shields settings count is the defaults plus 2 and zero after
  // clearing 1 day time range.
  for (const auto& type_count : GetBraveSettingsCount()) {
    const ContentSettingsType content_type = type_count.first;
    EXPECT_EQ(initial_count.at(content_type) + 1u, type_count.second)
        << static_cast<int32_t>(content_type);
  }

  const base::Time end_time = base::Time::Now();
  const base::Time begin_time = end_time - base::Days(1);
  delegate()->ClearBraveContentSettings(begin_time, end_time);

  for (const auto& type_count : GetBraveSettingsCount()) {
    const ContentSettingsType content_type = type_count.first;
    EXPECT_EQ(initial_count.at(content_type), type_count.second)
        << static_cast<int32_t>(content_type);
  }
}
