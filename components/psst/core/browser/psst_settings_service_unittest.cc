/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/core/browser/psst_settings_service.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "brave/components/brave_origin/features.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/psst/core/browser/pref_names.h"
#include "brave/components/psst/core/common/features.h"
#include "brave/components/psst/core/common/psst_metadata_schema.h"
#include "brave/components/skus/browser/pref_names.h"
#include "build/build_config.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/policy/core/common/mock_policy_service.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

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

constexpr char kTestProfileId[] = "test-profile-id";

// Minimal delegate: these tests don't exercise SKU connectivity.
class NullBraveOriginDelegate
    : public brave_origin::BraveOriginService::Delegate {
 public:
  void OpenOriginSettings() override {}
  mojo::PendingRemote<skus::mojom::SkusService> GetSkusService() override {
    return {};
  }
};

}  // namespace

namespace psst {

class PsstSettingsServiceUnitTest : public testing::Test {
 public:
  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());
    psst::RegisterProfilePrefs(prefs_.registry());
    map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &prefs_, /*is_off_the_record=*/false, /*store_last_modified=*/false,
        /*restore_session=*/false, /*should_record_metrics=*/false);
    psst_settings_service_ = std::make_unique<PsstSettingsService>(
        *map_, &prefs_, /*brave_origin_service=*/nullptr);
  }

  void TearDown() override {
    psst_settings_service_.reset();
    map_->ShutdownOnUIThread();
  }

  HostContentSettingsMap* map() { return map_.get(); }
  PsstSettingsService* settings_service() {
    return psst_settings_service_.get();
  }

 private:
  base::test::ScopedFeatureList feature_list_{psst::features::kEnablePsst};
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> map_;
  std::unique_ptr<PsstSettingsService> psst_settings_service_;
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

class PsstSettingsServiceBraveOriginUnitTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(brave_origin::features::kBraveOrigin);

    local_state_.registry()->RegisterDictionaryPref(
        brave_origin::kBraveOriginPolicies);
    local_state_.registry()->RegisterBooleanPref(
        brave_origin::kOriginPurchaseValidated, false);
    local_state_.registry()->RegisterBooleanPref(
        brave_origin::kOriginPoliciesWereEnforced, false);
    local_state_.registry()->RegisterDictionaryPref(skus::prefs::kSkusState);
#if BUILDFLAG(IS_LINUX)
    local_state_.registry()->RegisterBooleanPref(
        brave_origin::kOriginFreeTierAccepted, false);
#endif

    HostContentSettingsMap::RegisterProfilePrefs(profile_prefs_.registry());
    psst::RegisterProfilePrefs(profile_prefs_.registry());
    map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &profile_prefs_, /*is_off_the_record=*/false,
        /*store_last_modified=*/false, /*restore_session=*/false,
        /*should_record_metrics=*/false);

    brave_origin::BraveOriginPolicyMap browser_policies;
    brave_origin::BraveOriginPolicyMap profile_policies;
    profile_policies.emplace(
        policy::key::kPsstEnabled,
        brave_origin::BraveOriginPolicyInfo(
            prefs::kPsstEnabled, /*default_value=*/false,
            /*user_settable=*/false, "psst_enabled.brave_origin_key"));

    auto* manager = brave_origin::BraveOriginPolicyManager::GetInstance();
    manager->Init(std::move(browser_policies), std::move(profile_policies),
                  &local_state_);
    manager->SetPurchased(true);
    local_state_.SetBoolean(brave_origin::kOriginPoliciesWereEnforced, true);

    brave_origin_service_ = std::make_unique<brave_origin::BraveOriginService>(
        &local_state_, &profile_prefs_, kTestProfileId,
        &mock_profile_policy_service_, &mock_browser_policy_service_,
        std::make_unique<NullBraveOriginDelegate>());

    psst_settings_service_ = std::make_unique<PsstSettingsService>(
        *map_, &profile_prefs_, brave_origin_service_.get());
  }

  void TearDown() override {
    psst_settings_service_.reset();
    brave_origin_service_.reset();
    map_->ShutdownOnUIThread();
    brave_origin::BraveOriginPolicyManager::GetInstance()->Shutdown();
  }

 protected:
  PsstSettingsService* settings_service() {
    return psst_settings_service_.get();
  }

  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  scoped_refptr<HostContentSettingsMap> map_;
  policy::MockPolicyService mock_profile_policy_service_;
  policy::MockPolicyService mock_browser_policy_service_;
  std::unique_ptr<brave_origin::BraveOriginService> brave_origin_service_;
  std::unique_ptr<PsstSettingsService> psst_settings_service_;
};

TEST_F(PsstSettingsServiceBraveOriginUnitTest,
       SetPsstEnabled_ControlledByBraveOrigin_UpdatesPolicyManagerValue) {
  policy::PolicyMap profile_policy_map;
  profile_policy_map.Set(policy::key::kPsstEnabled,
                         policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                         base::Value(true), nullptr);
  policy::PolicyMap empty_browser_policy_map;
  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(profile_policy_map));
  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_browser_policy_map));
  ASSERT_TRUE(brave_origin_service_->IsPolicyControlledByBraveOrigin(
      policy::key::kPsstEnabled));

  settings_service()->SetPsstEnabled(false);

  auto policy_value =
      brave_origin::BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
          policy::key::kPsstEnabled, kTestProfileId);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_FALSE(policy_value.value());
  EXPECT_FALSE(settings_service()->IsPsstEnabled());
}

TEST_F(PsstSettingsServiceBraveOriginUnitTest,
       IsPsstEnabled_ControlledByBraveOrigin_ReadsPolicyManagerNotRawPref) {
  policy::PolicyMap profile_policy_map;
  profile_policy_map.Set(policy::key::kPsstEnabled,
                         policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                         base::Value(true), nullptr);
  policy::PolicyMap empty_browser_policy_map;
  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(profile_policy_map));
  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_browser_policy_map));

  // Deliberately make the raw pref disagree with the BraveOrigin policy
  // manager value, to prove IsPsstEnabled() consults the policy manager
  // (via GetPolicyValue()) rather than just reading the pref.
  profile_prefs_.SetBoolean(prefs::kPsstEnabled, true);
  brave_origin::BraveOriginPolicyManager::GetInstance()->SetPolicyValue(
      policy::key::kPsstEnabled, false, kTestProfileId);

  EXPECT_FALSE(settings_service()->IsPsstEnabled());
}

TEST_F(PsstSettingsServiceBraveOriginUnitTest,
       SetPsstEnabled_NotControlledByBraveOrigin_WritesPrefDirectly) {
  policy::PolicyMap empty_profile_policy_map;
  policy::PolicyMap empty_browser_policy_map;
  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_profile_policy_map));
  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_browser_policy_map));
  ASSERT_FALSE(brave_origin_service_->IsPolicyControlledByBraveOrigin(
      policy::key::kPsstEnabled));

  settings_service()->SetPsstEnabled(true);

  EXPECT_TRUE(settings_service()->IsPsstEnabled());
  // BraveOrigin's policy manager must stay untouched -- the write went
  // straight to the pref since the policy isn't BraveOrigin-controlled.
  auto policy_value =
      brave_origin::BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
          policy::key::kPsstEnabled, kTestProfileId);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_FALSE(policy_value.value());  // still the registered default
}

TEST_F(PsstSettingsServiceBraveOriginUnitTest,
       SetPsstEnabled_ControlledAndNotUserSettable_ClearsPrefAtDefaultValue) {
  policy::PolicyMap profile_policy_map;
  profile_policy_map.Set(policy::key::kPsstEnabled,
                         policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                         base::Value(true), nullptr);
  policy::PolicyMap empty_browser_policy_map;
  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(profile_policy_map));
  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_browser_policy_map));

  settings_service()->SetPsstEnabled(false);

  EXPECT_TRUE(profile_prefs_.HasPrefPath(prefs::kPsstEnabled));
}

}  // namespace psst
