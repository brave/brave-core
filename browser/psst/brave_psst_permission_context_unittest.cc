/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/psst/brave_psst_permission_context.h"

#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {
class BravePsstPermissionContextUnitTest : public testing::Test {
 public:
  void SetUp() override {
    auto* map = HostContentSettingsMapFactory::GetForProfile(&profile_);
    ASSERT_TRUE(map);
    psst_permission_context_ =
        std::make_unique<BravePsstPermissionContext>(map);
  }

  TestingProfile* profile() { return &profile_; }

  BravePsstPermissionContext* psst_permission_context() {
    return psst_permission_context_.get();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  std::unique_ptr<BravePsstPermissionContext> psst_permission_context_;
};

TEST_F(BravePsstPermissionContextUnitTest,
       DontAllowToCreatePermissionForWrongSchema) {
  const PsstPermissionInfo first_permission_info{
      psst::ConsentStatus::kAllow, 1, "user123", base::Value::List()};

  const url::Origin http_scheme_origin =
      url::Origin::Create(GURL("http://a.test"));
  ASSERT_TRUE(
      psst_permission_context()->GetGrantedObjects(http_scheme_origin).empty());
  psst_permission_context()->CreateOrUpdate(http_scheme_origin,
                                            first_permission_info);
  ASSERT_TRUE(
      psst_permission_context()->GetGrantedObjects(http_scheme_origin).empty());

  const url::Origin file_scheme_origin =
      url::Origin::Create(GURL("file://a.test"));
  ASSERT_TRUE(
      psst_permission_context()->GetGrantedObjects(file_scheme_origin).empty());
  psst_permission_context()->CreateOrUpdate(file_scheme_origin,
                                            first_permission_info);
  ASSERT_TRUE(
      psst_permission_context()->GetGrantedObjects(file_scheme_origin).empty());

  const url::Origin brave_scheme_origin =
      url::Origin::Create(GURL("brave://a.test"));
  ASSERT_TRUE(psst_permission_context()
                  ->GetGrantedObjects(brave_scheme_origin)
                  .empty());
  psst_permission_context()->CreateOrUpdate(brave_scheme_origin,
                                            first_permission_info);
  ASSERT_TRUE(psst_permission_context()
                  ->GetGrantedObjects(brave_scheme_origin)
                  .empty());

  const url::Origin chrome_scheme_origin =
      url::Origin::Create(GURL("chrome://a.test"));
  ASSERT_TRUE(psst_permission_context()
                  ->GetGrantedObjects(chrome_scheme_origin)
                  .empty());
  psst_permission_context()->CreateOrUpdate(chrome_scheme_origin,
                                            first_permission_info);
  ASSERT_TRUE(psst_permission_context()
                  ->GetGrantedObjects(chrome_scheme_origin)
                  .empty());
}

TEST_F(BravePsstPermissionContextUnitTest, CreateUpdateRevokePermissionInfo) {
  const url::Origin origin = url::Origin::Create(GURL("https://a.test"));
  const std::string first_user_id = "first-user123";
  const std::string second_user_id = "second-user123";

  ASSERT_FALSE(
      psst_permission_context()->GetPsstPermissionInfo(origin, first_user_id));

  const PsstPermissionInfo first_permission_info{
      psst::ConsentStatus::kAllow, 1, first_user_id, base::Value::List()};
  const PsstPermissionInfo second_permission_info{
      psst::ConsentStatus::kAllow, 1, second_user_id, base::Value::List()};
  ASSERT_TRUE(psst_permission_context()->GetGrantedObjects(origin).empty());
  psst_permission_context()->CreateOrUpdate(origin, first_permission_info);
  ASSERT_EQ(psst_permission_context()->GetGrantedObjects(origin).size(), 1u);
  psst_permission_context()->CreateOrUpdate(origin, second_permission_info);
  ASSERT_EQ(psst_permission_context()->GetGrantedObjects(origin).size(), 2u);

  auto first_permission_info_value =
      psst_permission_context()->GetPsstPermissionInfo(
          origin, first_permission_info.user_id);
  ASSERT_EQ(first_permission_info_value->consent_status,
            first_permission_info.consent_status);
  ASSERT_EQ(first_permission_info_value->script_version,
            first_permission_info.script_version);
  ASSERT_EQ(first_permission_info_value->user_id,
            first_permission_info.user_id);
  ASSERT_EQ(first_permission_info_value->urls_to_skip,
            first_permission_info.urls_to_skip);

  auto second_permission_info_value =
      psst_permission_context()->GetPsstPermissionInfo(
          origin, second_permission_info.user_id);
  ASSERT_EQ(second_permission_info_value->consent_status,
            second_permission_info.consent_status);
  ASSERT_EQ(second_permission_info_value->script_version,
            second_permission_info.script_version);
  ASSERT_EQ(second_permission_info_value->user_id,
            second_permission_info.user_id);
  ASSERT_EQ(second_permission_info_value->urls_to_skip,
            second_permission_info.urls_to_skip);

  const PsstPermissionInfo modified_permission_info{
      psst::ConsentStatus::kBlock, first_permission_info.script_version,
      first_permission_info.user_id, base::Value::List()};
  psst_permission_context()->CreateOrUpdate(origin, modified_permission_info);
  auto modified_permission_info_value =
      psst_permission_context()->GetPsstPermissionInfo(
          origin, modified_permission_info.user_id);
  ASSERT_EQ(modified_permission_info_value->consent_status,
            modified_permission_info.consent_status);
  ASSERT_EQ(modified_permission_info_value->script_version,
            modified_permission_info.script_version);
  ASSERT_EQ(modified_permission_info_value->user_id,
            modified_permission_info.user_id);
  ASSERT_EQ(modified_permission_info_value->urls_to_skip,
            modified_permission_info.urls_to_skip);

  psst_permission_context()->Revoke(origin, first_permission_info.user_id);
  ASSERT_EQ(psst_permission_context()->GetGrantedObjects(origin).size(), 1u);

  ASSERT_FALSE(psst_permission_context()->GetPsstPermissionInfo(
      origin, first_permission_info.user_id));
  ASSERT_TRUE(psst_permission_context()->GetPsstPermissionInfo(
      origin, second_permission_info.user_id));
}

}  // namespace psst
