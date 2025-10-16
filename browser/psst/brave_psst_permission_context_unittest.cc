/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/core/brave_psst_permission_context.h"

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/psst/brave_psst_permission_context_factory.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/psst_permission_schema.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

base::Value::List VectorToList(const std::vector<std::string>& values) {
  base::Value::List list;
  for (auto& value : values) {
    list.Append(value);
  }
  return list;
}

std::string ParseConsentStatus(const psst::ConsentStatus consent_status) {
  switch (consent_status) {
    case psst::ConsentStatus::kAsk:
      return "kAsk";
    case psst::ConsentStatus::kAllow:
      return "kAllow";
    case psst::ConsentStatus::kBlock:
      return "kBlock";
    default:
      return "kNone";
  }
}

base::Value::Dict CreatePsstPermissionDict(
    psst::ConsentStatus consent_status,
    int script_version,
    const std::string& user_id,
    const std::vector<std::string>& urls_to_skip) {
  base::Value::Dict object;
  object.Set("user_id", user_id);
  object.Set("consent_status", ParseConsentStatus(consent_status));
  object.Set("script_version", script_version);
  object.Set("urls_to_skip", VectorToList(urls_to_skip));
  return object;
}

}  // namespace

namespace psst {
class BravePsstPermissionContextUnitTest : public testing::Test {
 public:
  void SetUp() override {
    psst_permission_context_ =
        BravePsstPermissionContextFactory::GetForProfile(&profile_);
    ASSERT_TRUE(psst_permission_context_);
  }

  void TearDown() override {
    psst_permission_context_ = nullptr;
    BravePsstPermissionContextFactory::GetInstance()->SetTestingFactory(
        &profile_, {});
  }

  TestingProfile* profile() { return &profile_; }

  BravePsstPermissionContext* psst_permission_context() {
    return psst_permission_context_;
  }

 private:
  base::test::ScopedFeatureList feature_list_{psst::features::kEnablePsst};
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  raw_ptr<BravePsstPermissionContext> psst_permission_context_;
};

TEST_F(BravePsstPermissionContextUnitTest,
       DontAllowToCreatePermissionForWrongSchema) {
  const auto first_permission_info = PsstPermissionInfo::FromValue(
      CreatePsstPermissionDict(ConsentStatus::kAllow, 1, "user123", {}));
  ASSERT_TRUE(first_permission_info);
  const url::Origin http_scheme_origin =
      url::Origin::Create(GURL("http://a.test"));
  ASSERT_TRUE(
      psst_permission_context()->GetGrantedObjects(http_scheme_origin).empty());
  psst_permission_context()->GrantPermission(http_scheme_origin,
                                             first_permission_info->Clone());
  ASSERT_TRUE(
      psst_permission_context()->GetGrantedObjects(http_scheme_origin).empty());

  const url::Origin file_scheme_origin =
      url::Origin::Create(GURL("file://a.test"));
  ASSERT_TRUE(
      psst_permission_context()->GetGrantedObjects(file_scheme_origin).empty());
  psst_permission_context()->GrantPermission(file_scheme_origin,
                                             first_permission_info->Clone());
  ASSERT_TRUE(
      psst_permission_context()->GetGrantedObjects(file_scheme_origin).empty());

  const url::Origin brave_scheme_origin =
      url::Origin::Create(GURL("brave://a.test"));
  ASSERT_TRUE(psst_permission_context()
                  ->GetGrantedObjects(brave_scheme_origin)
                  .empty());
  psst_permission_context()->GrantPermission(brave_scheme_origin,
                                             first_permission_info->Clone());
  ASSERT_TRUE(psst_permission_context()
                  ->GetGrantedObjects(brave_scheme_origin)
                  .empty());

  const url::Origin chrome_scheme_origin =
      url::Origin::Create(GURL("chrome://a.test"));
  ASSERT_TRUE(psst_permission_context()
                  ->GetGrantedObjects(chrome_scheme_origin)
                  .empty());
  psst_permission_context()->GrantPermission(chrome_scheme_origin,
                                             first_permission_info->Clone());
  ASSERT_TRUE(psst_permission_context()
                  ->GetGrantedObjects(chrome_scheme_origin)
                  .empty());
}

TEST_F(BravePsstPermissionContextUnitTest, CreateUpdateRevokePermissionInfo) {
  const url::Origin origin = url::Origin::Create(GURL("https://a.test"));
  const std::string first_user_id = "first-user123";
  const std::string second_user_id = "second-user123";

  ASSERT_FALSE(psst_permission_context()->HasPermission(origin, first_user_id));

  const auto first_permission_info =
      PsstPermissionInfo::FromValue(CreatePsstPermissionDict(
          ConsentStatus::kAllow, 1, first_user_id, std::vector<std::string>()));
  const auto second_permission_info = PsstPermissionInfo::FromValue(
      CreatePsstPermissionDict(ConsentStatus::kAllow, 1, second_user_id,
                               std::vector<std::string>()));
  ASSERT_TRUE(psst_permission_context()->GetGrantedObjects(origin).empty());
  psst_permission_context()->GrantPermission(origin,
                                             first_permission_info->Clone());
  ASSERT_EQ(psst_permission_context()->GetGrantedObjects(origin).size(), 1u);
  psst_permission_context()->GrantPermission(origin,
                                             second_permission_info->Clone());
  ASSERT_EQ(psst_permission_context()->GetGrantedObjects(origin).size(), 2u);

  auto first_permission_info_value =
      psst_permission_context()->GetPsstPermissionInfo(
          origin, first_permission_info->user_id);
  ASSERT_TRUE(first_permission_info_value.has_value());
  ASSERT_EQ(first_permission_info_value.value().consent_status,
            first_permission_info->consent_status);
  ASSERT_EQ(first_permission_info_value.value().script_version,
            first_permission_info->script_version);
  ASSERT_EQ(first_permission_info_value.value().user_id,
            first_permission_info->user_id);
  ASSERT_EQ(first_permission_info_value.value().urls_to_skip,
            first_permission_info->urls_to_skip);

  auto second_permission_info_value =
      psst_permission_context()->GetPsstPermissionInfo(
          origin, second_permission_info->user_id);
  ASSERT_TRUE(second_permission_info_value.has_value());
  ASSERT_EQ(second_permission_info_value.value().consent_status,
            second_permission_info->consent_status);
  ASSERT_EQ(second_permission_info_value.value().script_version,
            second_permission_info->script_version);
  ASSERT_EQ(second_permission_info_value.value().user_id,
            second_permission_info->user_id);
  ASSERT_EQ(second_permission_info_value.value().urls_to_skip,
            second_permission_info->urls_to_skip);

  const auto modified_permission_info =
      PsstPermissionInfo::FromValue(CreatePsstPermissionDict(
          ConsentStatus::kBlock, first_permission_info->script_version,
          first_permission_info->user_id, std::vector<std::string>()));
  psst_permission_context()->GrantPermission(origin,
                                             modified_permission_info->Clone());

  auto modified_permission_info_value =
      psst_permission_context()->GetPsstPermissionInfo(
          origin, modified_permission_info->user_id);
  ASSERT_TRUE(second_permission_info_value.has_value());
  ASSERT_EQ(modified_permission_info_value.value().consent_status,
            modified_permission_info->consent_status);
  ASSERT_EQ(modified_permission_info_value.value().script_version,
            modified_permission_info->script_version);
  ASSERT_EQ(modified_permission_info_value.value().user_id,
            modified_permission_info->user_id);
  ASSERT_EQ(modified_permission_info_value.value().urls_to_skip,
            modified_permission_info->urls_to_skip);

  psst_permission_context()->RevokePermission(origin,
                                              first_permission_info->user_id);
  ASSERT_EQ(psst_permission_context()->GetGrantedObjects(origin).size(), 1u);

  ASSERT_FALSE(psst_permission_context()->HasPermission(
      origin, first_permission_info->user_id));
  ASSERT_TRUE(psst_permission_context()->HasPermission(
      origin, second_permission_info->user_id));
}

}  // namespace psst
