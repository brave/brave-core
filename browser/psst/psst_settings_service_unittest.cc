/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/core/psst_settings_service.h"

#include "base/test/scoped_feature_list.h"
#include "brave/browser/psst/psst_settings_service_factory.h"
#include "brave/components/psst/browser/core/brave_psst_utils.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/psst_metadata_schema.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

base::Value::List VectorToList(const std::vector<std::string>& values) {
  base::Value::List list;
  for (auto& value : values) {
    list.Append(value);
  }
  return list;
}

base::Value::Dict CreatePsstSettingsDict(
    psst::ConsentStatus consent_status,
    int script_version,
    const std::string& user_id,
    const std::vector<std::string>& urls_to_skip) {
  base::Value::Dict object;
  object.Set("user_id", user_id);
  object.Set("consent_status", ToString(consent_status));
  object.Set("script_version", script_version);
  object.Set("urls_to_skip", VectorToList(urls_to_skip));
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
      base::Value::List());
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      http_scheme_origin, first_metadata->user_id));
  const url::Origin file_scheme_origin =
      url::Origin::Create(GURL("file://a.test"));
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      file_scheme_origin, first_metadata->user_id));
  settings_service()->SetPsstWebsiteSettings(
      file_scheme_origin, first_metadata->consent_status,
      first_metadata->script_version, first_metadata->user_id,
      base::Value::List());
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      file_scheme_origin, first_metadata->user_id));
  const url::Origin brave_scheme_origin =
      url::Origin::Create(GURL("brave://a.test"));
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      brave_scheme_origin, first_metadata->user_id));
  settings_service()->SetPsstWebsiteSettings(
      brave_scheme_origin, first_metadata->consent_status,
      first_metadata->script_version, first_metadata->user_id,
      base::Value::List());
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      brave_scheme_origin, first_metadata->user_id));

  const url::Origin chrome_scheme_origin =
      url::Origin::Create(GURL("chrome://a.test"));
  ASSERT_FALSE(settings_service()->GetPsstWebsiteSettings(
      chrome_scheme_origin, first_metadata->user_id));
  settings_service()->SetPsstWebsiteSettings(
      chrome_scheme_origin, first_metadata->consent_status,
      first_metadata->script_version, first_metadata->user_id,
      base::Value::List());
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
      first_metadata->user_id, base::Value::List());

  ASSERT_EQ(GetContentSettingsCountByOrigin(map(), origin), 1u);
  settings_service()->SetPsstWebsiteSettings(
      origin, second_metadata->consent_status, second_metadata->script_version,
      second_metadata->user_id, base::Value::List());
  ASSERT_EQ(GetContentSettingsCountByOrigin(map(), origin), 2u);

  auto first_metadata_value =
      GetPsstWebsiteSettings(map(), origin, first_metadata->user_id);
  ASSERT_TRUE(first_metadata_value.has_value());
  ASSERT_EQ(first_metadata_value.value().consent_status,
            first_metadata->consent_status);
  ASSERT_EQ(first_metadata_value.value().script_version,
            first_metadata->script_version);
  ASSERT_EQ(first_metadata_value.value().user_id, first_metadata->user_id);
  ASSERT_EQ(first_metadata_value.value().urls_to_skip,
            first_metadata->urls_to_skip);

  auto second_metadata_value =
      GetPsstWebsiteSettings(map(), origin, second_metadata->user_id);
  ASSERT_TRUE(second_metadata_value.has_value());
  ASSERT_EQ(second_metadata_value.value().consent_status,
            second_metadata->consent_status);
  ASSERT_EQ(second_metadata_value.value().script_version,
            second_metadata->script_version);
  ASSERT_EQ(second_metadata_value.value().user_id, second_metadata->user_id);
  ASSERT_EQ(second_metadata_value.value().urls_to_skip,
            second_metadata->urls_to_skip);

  const auto modified_metadata =
      PsstWebsiteSettings::FromValue(CreatePsstSettingsDict(
          ConsentStatus::kBlock, first_metadata->script_version,
          first_metadata->user_id, std::vector<std::string>()));
  settings_service()->SetPsstWebsiteSettings(
      origin, modified_metadata->consent_status,
      modified_metadata->script_version, modified_metadata->user_id,
      base::Value::List());

  auto modified_metadata_value =
      GetPsstWebsiteSettings(map(), origin, modified_metadata->user_id);
  ASSERT_TRUE(modified_metadata_value.has_value());
  ASSERT_EQ(modified_metadata_value.value().consent_status,
            modified_metadata->consent_status);
  ASSERT_EQ(modified_metadata_value.value().script_version,
            modified_metadata->script_version);
  ASSERT_EQ(modified_metadata_value.value().user_id,
            modified_metadata->user_id);
  ASSERT_EQ(modified_metadata_value.value().urls_to_skip,
            modified_metadata->urls_to_skip);
}

}  // namespace psst
