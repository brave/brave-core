// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/common/prefs.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

namespace {

const char kPsstSettingsTemplate[] = R"({
        "enable_psst": true,
        "{name}": {
          "{user-id}": {
            "consent_status": {cons_stat},
            "script_version": {scr_ver},
            "urls_to_skip": {uts}
          }
        }
      })";

base::Value::Dict CreatePsstSettingsDict(
    const std::string& name,
    const std::string& user_id,
    const prefs::ConsentStatus consent_status,
    const int script_version,
    const base::Value::List& urls_to_skip = {}) {
  std::string json_str = kPsstSettingsTemplate;
  base::ReplaceSubstringsAfterOffset(&json_str, 0, "{name}", name);
  base::ReplaceSubstringsAfterOffset(&json_str, 0, "{user-id}", user_id);
  base::ReplaceSubstringsAfterOffset(&json_str, 0, "{scr_ver}",
                                     base::NumberToString(script_version));
  base::ReplaceSubstringsAfterOffset(
      &json_str, 0, "{cons_stat}",
      base::NumberToString(static_cast<int>(consent_status)));

  std::string urls_to_skip_json;
  base::JSONWriter::Write(urls_to_skip, &urls_to_skip_json);
  base::ReplaceSubstringsAfterOffset(&json_str, 0, "{uts}", urls_to_skip_json);
  return base::test::ParseJsonDict(json_str);
}

}  // namespace

class PsstPrefsTest : public ::testing::Test {
 public:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kEnablePsst);
    RegisterProfilePrefs(pref_service_.registry());
  }

  TestingPrefServiceSimple pref_service_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(PsstPrefsTest, RetrievingPsstSettingsFromPrefs) {
  const std::string name = "linkedin";
  const std::string user_id = "test-user-id";
  const prefs::ConsentStatus consent_status = prefs::ConsentStatus::kBlock;
  const int script_version = 5;
  base::Value::List urls_to_skip;
  urls_to_skip.Append("https://example.com");
  urls_to_skip.Append("https://example1.com");

  EXPECT_FALSE(prefs::GetConsentStatus(name, user_id, pref_service_));
  EXPECT_FALSE(prefs::GetScriptVersion(name, user_id, pref_service_));
  EXPECT_FALSE(prefs::GetUrlsToSkip(name, user_id, pref_service_));

  auto json_value = CreatePsstSettingsDict(name, user_id, consent_status,
                                           script_version, urls_to_skip);
  pref_service_.SetDict(prefs::kPsstSettingsPref, json_value.Clone());

  auto consent_status_result =
      prefs::GetConsentStatus(name, user_id, pref_service_);
  EXPECT_TRUE(consent_status_result);
  EXPECT_EQ(*consent_status_result, consent_status);

  auto script_version_result =
      prefs::GetScriptVersion(name, user_id, pref_service_);
  EXPECT_TRUE(script_version_result);
  EXPECT_EQ(*script_version_result, script_version);

  auto urls_to_skip_result = prefs::GetUrlsToSkip(name, user_id, pref_service_);
  EXPECT_TRUE(urls_to_skip_result);
  EXPECT_EQ(*urls_to_skip_result, urls_to_skip);
}

TEST_F(PsstPrefsTest, UpdatePsstSettingsFromPrefs) {
  const std::string name = "linkedin";
  const std::string user_id = "test-user-id";
  const prefs::ConsentStatus consent_status = prefs::ConsentStatus::kBlock;
  const int script_version = 5;
  base::Value::List urls_to_skip;
  urls_to_skip.Append("https://example.com");
  urls_to_skip.Append("https://example1.com");

  prefs::SetPsstSettings(name, user_id, consent_status, script_version,
                         urls_to_skip.Clone(), pref_service_);

  auto consent_status_result =
      prefs::GetConsentStatus(name, user_id, pref_service_);
  EXPECT_TRUE(consent_status_result);
  EXPECT_EQ(*consent_status_result, consent_status);

  auto script_version_result =
      prefs::GetScriptVersion(name, user_id, pref_service_);
  EXPECT_TRUE(script_version_result);
  EXPECT_EQ(*script_version_result, script_version);

  auto urls_to_skip_result = prefs::GetUrlsToSkip(name, user_id, pref_service_);
  EXPECT_TRUE(urls_to_skip_result);
  EXPECT_EQ(*urls_to_skip_result, urls_to_skip);
}

}  // namespace psst
