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

TEST_F(BraveEthereumPermissionContextUnitTest, AddEthereumPermission) {
  url::Origin origin = url::Origin::Create(GURL("https://www.brave.com/"));
  const std::string address = "0x407637cC04893DA7FA4A7C0B58884F82d69eD448";
  bool has_permission;
  bool success =
      permissions::BraveEthereumPermissionContext::HasEthereumPermission(
          browser_context(), origin, address, &has_permission);
  EXPECT_TRUE(success);
  EXPECT_FALSE(has_permission);

  success = permissions::BraveEthereumPermissionContext::AddEthereumPermission(
      browser_context(), origin, address);
  EXPECT_TRUE(success);

  // Verify the permission is set
  success = permissions::BraveEthereumPermissionContext::HasEthereumPermission(
      browser_context(), origin, address, &has_permission);
  EXPECT_TRUE(success);
  EXPECT_TRUE(has_permission);
}

TEST_F(BraveEthereumPermissionContextUnitTest, ResetEthereumPermission) {
  url::Origin origin = url::Origin::Create(GURL("https://www.brave.com/"));
  const std::string address = "0x407637cC04893DA7FA4A7C0B58884F82d69eD448";

  bool success =
      permissions::BraveEthereumPermissionContext::AddEthereumPermission(
          browser_context(), origin, address);
  EXPECT_TRUE(success);

  // Adding twice is OK
  success = permissions::BraveEthereumPermissionContext::AddEthereumPermission(
      browser_context(), origin, address);
  EXPECT_TRUE(success);

  // Verify the permission is set
  bool has_permission;
  success = permissions::BraveEthereumPermissionContext::HasEthereumPermission(
      browser_context(), origin, address, &has_permission);
  EXPECT_TRUE(success);
  EXPECT_TRUE(has_permission);

  // Reset the permission
  ASSERT_TRUE(
      permissions::BraveEthereumPermissionContext::ResetEthereumPermission(
          browser_context(), origin, address));

  // Verify the permission is reset
  success = permissions::BraveEthereumPermissionContext::HasEthereumPermission(
      browser_context(), origin, address, &has_permission);
  EXPECT_TRUE(success);
  EXPECT_FALSE(has_permission);
}

}  // namespace permissions
