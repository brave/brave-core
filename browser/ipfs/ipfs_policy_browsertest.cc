/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/browser/ipfs/content_browser_client_helper.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/ipfs/ipfs_tab_helper.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_navigation_throttle.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/navigation_handle_observer.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

#include "components/infobars/core/infobar.h"

namespace policy {

template <bool enable>
class IpfsPolicyTest : public InProcessBrowserTest {
 public:
  IpfsPolicyTest() {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
    ipfs_url_ = GURL(
        "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/"
        "wiki/Vincent_van_Gogh.html");

    auto* prefs = user_prefs::UserPrefs::Get(browser_context());
    prefs->SetInteger(
        kIPFSResolveMethod,
        static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));
  }

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
    PolicyMap policies;
    policies.Set(key::kIPFSEnabled, POLICY_LEVEL_MANDATORY,
                 POLICY_SCOPE_USER, POLICY_SOURCE_PLATFORM,
                 base::Value(enable), nullptr);
    provider_.UpdateChromePolicy(policies);
  }
  PrefService* prefs() { return user_prefs::UserPrefs::Get(browser_context()); }
  content::WebContents* web_contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::BrowserContext* browser_context() {
    return web_contents()->GetBrowserContext();
  }

  size_t infobar_count() const {
    InfoBarService* infobar_service =
        InfoBarService::FromWebContents(web_contents());
    return infobar_service->infobar_count();
  }

  const GURL& ipfs_url() { return ipfs_url_; }

 private:
  MockConfigurationPolicyProvider provider_;
  base::test::ScopedFeatureList feature_list_;
  GURL ipfs_url_;
};

using IpfsEnabledPolicyTest = IpfsPolicyTest<true>;
using IpfsDisabledPolicyTest = IpfsPolicyTest<false>;

IN_PROC_BROWSER_TEST_F(IpfsEnabledPolicyTest, IsIpfsDisabledByPolicy) {
  auto* prefs = user_prefs::UserPrefs::Get(browser_context());
  EXPECT_FALSE(ipfs::IsIpfsDisabledByPolicy(prefs));
  EXPECT_TRUE(prefs->FindPreference(kIPFSEnabled));
  EXPECT_TRUE(prefs->GetBoolean(kIPFSEnabled));
}

IN_PROC_BROWSER_TEST_F(IpfsDisabledPolicyTest, IsIpfsDisabledByPolicy) {
  auto* prefs = user_prefs::UserPrefs::Get(browser_context());
  EXPECT_TRUE(ipfs::IsIpfsDisabledByPolicy(prefs));
  EXPECT_TRUE(prefs->FindPreference(kIPFSEnabled));
  EXPECT_FALSE(prefs->GetBoolean(kIPFSEnabled));
}

IN_PROC_BROWSER_TEST_F(IpfsEnabledPolicyTest, GetService) {
  EXPECT_NE(ipfs::IpfsServiceFactory::GetForContext(browser_context()),
            nullptr);
}

IN_PROC_BROWSER_TEST_F(IpfsDisabledPolicyTest, GetService) {
  EXPECT_EQ(ipfs::IpfsServiceFactory::GetForContext(browser_context()),
            nullptr);
}

IN_PROC_BROWSER_TEST_F(IpfsEnabledPolicyTest, IPFSPageAccess) {
  content::NavigationHandleObserver observer(web_contents(),
                                             GURL("chrome://ipfs-internals"));
  EXPECT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://ipfs-internals")));
  EXPECT_TRUE(observer.has_committed());
  EXPECT_FALSE(observer.is_error());
}

IN_PROC_BROWSER_TEST_F(IpfsDisabledPolicyTest, IPFSPageAccess) {
  content::NavigationHandleObserver observer(web_contents(),
                                             GURL("chrome://ipfs-internals"));
  EXPECT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://ipfs-internals")));
  EXPECT_TRUE(observer.has_committed());
  EXPECT_TRUE(observer.is_error());
}

IN_PROC_BROWSER_TEST_F(IpfsDisabledPolicyTest, IPFSPageAccessWithRedirect) {
  content::NavigationHandleObserver observer(web_contents(),
                                             GURL("chrome://ipfs-internals"));
  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("chrome://ipfs")));
  EXPECT_TRUE(observer.has_committed());
  EXPECT_TRUE(observer.is_error());
}

IN_PROC_BROWSER_TEST_F(IpfsEnabledPolicyTest, NavigationThrottle) {
  content::MockNavigationHandle test_handle(web_contents());
  auto throttle = ipfs::IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &test_handle, ipfs::IpfsServiceFactory::GetForContext(browser_context()),
      prefs(), "en-US");
  EXPECT_TRUE(throttle != nullptr);
}

IN_PROC_BROWSER_TEST_F(IpfsDisabledPolicyTest, NavigationThrottle) {
  content::MockNavigationHandle test_handle(web_contents());
  auto throttle = ipfs::IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &test_handle, ipfs::IpfsServiceFactory::GetForContext(browser_context()),
      prefs(), "en-US");
  EXPECT_TRUE(throttle == nullptr);
}

IN_PROC_BROWSER_TEST_F(IpfsEnabledPolicyTest, TabHelper) {
  EXPECT_TRUE(ipfs::IPFSTabHelper::MaybeCreateForWebContents(web_contents()));
}

IN_PROC_BROWSER_TEST_F(IpfsDisabledPolicyTest, TabHelper) {
  EXPECT_FALSE(ipfs::IPFSTabHelper::MaybeCreateForWebContents(web_contents()));
}

IN_PROC_BROWSER_TEST_F(IpfsEnabledPolicyTest, HandleIPFSURLRewrite) {
  GURL url(ipfs_url());
  EXPECT_TRUE(ipfs::HandleIPFSURLRewrite(&url, browser_context()));
}

IN_PROC_BROWSER_TEST_F(IpfsDisabledPolicyTest, HandleIPFSURLRewrite) {
  GURL url(ipfs_url());
  EXPECT_FALSE(ipfs::HandleIPFSURLRewrite(&url, browser_context()));
}

}  // namespace policy
