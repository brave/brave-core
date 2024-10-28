/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/permissions/brave_permission_manager.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/permissions/permission_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/permissions/permission_context_base.h"
#include "components/permissions/permission_manager.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/permissions_client.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"
#include "url/origin.h"

using testing::ElementsAreArray;

namespace permissions {

namespace {

class PermissionRequestManagerObserver
    : public PermissionRequestManager::Observer {
 public:
  explicit PermissionRequestManagerObserver(PermissionRequestManager* manager)
      : manager_(manager) {
    manager_->AddObserver(this);
  }

  ~PermissionRequestManagerObserver() override {
    manager_->RemoveObserver(this);
  }

  // PermissionRequestManager::Observer:
  void OnPromptAdded() override { is_showing_bubble_ = true; }
  void OnPromptRemoved() override { is_showing_bubble_ = false; }
  void OnRequestsFinalized() override { is_requests_finalized_ = true; }

  bool IsShowingBubble() { return is_showing_bubble_; }
  bool IsRequestsFinalized() { return is_requests_finalized_; }
  void Reset() { is_showing_bubble_ = is_requests_finalized_ = false; }

 private:
  raw_ptr<PermissionRequestManager> manager_ = nullptr;
  bool is_showing_bubble_ = false;
  bool is_requests_finalized_ = false;
};

}  // namespace

class PermissionManagerBrowserTest : public InProcessBrowserTest {
 public:
  PermissionManagerBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitAndEnableFeature(
        brave_wallet::features::kNativeBraveWalletFeature);
  }

  ~PermissionManagerBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server()->ServeFilesFromSourceDirectory(GetChromeTestDataDir());
    ASSERT_TRUE(https_server()->Start());
    SetPermissionManagerForProfile(browser()->profile());
  }

  void SetPermissionManagerForProfile(Profile* profile) {
    permission_manager_ = static_cast<permissions::BravePermissionManager*>(
        PermissionManagerFactory::GetForProfile(profile));
  }

  PermissionRequestManager* GetPermissionRequestManager() {
    return PermissionRequestManager::FromWebContents(
        browser()->tab_strip_model()->GetActiveWebContents());
  }

  HostContentSettingsMap* host_content_settings_map(Profile* profile) {
    return HostContentSettingsMapFactory::GetForProfile(profile);
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }
  url::Origin GetLastCommitedOrigin() {
    return url::Origin::Create(web_contents()->GetLastCommittedURL());
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }
  BravePermissionManager* permission_manager() { return permission_manager_; }

  bool IsPendingGroupedRequestsEmpty(ContentSettingsType type) {
    PermissionContextBase* context =
        permission_manager()->GetPermissionContextForTesting(type);
    return context->IsPendingGroupedRequestsEmptyForTesting();
  }

  void TestRequestPermissionsDoNotLeak(Profile* profile1, Profile* profile2) {
    SetPermissionManagerForProfile(profile1);
    auto* permission_request_manager = GetPermissionRequestManager();
    const std::string address = "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A";
    ContentSettingsType type = ContentSettingsType::BRAVE_ETHEREUM;
    blink::PermissionType permission = blink::PermissionType::BRAVE_ETHEREUM;

    RequestType request_type = ContentSettingsTypeToRequestType(type);
    url::Origin sub_request_origin;
    ASSERT_TRUE(brave_wallet::GetSubRequestOrigin(
        request_type, GetLastCommitedOrigin(), address, &sub_request_origin));

    url::Origin origin;
    ASSERT_TRUE(brave_wallet::GetConcatOriginFromWalletAddresses(
        GetLastCommitedOrigin(), {address}, &origin));

    auto observer = std::make_unique<PermissionRequestManagerObserver>(
        permission_request_manager);

    base::MockCallback<base::OnceCallback<void(
        const std::vector<blink::mojom::PermissionStatus>&)>>
        callback;

    permission_manager()->RequestPermissionsForOrigin(
        {permission}, web_contents()->GetPrimaryMainFrame(), origin.GetURL(),
        true, callback.Get());

    content::RunAllTasksUntilIdle();
    permissions::BraveWalletPermissionContext::AcceptOrCancel(
        {address}, brave_wallet::mojom::PermissionLifetimeOption::kForever,
        web_contents());

    EXPECT_TRUE(observer->IsRequestsFinalized());
    EXPECT_TRUE(!observer->IsShowingBubble());
    EXPECT_TRUE(IsPendingGroupedRequestsEmpty(type));

    // Verify the permission has changed for profile1
    EXPECT_EQ(host_content_settings_map(profile1)->GetContentSetting(
                  sub_request_origin.GetURL(), GetLastCommitedOrigin().GetURL(),
                  type),
              ContentSetting::CONTENT_SETTING_ALLOW);

    // Verify the permission hasn't leaked to profile2
    EXPECT_EQ(host_content_settings_map(profile2)->GetContentSetting(
                  sub_request_origin.GetURL(), GetLastCommitedOrigin().GetURL(),
                  type),
              ContentSetting::CONTENT_SETTING_ASK);
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;
  raw_ptr<BravePermissionManager, DanglingUntriaged> permission_manager_ =
      nullptr;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(PermissionManagerBrowserTest, RequestPermissions) {
  const GURL& url = https_server()->GetURL("a.test", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  auto* permission_request_manager = GetPermissionRequestManager();
  EXPECT_FALSE(permission_request_manager->IsRequestInProgress());
  struct {
    std::vector<std::string> addresses;
    ContentSettingsType type;
    blink::PermissionType permission;
  } cases[] = {{{"0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
                 "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"},
                ContentSettingsType::BRAVE_ETHEREUM,
                blink::PermissionType::BRAVE_ETHEREUM},
               {{"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                 "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV"},
                ContentSettingsType::BRAVE_SOLANA,
                blink::PermissionType::BRAVE_SOLANA}};
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    const std::vector<std::string>& addresses = cases[i].addresses;
    RequestType request_type = ContentSettingsTypeToRequestType(cases[i].type);
    EXPECT_TRUE(IsPendingGroupedRequestsEmpty(cases[i].type)) << "case: " << i;

    std::vector<blink::PermissionType> permissions(addresses.size(),
                                                   cases[i].permission);
    std::vector<url::Origin> sub_request_origins(addresses.size());
    for (size_t j = 0; j < addresses.size(); ++j) {
      ASSERT_TRUE(brave_wallet::GetSubRequestOrigin(
          request_type, GetLastCommitedOrigin(), addresses[j],
          &sub_request_origins[j]))
          << "case: " << i << ", address: " << j;
    }

    url::Origin origin;
    ASSERT_TRUE(brave_wallet::GetConcatOriginFromWalletAddresses(
        GetLastCommitedOrigin(), addresses, &origin));

    auto observer = std::make_unique<PermissionRequestManagerObserver>(
        permission_request_manager);

    base::MockCallback<base::OnceCallback<void(
        const std::vector<blink::mojom::PermissionStatus>&)>>
        callback;
    EXPECT_CALL(callback,
                Run(ElementsAreArray({blink::mojom::PermissionStatus::ASK,
                                      blink::mojom::PermissionStatus::ASK})))
        .Times(1);
    permission_manager()->RequestPermissionsForOrigin(
        permissions, web_contents()->GetPrimaryMainFrame(), origin.GetURL(),
        true, callback.Get());

    content::RunAllTasksUntilIdle();

    EXPECT_TRUE(permission_request_manager->IsRequestInProgress())
        << "case: " << i;
    EXPECT_TRUE(observer->IsShowingBubble()) << "case: " << i;
    // update anchor should not dismiss the bubble
    permission_request_manager->UpdateAnchor();
    EXPECT_TRUE(observer->IsShowingBubble()) << "case: " << i;
    EXPECT_FALSE(IsPendingGroupedRequestsEmpty(cases[i].type)) << "case: " << i;

    // Check sub-requests are created as expected.
    EXPECT_EQ(permission_request_manager->Requests().size(), addresses.size());
    for (size_t j = 0; j < permission_request_manager->Requests().size(); ++j) {
      EXPECT_EQ(permission_request_manager->Requests()[j]->request_type(),
                request_type)
          << "case: " << i << ", address: " << j;
      EXPECT_EQ(sub_request_origins[j].GetURL(),
                permission_request_manager->Requests()[j]->requesting_origin())
          << "case: " << i << ", address: " << j;
    }

    // Test dismissing request.
    permissions::BraveWalletPermissionContext::Cancel(web_contents());
    testing::Mock::VerifyAndClearExpectations(&callback);
    EXPECT_TRUE(observer->IsRequestsFinalized()) << "case: " << i;
    EXPECT_TRUE(!observer->IsShowingBubble()) << "case: " << i;
    EXPECT_TRUE(IsPendingGroupedRequestsEmpty(cases[i].type)) << "case: " << i;

    for (size_t j = 0; j < addresses.size(); ++j) {
      EXPECT_EQ(host_content_settings_map(browser()->profile())
                    ->GetContentSetting(sub_request_origins[j].GetURL(),
                                        GetLastCommitedOrigin().GetURL(),
                                        cases[i].type),
                ContentSetting::CONTENT_SETTING_ASK)
          << "case: " << i << ", address: " << j;
    }

    observer->Reset();
    EXPECT_CALL(
        callback,
        Run(ElementsAreArray({blink::mojom::PermissionStatus::ASK,
                              blink::mojom::PermissionStatus::GRANTED})))
        .Times(1);
    permission_manager()->RequestPermissionsForOrigin(
        permissions, web_contents()->GetPrimaryMainFrame(), origin.GetURL(),
        true, callback.Get());

    content::RunAllTasksUntilIdle();
    EXPECT_TRUE(permission_request_manager->IsRequestInProgress())
        << "case: " << i;
    EXPECT_TRUE(observer->IsShowingBubble()) << "case: " << i;
    // update anchor should not dismiss the bubble
    permission_request_manager->UpdateAnchor();
    EXPECT_TRUE(observer->IsShowingBubble()) << "case: " << i;
    EXPECT_FALSE(IsPendingGroupedRequestsEmpty(cases[i].type)) << "case: " << i;

    // Check sub-requests are created as expected.
    EXPECT_EQ(permission_request_manager->Requests().size(), addresses.size());
    for (size_t j = 0; j < permission_request_manager->Requests().size(); ++j) {
      EXPECT_EQ(permission_request_manager->Requests()[j]->request_type(),
                request_type)
          << "case: " << i << ", address: " << j;
      EXPECT_EQ(sub_request_origins[j].GetURL(),
                permission_request_manager->Requests()[j]->requesting_origin())
          << "case: " << i << ", address: " << j;
    }

    // Test accepting request with one of the address.
    permissions::BraveWalletPermissionContext::AcceptOrCancel(
        std::vector<std::string>{addresses[1]},
        brave_wallet::mojom::PermissionLifetimeOption::kForever,
        web_contents());
    testing::Mock::VerifyAndClearExpectations(&callback);
    std::vector<ContentSetting> expected_settings(
        {ContentSetting::CONTENT_SETTING_ASK,
         ContentSetting::CONTENT_SETTING_ALLOW});
    EXPECT_TRUE(observer->IsRequestsFinalized()) << "case: " << i;
    EXPECT_TRUE(!observer->IsShowingBubble()) << "case: " << i;
    EXPECT_TRUE(IsPendingGroupedRequestsEmpty(cases[i].type)) << "case: " << i;

    for (size_t j = 0; j < addresses.size(); ++j) {
      EXPECT_EQ(host_content_settings_map(browser()->profile())
                    ->GetContentSetting(sub_request_origins[j].GetURL(),
                                        GetLastCommitedOrigin().GetURL(),
                                        cases[i].type),
                expected_settings[j])
          << "case: " << i << ", address: " << j;
    }
  }
}

IN_PROC_BROWSER_TEST_F(PermissionManagerBrowserTest,
                       IncognitoPermissionsDoNotLeak) {
  const GURL& url = https_server()->GetURL("a.test", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  auto* profile = browser()->profile();
  auto* incognito_profile =
      CreateIncognitoBrowser(browser()->profile())->profile();

  // Verify permissions do not leak from incongito profile into normal profile.
  TestRequestPermissionsDoNotLeak(incognito_profile, profile);
}

IN_PROC_BROWSER_TEST_F(PermissionManagerBrowserTest, PermissionsDoNotLeak) {
  const GURL& url = https_server()->GetURL("a.test", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  auto* profile = browser()->profile();
  auto* incognito_profile =
      CreateIncognitoBrowser(browser()->profile())->profile();

  // Verify permissions do not leak from normal profile into incognito profile.
  TestRequestPermissionsDoNotLeak(profile, incognito_profile);
}

IN_PROC_BROWSER_TEST_F(PermissionManagerBrowserTest,
                       RequestPermissionsTabClosed) {
  const GURL& url = https_server()->GetURL("a.test", "/empty.html");

  struct {
    std::vector<std::string> addresses;
    ContentSettingsType type;
    blink::PermissionType permission;
  } cases[] = {{{"0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8C",
                 "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8D"},
                ContentSettingsType::BRAVE_ETHEREUM,
                blink::PermissionType::BRAVE_ETHEREUM},
               {{"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                 "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV"},
                ContentSettingsType::BRAVE_SOLANA,
                blink::PermissionType::BRAVE_SOLANA}};
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    ASSERT_TRUE(AddTabAtIndexToBrowser(browser(), 0, url,
                                       ui::PAGE_TRANSITION_TYPED, true));
    auto* permission_request_manager = GetPermissionRequestManager();
    EXPECT_FALSE(permission_request_manager->IsRequestInProgress());
    const std::vector<std::string>& addresses = cases[i].addresses;
    RequestType request_type = ContentSettingsTypeToRequestType(cases[i].type);
    EXPECT_TRUE(IsPendingGroupedRequestsEmpty(cases[i].type)) << "case: " << i;

    std::vector<blink::PermissionType> permissions(addresses.size(),
                                                   cases[i].permission);
    std::vector<url::Origin> sub_request_origins(addresses.size());
    for (size_t j = 0; j < addresses.size(); ++j) {
      ASSERT_TRUE(brave_wallet::GetSubRequestOrigin(
          request_type, GetLastCommitedOrigin(), addresses[j],
          &sub_request_origins[j]))
          << "case: " << i << ", address: " << j;
    }

    url::Origin origin;
    ASSERT_TRUE(brave_wallet::GetConcatOriginFromWalletAddresses(
        GetLastCommitedOrigin(), addresses, &origin))
        << "case: " << i;

    auto observer = std::make_unique<PermissionRequestManagerObserver>(
        permission_request_manager);

    permission_manager()->RequestPermissionsForOrigin(
        permissions, web_contents()->GetPrimaryMainFrame(), origin.GetURL(),
        true, base::DoNothing());

    content::RunAllTasksUntilIdle();

    EXPECT_TRUE(permission_request_manager->IsRequestInProgress())
        << "case: " << i;
    EXPECT_TRUE(observer->IsShowingBubble()) << "case: " << i;
    // update anchor should not dismiss the bubble
    permission_request_manager->UpdateAnchor();
    EXPECT_TRUE(observer->IsShowingBubble()) << "case: " << i;
    EXPECT_FALSE(IsPendingGroupedRequestsEmpty(cases[i].type)) << "case: " << i;

    // Check sub-requests are created as expected.
    EXPECT_EQ(permission_request_manager->Requests().size(), addresses.size());
    for (size_t j = 0; j < permission_request_manager->Requests().size(); ++j) {
      EXPECT_EQ(permission_request_manager->Requests()[j]->request_type(),
                request_type)
          << "case: " << i << ", address: " << j;
      EXPECT_EQ(sub_request_origins[j].GetURL(),
                permission_request_manager->Requests()[j]->requesting_origin())
          << "case: " << i << ", address: " << j;
    }

    // Remove the observer before closing the tab.
    observer.reset();

    // Close tab with active request pending.
    content::WebContentsDestroyedWatcher tab_destroyed_watcher(web_contents());
    browser()->tab_strip_model()->CloseWebContentsAt(0,
                                                     TabCloseTypes::CLOSE_NONE);
    tab_destroyed_watcher.Wait();
    EXPECT_TRUE(IsPendingGroupedRequestsEmpty(cases[i].type));
  }
}

IN_PROC_BROWSER_TEST_F(PermissionManagerBrowserTest, GetCanonicalOrigin) {
  const GURL& url = https_server()->GetURL("a.test", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  struct {
    std::vector<std::string> addresses;
    ContentSettingsType type;
  } cases[] = {{{"0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
                 "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"},
                ContentSettingsType::BRAVE_ETHEREUM},
               {{"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                 "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV"},
                ContentSettingsType::BRAVE_SOLANA}};
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    url::Origin origin;
    ASSERT_TRUE(brave_wallet::GetConcatOriginFromWalletAddresses(
        GetLastCommitedOrigin(), cases[i].addresses, &origin))
        << "case: " << i;

    EXPECT_EQ(origin.GetURL(), permissions::PermissionUtil::GetCanonicalOrigin(
                                   cases[i].type, origin.GetURL(),
                                   GetLastCommitedOrigin().GetURL()))
        << "GetCanonicalOrigin should return requesting_origin for Ethereum "
           "permission. case: "
        << i;
  }
}

}  // namespace permissions
