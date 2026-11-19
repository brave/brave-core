/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/core/browser/psst_settings_service.h"

#include "base/test/scoped_feature_list.h"
#include "brave/browser/psst/psst_settings_service_factory.h"
#include "brave/components/psst/core/common/features.h"
#include "brave/components/psst/core/common/psst_metadata_schema.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

base::ListValue VectorToList(const std::vector<std::string>& values) {
  base::ListValue list;
  for (auto& value : values) {
    list.Append(value);
  }
  return list;
}

base::DictValue CreatePsstSettingsDict(
    psst::ConsentStatus consent_status,
    int script_version,
    const std::string& user_id,
    const std::vector<std::string>& uids_to_perform) {
  base::DictValue object;
  object.Set("user_id", user_id);
  object.Set("consent_status", ToString(consent_status));
  object.Set("script_version", script_version);
  object.Set("uids_to_perform", VectorToList(uids_to_perform));
  return object;
}

size_t GetContentSettingsCountByOrigin(const HostContentSettingsMap* map,
                                       const url::Origin& origin) {
  auto metadata_objects = map->GetWebsiteSetting(
      origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST);
  return metadata_objects.GetIfDict() ? metadata_objects.GetIfDict()->size()
                                      : 0;
}

}  // namespace

namespace psst {

class PsstSettingsServiceUnitTest : public testing::Test {
 public:
  void SetUp() override {
    map_ = HostContentSettingsMapFactory::GetForProfile(&profile_);
    ASSERT_TRUE(map_);
    psst_settings_service_ =
        PsstSettingsServiceFactory::GetForProfile(&profile_);
    ASSERT_TRUE(psst_settings_service_);
  }

  HostContentSettingsMap* map() { return map_.get(); }
  PsstSettingsService* settings_service() { return psst_settings_service_; }

 private:
  base::test::ScopedFeatureList feature_list_{psst::features::kEnablePsst};
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  scoped_refptr<HostContentSettingsMap> map_;
  raw_ptr<PsstSettingsService> psst_settings_service_;
};

TEST_F(PsstSettingsServiceUnitTest, DontAllowToSaveMetadataForWrongSchema) {
  const auto first_metadata = PsstWebsiteSettings::FromValue(
      CreatePsstSettingsDict(ConsentStatus::kAllow, 1, "user123", {}));
  ASSERT_TRUE(first_metadata);
  const url::Origin http_scheme_origin =
      url::Origin::Create(GURL("http://a.test"));
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      http_scheme_origin, first_metadata->user_id));
  settings_service()->SetPsstWebsiteSettings(
      http_scheme_origin, first_metadata->consent_status,
      first_metadata->script_version, first_metadata->user_id,
      base::ListValue());
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      http_scheme_origin, first_metadata->user_id));
  const url::Origin file_scheme_origin =
      url::Origin::Create(GURL("file://a.test"));
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      file_scheme_origin, first_metadata->user_id));
  settings_service()->SetPsstWebsiteSettings(
      file_scheme_origin, first_metadata->consent_status,
      first_metadata->script_version, first_metadata->user_id,
      base::ListValue());
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      file_scheme_origin, first_metadata->user_id));
  const url::Origin brave_scheme_origin =
      url::Origin::Create(GURL("brave://a.test"));
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      brave_scheme_origin, first_metadata->user_id));
  settings_service()->SetPsstWebsiteSettings(
      brave_scheme_origin, first_metadata->consent_status,
      first_metadata->script_version, first_metadata->user_id,
      base::ListValue());
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      brave_scheme_origin, first_metadata->user_id));

  const url::Origin chrome_scheme_origin =
      url::Origin::Create(GURL("chrome://a.test"));
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      chrome_scheme_origin, first_metadata->user_id));
  settings_service()->SetPsstWebsiteSettings(
      chrome_scheme_origin, first_metadata->consent_status,
      first_metadata->script_version, first_metadata->user_id,
      base::ListValue());
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      chrome_scheme_origin, first_metadata->user_id));
}

TEST_F(PsstSettingsServiceUnitTest, CreateOrUpdateMetadata) {
  const url::Origin origin = url::Origin::Create(GURL("https://a.test"));
  const std::string first_user_id = "first-user123";
  const std::string second_user_id = "second-user123";

  ASSERT_FALSE(
      settings_service()->GetPsstWebsiteSettings(origin, first_user_id));

  const auto first_metadata =
      PsstWebsiteSettings::FromValue(CreatePsstSettingsDict(
          ConsentStatus::kAllow, 1, first_user_id, std::vector<std::string>()));
  const auto second_metadata = PsstWebsiteSettings::FromValue(
      CreatePsstSettingsDict(ConsentStatus::kAllow, 1, second_user_id,
                             std::vector<std::string>()));
  ASSERT_EQ(GetContentSettingsCountByOrigin(map(), origin), 0u);

  settings_service()->SetPsstWebsiteSettings(
      origin, first_metadata->consent_status, first_metadata->script_version,
      first_metadata->user_id, base::ListValue());

  ASSERT_EQ(GetContentSettingsCountByOrigin(map(), origin), 1u);
  settings_service()->SetPsstWebsiteSettings(
      origin, second_metadata->consent_status, second_metadata->script_version,
      second_metadata->user_id, base::ListValue());
  ASSERT_EQ(GetContentSettingsCountByOrigin(map(), origin), 2u);

  auto first_metadata_value = settings_service()->GetPsstWebsiteSettings(
      origin, first_metadata->user_id);
  ASSERT_TRUE(first_metadata_value.has_value());
  ASSERT_EQ(first_metadata_value.value().consent_status,
            first_metadata->consent_status);
  ASSERT_EQ(first_metadata_value.value().script_version,
            first_metadata->script_version);
  ASSERT_EQ(first_metadata_value.value().user_id, first_metadata->user_id);
  ASSERT_EQ(first_metadata_value.value().uids_to_perform,
            first_metadata->uids_to_perform);

  auto second_metadata_value = settings_service()->GetPsstWebsiteSettings(
      origin, second_metadata->user_id);
  ASSERT_TRUE(second_metadata_value.has_value());
  ASSERT_EQ(second_metadata_value.value().consent_status,
            second_metadata->consent_status);
  ASSERT_EQ(second_metadata_value.value().script_version,
            second_metadata->script_version);
  ASSERT_EQ(second_metadata_value.value().user_id, second_metadata->user_id);
  ASSERT_EQ(second_metadata_value.value().uids_to_perform,
            second_metadata->uids_to_perform);

  const auto modified_metadata =
      PsstWebsiteSettings::FromValue(CreatePsstSettingsDict(
          ConsentStatus::kBlock, first_metadata->script_version,
          first_metadata->user_id, std::vector<std::string>()));
  settings_service()->SetPsstWebsiteSettings(
      origin, modified_metadata->consent_status,
      modified_metadata->script_version, modified_metadata->user_id,
      base::ListValue());

  auto modified_metadata_value = settings_service()->GetPsstWebsiteSettings(
      origin, modified_metadata->user_id);
  ASSERT_TRUE(modified_metadata_value.has_value());
  ASSERT_EQ(modified_metadata_value.value().consent_status,
            modified_metadata->consent_status);
  ASSERT_EQ(modified_metadata_value.value().script_version,
            modified_metadata->script_version);
  ASSERT_EQ(modified_metadata_value.value().user_id,
            modified_metadata->user_id);
  ASSERT_EQ(modified_metadata_value.value().uids_to_perform,
            modified_metadata->uids_to_perform);
}

TEST_F(PsstSettingsServiceUnitTest, InfobarCounterIsPerOrigin) {
  const url::Origin origin_a = url::Origin::Create(GURL("https://a.test"));
  const url::Origin origin_b = url::Origin::Create(GURL("https://b.test"));

  EXPECT_EQ(settings_service()->GetInfobarShownCount(origin_a), 0);
  EXPECT_EQ(settings_service()->GetInfobarShownCount(origin_b), 0);
  EXPECT_EQ(settings_service()->GetInfobarDeclinedCount(origin_a), 0);
  EXPECT_EQ(settings_service()->GetInfobarDeclinedCount(origin_b), 0);

  settings_service()->IncrementInfobarShownCount(origin_a);
  settings_service()->IncrementInfobarDeclineCount(origin_a);
  EXPECT_EQ(settings_service()->GetInfobarShownCount(origin_a), 1);
  EXPECT_EQ(settings_service()->GetInfobarShownCount(origin_b), 0);
  EXPECT_EQ(settings_service()->GetInfobarDeclinedCount(origin_a), 1);
  EXPECT_EQ(settings_service()->GetInfobarDeclinedCount(origin_b), 0);

  settings_service()->IncrementInfobarShownCount(origin_a);
  settings_service()->IncrementInfobarShownCount(origin_b);
  EXPECT_EQ(settings_service()->GetInfobarShownCount(origin_a), 2);
  EXPECT_EQ(settings_service()->GetInfobarShownCount(origin_b), 1);

  settings_service()->IncrementInfobarDeclineCount(origin_a);
  settings_service()->IncrementInfobarDeclineCount(origin_b);
  EXPECT_EQ(settings_service()->GetInfobarDeclinedCount(origin_a), 2);
  EXPECT_EQ(settings_service()->GetInfobarDeclinedCount(origin_b), 1);
}

TEST_F(PsstSettingsServiceUnitTest,
       InfobarCounterDoesNotAffectWebsiteSettings) {
  const url::Origin origin = url::Origin::Create(GURL("https://a.test"));
  const std::string user_id = "user123";
  const auto metadata = PsstWebsiteSettings::FromValue(CreatePsstSettingsDict(
      ConsentStatus::kAllow, 1, user_id, std::vector<std::string>()));
  ASSERT_TRUE(metadata);

  settings_service()->SetPsstWebsiteSettings(
      origin, metadata->consent_status, metadata->script_version,
      metadata->user_id, base::ListValue());
  settings_service()->IncrementInfobarShownCount(origin);
  settings_service()->IncrementInfobarShownCount(origin);
  settings_service()->IncrementInfobarDeclineCount(origin);

  EXPECT_EQ(settings_service()->GetInfobarShownCount(origin), 2);
  EXPECT_EQ(settings_service()->GetInfobarDeclinedCount(origin), 1);
  const auto website_settings =
      settings_service()->GetPsstWebsiteSettings(origin, user_id);
  ASSERT_TRUE(website_settings.has_value());
  EXPECT_EQ(website_settings->consent_status, metadata->consent_status);
  EXPECT_EQ(website_settings->script_version, metadata->script_version);
}

TEST_F(PsstSettingsServiceUnitTest, IncrementInfobarCounterIgnoresNonHttps) {
  const url::Origin origin = url::Origin::Create(GURL("http://a.test"));
  settings_service()->IncrementInfobarShownCount(origin);
  settings_service()->IncrementInfobarDeclineCount(origin);
  EXPECT_EQ(settings_service()->GetInfobarShownCount(origin), 0);
  EXPECT_EQ(settings_service()->GetInfobarDeclinedCount(origin), 0);
}

TEST_F(PsstSettingsServiceUnitTest,
       GetAllPsstWebsitesSettingsReturnsPerHostData) {
  const url::Origin origin_a = url::Origin::Create(GURL("https://a.test"));
  const url::Origin origin_b = url::Origin::Create(GURL("https://b.test"));
  const std::string user_id1 = "user-id1";
  const std::string user_id2 = "user-id2";
  const std::vector<std::string> uids_to_perform = {"1", "2", "3", "4"};

  const auto metadata1 = PsstWebsiteSettings::FromValue(CreatePsstSettingsDict(
      ConsentStatus::kAllow, 8, user_id1, uids_to_perform));
  const auto metadata2 = PsstWebsiteSettings::FromValue(CreatePsstSettingsDict(
      ConsentStatus::kAllow, 8, user_id2, uids_to_perform));
  ASSERT_TRUE(metadata1);
  ASSERT_TRUE(metadata2);

  settings_service()->SetPsstWebsiteSettings(
      origin_a, metadata1->consent_status, metadata1->script_version,
      metadata1->user_id, VectorToList(metadata1->uids_to_perform));
  settings_service()->SetPsstWebsiteSettings(
      origin_a, metadata2->consent_status, metadata2->script_version,
      metadata2->user_id, VectorToList(metadata2->uids_to_perform));

  settings_service()->IncrementInfobarShownCount(origin_a);
  settings_service()->IncrementInfobarShownCount(origin_a);
  settings_service()->IncrementInfobarDeclineCount(origin_a);
  settings_service()->IncrementInfobarShownCount(origin_b);

  const auto all_settings = settings_service()->GetAllPsstWebsitesSettings();
  ASSERT_EQ(all_settings.size(), 2u);

  const auto it_a = all_settings.find("a.test");
  ASSERT_NE(it_a, all_settings.end());
  EXPECT_EQ(it_a->second.infobar_shown_counter, 2);
  EXPECT_EQ(it_a->second.infobar_decline_counter, 1);
  ASSERT_EQ(it_a->second.website_settings.size(), 2u);
  EXPECT_EQ(it_a->second.website_settings[0].user_id, user_id1);
  EXPECT_EQ(it_a->second.website_settings[0].uids_to_perform, uids_to_perform);
  EXPECT_EQ(it_a->second.website_settings[1].user_id, user_id2);

  const auto it_b = all_settings.find("b.test");
  ASSERT_NE(it_b, all_settings.end());
  EXPECT_EQ(it_b->second.infobar_shown_counter, 1);
  EXPECT_EQ(it_b->second.infobar_decline_counter, 0);
  EXPECT_TRUE(it_b->second.website_settings.empty());
}

}  // namespace psst
