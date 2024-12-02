/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/permissions/brave_permission_manager.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/permissions/permission_manager_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/permissions/permission_util.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace permissions {

class BraveWalletPermissionContextUnitTest : public testing::Test {
 public:
  BraveWalletPermissionContextUnitTest() = default;
  ~BraveWalletPermissionContextUnitTest() override = default;

  void SetUp() override {
    map_ = HostContentSettingsMapFactory::GetForProfile(&profile_);
    profile_.SetPermissionControllerDelegate(
        base::WrapUnique(static_cast<BravePermissionManager*>(
            PermissionManagerFactory::GetInstance()
                ->BuildServiceInstanceForBrowserContext(browser_context())
                .release())));
  }

  void TearDown() override {
    profile_.SetPermissionControllerDelegate(nullptr);
  }

  HostContentSettingsMap* map() { return map_.get(); }

  content::BrowserContext* browser_context() { return &profile_; }
  bool Matches(const GURL& url1, const GURL& url2) {
    return (url1.scheme() == url2.scheme()) && (url1.host() == url2.host()) &&
           ((url1.has_port() && url2.has_port()) ? url1.port() == url2.port()
                                                 : true);
  }

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  TestingProfile profile_;
  scoped_refptr<HostContentSettingsMap> map_;
};

TEST_F(BraveWalletPermissionContextUnitTest, AddPermission) {
  url::Origin origin = url::Origin::Create(GURL("https://www.brave.com/"));
  const struct {
    const char* address;
    blink::PermissionType type;
  } cases[] = {{"0x407637cC04893DA7FA4A7C0B58884F82d69eD448",
                blink::PermissionType::BRAVE_ETHEREUM},
               {"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                blink::PermissionType::BRAVE_SOLANA}};
  for (auto entry : cases) {
    SCOPED_TRACE(entry.address);
    bool has_permission;
    bool success = permissions::BraveWalletPermissionContext::HasPermission(
        entry.type, browser_context(), origin, entry.address, &has_permission);
    EXPECT_TRUE(success);
    EXPECT_FALSE(has_permission);

    success = permissions::BraveWalletPermissionContext::AddPermission(
        entry.type, browser_context(), origin, entry.address);
    EXPECT_TRUE(success);

    // Verify the permission is set
    success = permissions::BraveWalletPermissionContext::HasPermission(
        entry.type, browser_context(), origin, entry.address, &has_permission);
    EXPECT_TRUE(success);
    EXPECT_TRUE(has_permission);

    // Set blocked content setting for the url.
    map()->SetContentSettingDefaultScope(
        origin.GetURL(), origin.GetURL(),
        PermissionUtil::PermissionTypeToContentSettingsTypeSafe(entry.type),
        CONTENT_SETTING_BLOCK);
    success = permissions::BraveWalletPermissionContext::HasPermission(
        entry.type, browser_context(), origin, entry.address, &has_permission);
    EXPECT_TRUE(success);
    EXPECT_FALSE(has_permission);

    // Set content setting to default
    map()->SetContentSettingDefaultScope(
        origin.GetURL(), origin.GetURL(),
        PermissionUtil::PermissionTypeToContentSettingsTypeSafe(entry.type),
        CONTENT_SETTING_DEFAULT);
    success = permissions::BraveWalletPermissionContext::HasPermission(
        entry.type, browser_context(), origin, entry.address, &has_permission);
    EXPECT_TRUE(success);
    EXPECT_TRUE(has_permission);
  }
}

TEST_F(BraveWalletPermissionContextUnitTest, ResetPermission) {
  url::Origin origin = url::Origin::Create(GURL("https://www.brave.com/"));
  const struct {
    const char* address;
    blink::PermissionType type;
  } cases[] = {{"0x407637cC04893DA7FA4A7C0B58884F82d69eD448",
                blink::PermissionType::BRAVE_ETHEREUM},
               {"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                blink::PermissionType::BRAVE_SOLANA}};
  for (auto entry : cases) {
    SCOPED_TRACE(entry.address);
    bool success = permissions::BraveWalletPermissionContext::AddPermission(
        entry.type, browser_context(), origin, entry.address);
    EXPECT_TRUE(success);

    // Adding twice is OK
    success = permissions::BraveWalletPermissionContext::AddPermission(
        entry.type, browser_context(), origin, entry.address);
    EXPECT_TRUE(success);

    // Verify the permission is set
    bool has_permission;
    success = permissions::BraveWalletPermissionContext::HasPermission(
        entry.type, browser_context(), origin, entry.address, &has_permission);
    EXPECT_TRUE(success);
    EXPECT_TRUE(has_permission);

    // CONTENT_SETTING_BLOCK shouldn't affect reset.
    map()->SetContentSettingDefaultScope(
        origin.GetURL(), origin.GetURL(),
        PermissionUtil::PermissionTypeToContentSettingsTypeSafe(entry.type),
        CONTENT_SETTING_BLOCK);
    // Reset the permission
    ASSERT_TRUE(permissions::BraveWalletPermissionContext::ResetPermission(
        entry.type, browser_context(), origin, entry.address));
    map()->SetContentSettingDefaultScope(
        origin.GetURL(), origin.GetURL(),
        PermissionUtil::PermissionTypeToContentSettingsTypeSafe(entry.type),
        CONTENT_SETTING_DEFAULT);

    // Verify the permission is reset
    success = permissions::BraveWalletPermissionContext::HasPermission(
        entry.type, browser_context(), origin, entry.address, &has_permission);
    EXPECT_TRUE(success);
    EXPECT_FALSE(has_permission);
  }
}

TEST_F(BraveWalletPermissionContextUnitTest, ResetAllPermissions) {
  url::Origin origin = url::Origin::Create(GURL("https://www.brave.com/"));
  const struct {
    const char* address;
    blink::PermissionType type;
  } cases[] = {{"0x407637cC04893DA7FA4A7C0B58884F82d69eD448",
                blink::PermissionType::BRAVE_ETHEREUM},
               {"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                blink::PermissionType::BRAVE_SOLANA}};
  for (auto entry : cases) {
    SCOPED_TRACE(entry.address);
    bool success = permissions::BraveWalletPermissionContext::AddPermission(
        entry.type, browser_context(), origin, entry.address);
    EXPECT_TRUE(success);

    // Verify the permission is set
    bool has_permission;
    success = permissions::BraveWalletPermissionContext::HasPermission(
        entry.type, browser_context(), origin, entry.address, &has_permission);
    EXPECT_TRUE(success);
    EXPECT_TRUE(has_permission);
  }

  // Reset all permissions
  permissions::BraveWalletPermissionContext::ResetAllPermissions(
      browser_context());

  // Verify permissions are reset
  for (auto entry : cases) {
    SCOPED_TRACE(entry.address);
    bool has_permission;
    bool success = permissions::BraveWalletPermissionContext::HasPermission(
        entry.type, browser_context(), origin, entry.address, &has_permission);
    EXPECT_TRUE(success);
    EXPECT_FALSE(has_permission);
  }
}

TEST_F(BraveWalletPermissionContextUnitTest, GetWebSitesWithPermission) {
  url::Origin origin = url::Origin::Create(GURL("https://www.brave.com/"));
  const struct {
    const char* address;
    ContentSettingsType type;
    blink::PermissionType permission;
  } cases[] = {
      {"0x407637cC04893DA7FA4A7C0B58884F82d69eD448",
       ContentSettingsType::BRAVE_ETHEREUM,
       blink::PermissionType::BRAVE_ETHEREUM},
      {"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
       ContentSettingsType::BRAVE_SOLANA, blink::PermissionType::BRAVE_SOLANA}};
  for (auto entry : cases) {
    SCOPED_TRACE(entry.address);
    bool success = permissions::BraveWalletPermissionContext::AddPermission(
        entry.permission, browser_context(), origin, entry.address);
    EXPECT_TRUE(success);

    std::vector<std::string> web_sites =
        permissions::BraveWalletPermissionContext::GetWebSitesWithPermission(
            entry.permission, browser_context());
    EXPECT_EQ(web_sites.size(), (uint32_t)1);

    url::Origin origin_wallet_address;
    EXPECT_TRUE(brave_wallet::GetSubRequestOrigin(
        permissions::ContentSettingsTypeToRequestType(entry.type), origin,
        entry.address, &origin_wallet_address));
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
    blink::PermissionType permission;
  } cases[] = {
      {"0x407637cC04893DA7FA4A7C0B58884F82d69eD448",
       ContentSettingsType::BRAVE_ETHEREUM,
       blink::PermissionType::BRAVE_ETHEREUM},
      {"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
       ContentSettingsType::BRAVE_SOLANA, blink::PermissionType::BRAVE_SOLANA}};
  for (auto entry : cases) {
    SCOPED_TRACE(entry.address);
    bool success = permissions::BraveWalletPermissionContext::AddPermission(
        entry.permission, browser_context(), origin, entry.address);
    EXPECT_TRUE(success);

    std::vector<std::string> web_sites =
        permissions::BraveWalletPermissionContext::GetWebSitesWithPermission(
            entry.permission, browser_context());
    EXPECT_EQ(web_sites.size(), (uint32_t)1);

    // Not a valid URL test
    EXPECT_FALSE(
        permissions::BraveWalletPermissionContext::ResetWebSitePermission(
            entry.permission, browser_context(), "not_valid"));

    EXPECT_TRUE(
        permissions::BraveWalletPermissionContext::ResetWebSitePermission(
            entry.permission, browser_context(), web_sites[0]));

    web_sites =
        permissions::BraveWalletPermissionContext::GetWebSitesWithPermission(
            entry.permission, browser_context());
    EXPECT_EQ(web_sites.size(), (uint32_t)0);
  }
}

}  // namespace permissions
