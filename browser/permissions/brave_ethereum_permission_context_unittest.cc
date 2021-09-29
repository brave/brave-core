/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace permissions {

class BraveEthereumPermissionContextUnitTest : public testing::Test {
 public:
  BraveEthereumPermissionContextUnitTest() = default;
  ~BraveEthereumPermissionContextUnitTest() override = default;

  content::BrowserContext* browser_context() { return &profile_; }
  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(&profile_);
  }

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  TestingProfile profile_;
};

TEST_F(BraveEthereumPermissionContextUnitTest, HasEthereumPermission) {
  GURL url("https://www.brave.com/");
  const std::string address = "0x407637cC04893DA7FA4A7C0B58884F82d69eD448";
  bool has_permission;
  bool success =
      permissions::BraveEthereumPermissionContext::HasEthereumPermission(
          browser_context(), url.spec(), address, &has_permission);
  EXPECT_TRUE(success);
  EXPECT_FALSE(has_permission);

  GURL origin_wallet_address;
  ASSERT_TRUE(
      brave_wallet::GetSubRequestOrigin(url, address, &origin_wallet_address));

  // Set the permission
  host_content_settings_map()->SetContentSettingDefaultScope(
      origin_wallet_address, url, ContentSettingsType::BRAVE_ETHEREUM,
      ContentSetting::CONTENT_SETTING_ALLOW);

  // Verify the permission is set
  success = permissions::BraveEthereumPermissionContext::HasEthereumPermission(
      browser_context(), url.spec(), address, &has_permission);
  EXPECT_TRUE(success);
  EXPECT_TRUE(has_permission);
}

TEST_F(BraveEthereumPermissionContextUnitTest, ResetEthereumPermission) {
  GURL url("https://www.brave.com/");
  const std::string address = "0x407637cC04893DA7FA4A7C0B58884F82d69eD448";

  GURL origin_wallet_address;
  ASSERT_TRUE(
      brave_wallet::GetSubRequestOrigin(url, address, &origin_wallet_address));

  // Set the permission
  host_content_settings_map()->SetContentSettingDefaultScope(
      origin_wallet_address, url, ContentSettingsType::BRAVE_ETHEREUM,
      ContentSetting::CONTENT_SETTING_ALLOW);

  // Verify the permission is set
  bool has_permission;
  bool success =
      permissions::BraveEthereumPermissionContext::HasEthereumPermission(
          browser_context(), url.spec(), address, &has_permission);
  EXPECT_TRUE(success);
  EXPECT_TRUE(has_permission);

  // Reset the permission
  ASSERT_TRUE(
      permissions::BraveEthereumPermissionContext::ResetEthereumPermission(
          browser_context(), url.spec(), address));

  // Verify the permission is reset
  success = permissions::BraveEthereumPermissionContext::HasEthereumPermission(
      browser_context(), url.spec(), address, &has_permission);
  EXPECT_TRUE(success);
  EXPECT_FALSE(has_permission);
}

}  // namespace permissions
