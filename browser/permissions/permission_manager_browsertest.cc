/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/bind.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/permissions/permission_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
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
  void OnBubbleAdded() override { is_showing_bubble_ = true; }
  void OnBubbleRemoved() override { is_showing_bubble_ = false; }
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
    https_server()->ServeFilesFromDirectory(GetChromeTestDataDir());
    ASSERT_TRUE(https_server()->Start());

    permission_manager_ =
        PermissionManagerFactory::GetForProfile(browser()->profile());
  }

  PermissionRequestManager* GetPermissionRequestManager() {
    return PermissionRequestManager::FromWebContents(
        browser()->tab_strip_model()->GetActiveWebContents());
  }

  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }
  GURL GetLastCommitedOrigin() {
    return url::Origin::Create(web_contents()->GetLastCommittedURL()).GetURL();
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }
  PermissionManager* permission_manager() { return permission_manager_; }

  bool IsPendingGroupedRequestsEmpty() {
    PermissionContextBase* context =
        permission_manager()->GetPermissionContextForTesting(
            ContentSettingsType::BRAVE_ETHEREUM);
    return context->IsPendingGroupedRequestsEmptyForTesting();
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;
  raw_ptr<PermissionManager> permission_manager_ = nullptr;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(PermissionManagerBrowserTest,
                       RequestEthereumPermissions) {
  const GURL& url = https_server()->GetURL("a.test", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  auto* permission_request_manager = GetPermissionRequestManager();
  EXPECT_FALSE(permission_request_manager->IsRequestInProgress());
  EXPECT_TRUE(IsPendingGroupedRequestsEmpty());

  std::vector<std::string> addresses = {
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"};
  std::vector<ContentSettingsType> types(addresses.size(),
                                         ContentSettingsType::BRAVE_ETHEREUM);
  std::vector<GURL> sub_request_origins(addresses.size(), GURL(""));
  for (size_t i = 0; i < addresses.size(); i++) {
    ASSERT_TRUE(brave_wallet::GetSubRequestOrigin(
        GetLastCommitedOrigin(), addresses[i], &sub_request_origins[i]));
  }

  GURL origin;
  ASSERT_TRUE(brave_wallet::GetConcatOriginFromWalletAddresses(
      GetLastCommitedOrigin(), addresses, &origin));

  auto observer = std::make_unique<PermissionRequestManagerObserver>(
      permission_request_manager);

  permission_manager()->RequestPermissions(
      types, web_contents()->GetMainFrame(), origin, true, base::DoNothing());

  content::RunAllTasksUntilIdle();

  EXPECT_TRUE(permission_request_manager->IsRequestInProgress());
  EXPECT_TRUE(observer->IsShowingBubble());
  EXPECT_FALSE(IsPendingGroupedRequestsEmpty());

  // Check sub-requests are created as expected.
  EXPECT_EQ(permission_request_manager->Requests().size(), addresses.size());
  for (size_t i = 0; i < permission_request_manager->Requests().size(); i++) {
    EXPECT_EQ(permission_request_manager->Requests()[i]->request_type(),
              RequestType::kBraveEthereum);
    EXPECT_EQ(sub_request_origins[i],
              permission_request_manager->Requests()[i]->requesting_origin());
  }

  // Test dismissing request.
  permissions::BraveEthereumPermissionContext::Cancel(web_contents());
  EXPECT_TRUE(observer->IsRequestsFinalized());
  EXPECT_TRUE(!observer->IsShowingBubble());
  EXPECT_TRUE(IsPendingGroupedRequestsEmpty());

  for (size_t i = 0; i < addresses.size(); i++) {
    EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                  sub_request_origins[i], GetLastCommitedOrigin(),
                  ContentSettingsType::BRAVE_ETHEREUM),
              ContentSetting::CONTENT_SETTING_ASK);
  }

  observer->Reset();
  permission_manager()->RequestPermissions(
      types, web_contents()->GetMainFrame(), origin, true, base::DoNothing());

  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(permission_request_manager->IsRequestInProgress());
  EXPECT_TRUE(observer->IsShowingBubble());
  EXPECT_FALSE(IsPendingGroupedRequestsEmpty());

  // Check sub-requests are created as expected.
  EXPECT_EQ(permission_request_manager->Requests().size(), addresses.size());
  for (size_t i = 0; i < permission_request_manager->Requests().size(); i++) {
    EXPECT_EQ(permission_request_manager->Requests()[i]->request_type(),
              RequestType::kBraveEthereum);
    EXPECT_EQ(sub_request_origins[i],
              permission_request_manager->Requests()[i]->requesting_origin());
  }

  // Test accepting request with one of the address.
  permissions::BraveEthereumPermissionContext::AcceptOrCancel(
      std::vector<std::string>{addresses[1]}, web_contents());
  std::vector<ContentSetting> expected_settings(
      {ContentSetting::CONTENT_SETTING_ASK,
       ContentSetting::CONTENT_SETTING_ALLOW});
  EXPECT_TRUE(observer->IsRequestsFinalized());
  EXPECT_TRUE(!observer->IsShowingBubble());
  EXPECT_TRUE(IsPendingGroupedRequestsEmpty());

  for (size_t i = 0; i < addresses.size(); i++) {
    EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                  sub_request_origins[i], GetLastCommitedOrigin(),
                  ContentSettingsType::BRAVE_ETHEREUM),
              expected_settings[i]);
  }
}

IN_PROC_BROWSER_TEST_F(PermissionManagerBrowserTest,
                       RequestEthereumPermissionsTabClosed) {
  const GURL& url = https_server()->GetURL("a.test", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  auto* permission_request_manager = GetPermissionRequestManager();
  EXPECT_FALSE(permission_request_manager->IsRequestInProgress());
  EXPECT_TRUE(IsPendingGroupedRequestsEmpty());

  std::vector<std::string> addresses = {
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8C",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8D"};
  std::vector<ContentSettingsType> types(addresses.size(),
                                         ContentSettingsType::BRAVE_ETHEREUM);
  std::vector<GURL> sub_request_origins(addresses.size(), GURL(""));
  for (size_t i = 0; i < addresses.size(); i++) {
    ASSERT_TRUE(brave_wallet::GetSubRequestOrigin(
        GetLastCommitedOrigin(), addresses[i], &sub_request_origins[i]));
  }

  GURL origin;
  ASSERT_TRUE(brave_wallet::GetConcatOriginFromWalletAddresses(
      GetLastCommitedOrigin(), addresses, &origin));

  auto observer = std::make_unique<PermissionRequestManagerObserver>(
      permission_request_manager);

  permission_manager()->RequestPermissions(
      types, web_contents()->GetMainFrame(), origin, true, base::DoNothing());

  content::RunAllTasksUntilIdle();

  EXPECT_TRUE(permission_request_manager->IsRequestInProgress());
  EXPECT_TRUE(observer->IsShowingBubble());
  EXPECT_FALSE(IsPendingGroupedRequestsEmpty());

  // Check sub-requests are created as expected.
  EXPECT_EQ(permission_request_manager->Requests().size(), addresses.size());
  for (size_t i = 0; i < permission_request_manager->Requests().size(); i++) {
    EXPECT_EQ(permission_request_manager->Requests()[i]->request_type(),
              RequestType::kBraveEthereum);
    EXPECT_EQ(sub_request_origins[i],
              permission_request_manager->Requests()[i]->requesting_origin());
  }

  // Remove the observer before closing the tab.
  observer.reset();

  // Close tab with active request pending.
  content::WebContentsDestroyedWatcher tab_destroyed_watcher(web_contents());
  browser()->tab_strip_model()->CloseWebContentsAt(0,
                                                   TabStripModel::CLOSE_NONE);
  tab_destroyed_watcher.Wait();
  EXPECT_TRUE(IsPendingGroupedRequestsEmpty());
}

IN_PROC_BROWSER_TEST_F(PermissionManagerBrowserTest,
                       RequestEthereumPermissionsBlock3PIframe) {
  std::vector<std::string> addresses = {
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"};

  GURL top_url(https_server()->GetURL("a.test", "/iframe.html"));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), top_url));
  GURL iframe_url(https_server()->GetURL("b.test", "/"));
  EXPECT_TRUE(NavigateIframeToURL(web_contents(), "test", iframe_url));

  auto* iframe_rfh = ChildFrameAt(web_contents()->GetMainFrame(), 0);

  // Will return empty responses without prompt.
  BraveEthereumPermissionContext::RequestPermissions(
      iframe_rfh, addresses,
      base::BindOnce([](const std::vector<ContentSetting>& responses) {
        EXPECT_TRUE(responses.empty());
      }));

  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(IsPendingGroupedRequestsEmpty());
}

IN_PROC_BROWSER_TEST_F(PermissionManagerBrowserTest, GetCanonicalOrigin) {
  const GURL& url = https_server()->GetURL("a.test", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  std::vector<std::string> addresses = {
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"};
  GURL origin;
  ASSERT_TRUE(brave_wallet::GetConcatOriginFromWalletAddresses(
      GetLastCommitedOrigin(), addresses, &origin));

  EXPECT_EQ(origin, permission_manager()->GetCanonicalOrigin(
                        ContentSettingsType::BRAVE_ETHEREUM, origin,
                        GetLastCommitedOrigin()))
      << "GetCanonicalOrigin should return requesting_origin for Ethereum "
         "permission.";
}

}  // namespace permissions
