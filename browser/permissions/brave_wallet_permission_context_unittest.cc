/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace permissions {

class BraveWalletPermissionContextUnitTest : public testing::Test {
 public:
  BraveWalletPermissionContextUnitTest() = default;
  ~BraveWalletPermissionContextUnitTest() override = default;

  content::BrowserContext* browser_context() { return &profile_; }
  bool Matches(const GURL& url1, const GURL& url2) {
    return (url1.scheme() == url2.scheme()) && (url1.host() == url2.host()) &&
           ((url1.has_port() && url2.has_port()) ? url1.port() == url2.port()
                                                 : true);
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

TEST_F(BraveWalletPermissionContextUnitTest, GetWebSitesWithPermission) {
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

    std::vector<std::string> web_sites =
        permissions::BraveWalletPermissionContext::GetWebSitesWithPermission(
            cases[i].type, browser_context());
    EXPECT_EQ(web_sites.size(), (uint32_t)1);

    url::Origin origin_wallet_address;
    EXPECT_TRUE(brave_wallet::GetSubRequestOrigin(
        permissions::ContentSettingsTypeToRequestType(cases[i].type), origin,
        cases[i].address, &origin_wallet_address));
    // origin_wallet_address looks like that
    // "https://www.brave.com__brg44hdsehzapvs8beqzvkq4egwevs3fre6ze2eno6s8/"
    // web_sites[0] looks like that
    // "https://www.brave.com__brg44hdsehzapvs8beqzvkq4egwevs3fre6ze2eno6s8:443"
    // That's why we are going to compare scheme, host and port if it's exist
    // in both URLs
    EXPECT_TRUE(Matches(origin_wallet_address.GetURL(), GURL(web_sites[0])));
  }
}

TEST_F(BraveWalletPermissionContextUnitTest, ResetWebSitePermission) {
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

    std::vector<std::string> web_sites =
        permissions::BraveWalletPermissionContext::GetWebSitesWithPermission(
            cases[i].type, browser_context());
    EXPECT_EQ(web_sites.size(), (uint32_t)1);

    // Not a valid URL test
    EXPECT_FALSE(
        permissions::BraveWalletPermissionContext::ResetWebSitePermission(
            cases[i].type, browser_context(), "not_valid"));

    EXPECT_TRUE(
        permissions::BraveWalletPermissionContext::ResetWebSitePermission(
            cases[i].type, browser_context(), web_sites[0]));

    web_sites =
        permissions::BraveWalletPermissionContext::GetWebSitesWithPermission(
            cases[i].type, browser_context());
    EXPECT_EQ(web_sites.size(), (uint32_t)0);
  }
}

}  // namespace permissions
