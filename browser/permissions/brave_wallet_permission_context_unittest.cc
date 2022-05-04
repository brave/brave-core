/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace permissions {

class BraveWalletPermissionContextUnitTest : public testing::Test {
 public:
  BraveWalletPermissionContextUnitTest() = default;
  ~BraveWalletPermissionContextUnitTest() override = default;

  content::BrowserContext* browser_context() { return &profile_; }
  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(&profile_);
  }

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  TestingProfile profile_;
};

TEST_F(BraveWalletPermissionContextUnitTest, AddPermission) {
  url::Origin origin = url::Origin::Create(GURL("https://www.brave.com/"));
  const struct {
    const char* address;
    ContentSettingsType type;
  } cases[] = {{"0x407637cC04893DA7FA4A7C0B58884F82d69eD448",
                ContentSettingsType::BRAVE_ETHEREUM},
               {"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                ContentSettingsType::BRAVE_SOLANA}};
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    bool has_permission;
    bool success = permissions::BraveWalletPermissionContext::HasPermission(
        cases[i].type, browser_context(), origin, cases[i].address,
        &has_permission);
    EXPECT_TRUE(success) << "case: " << i;
    EXPECT_FALSE(has_permission) << "case: " << i;

    success = permissions::BraveWalletPermissionContext::AddPermission(
        cases[i].type, browser_context(), origin, cases[i].address);
    EXPECT_TRUE(success) << "case: " << i;

    // Verify the permission is set
    success = permissions::BraveWalletPermissionContext::HasPermission(
        cases[i].type, browser_context(), origin, cases[i].address,
        &has_permission);
    EXPECT_TRUE(success) << "case: " << i;
    EXPECT_TRUE(has_permission) << "case: " << i;
  }
}

TEST_F(BraveWalletPermissionContextUnitTest, ResetPermission) {
  url::Origin origin = url::Origin::Create(GURL("https://www.brave.com/"));
  const struct {
    const char* address;
    ContentSettingsType type;
  } cases[] = {{"0x407637cC04893DA7FA4A7C0B58884F82d69eD448",
                ContentSettingsType::BRAVE_ETHEREUM},
               {"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                ContentSettingsType::BRAVE_SOLANA}};
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    bool success = permissions::BraveWalletPermissionContext::AddPermission(
        cases[i].type, browser_context(), origin, cases[i].address);
    EXPECT_TRUE(success) << "case: " << i;

    // Adding twice is OK
    success = permissions::BraveWalletPermissionContext::AddPermission(
        cases[i].type, browser_context(), origin, cases[i].address);
    EXPECT_TRUE(success) << "case: " << i;

    // Verify the permission is set
    bool has_permission;
    success = permissions::BraveWalletPermissionContext::HasPermission(
        cases[i].type, browser_context(), origin, cases[i].address,
        &has_permission);
    EXPECT_TRUE(success) << "case: " << i;
    EXPECT_TRUE(has_permission) << "case: " << i;

    // Reset the permission
    ASSERT_TRUE(permissions::BraveWalletPermissionContext::ResetPermission(
        cases[i].type, browser_context(), origin, cases[i].address));

    // Verify the permission is reset
    success = permissions::BraveWalletPermissionContext::HasPermission(
        cases[i].type, browser_context(), origin, cases[i].address,
        &has_permission);
    EXPECT_TRUE(success) << "case: " << i;
    EXPECT_FALSE(has_permission) << "case: " << i;
  }
}

}  // namespace permissions
