/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"

#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/values.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "url/gurl.h"

namespace brave_shields {

namespace {

class TestBraveShieldsWebContentsObserver
    : public BraveShieldsWebContentsObserver {
 public:
  explicit TestBraveShieldsWebContentsObserver(
      content::WebContents* web_contents)
      : BraveShieldsWebContentsObserver(web_contents) {}

  // brave_shields::mojom::BraveShieldsHost.
  void OnJavaScriptBlocked(const std::u16string& details) override {
    BraveShieldsWebContentsObserver::OnJavaScriptBlocked(details);
    block_javascript_count_++;
  }

  void Reset() { block_javascript_count_ = 0; }

  int block_javascript_count() { return block_javascript_count_; }

 private:
  int block_javascript_count_ = 0;
};

}  // namespace

class BraveShieldsWebContentsObserverBrowserTest : public InProcessBrowserTest {
 public:
  BraveShieldsWebContentsObserverBrowserTest() = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
    content_settings_ =
        HostContentSettingsMapFactory::GetForProfile(browser()->profile());

    // We can't simply create a new BraveShieldsWebContentsObserver for the same
    // WebContents, as that class will instatiate a RenderFrameHostReceiverSet
    // and we won't be able to intercept the mojo messages received for the
    // brave_shields::mojom::BraveShieldsHost interface for testing purposes.
    // Instead we call SetReceiverImplForTesting() to make sure that the mojo
    // receiver will be bound to our TestBraveShieldsWebContentsObserver class,
    // allowing us to intercept any message we are interested in.
    brave_shields_web_contents_observer_ =
        new TestBraveShieldsWebContentsObserver(GetWebContents());
    BraveShieldsWebContentsObserver::SetReceiverImplForTesting(
        brave_shields_web_contents_observer_);
  }

  std::vector<GURL> GetBlockedJsList() {
    return brave_shields::BraveShieldsTabHelper::FromWebContents(
               GetWebContents())
        ->GetBlockedJsList();
  }

  std::vector<GURL> GetAllowedJsList() {
    return brave_shields::BraveShieldsTabHelper::FromWebContents(
               GetWebContents())
        ->GetAllowedJsList();
  }

  void ClearAllResourcesList() {
    return brave_shields::BraveShieldsTabHelper::FromWebContents(
               GetWebContents())
        ->ClearAllResourcesList();
  }

  void TearDownOnMainThread() override {
    BraveShieldsWebContentsObserver::SetReceiverImplForTesting(nullptr);
  }

  content::WebContents* GetWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  HostContentSettingsMap* content_settings() { return content_settings_; }

  TestBraveShieldsWebContentsObserver* brave_shields_web_contents_observer() {
    return brave_shields_web_contents_observer_;
  }

 private:
  raw_ptr<HostContentSettingsMap> content_settings_ = nullptr;
  raw_ptr<TestBraveShieldsWebContentsObserver>
      brave_shields_web_contents_observer_ = nullptr;
};

class BraveShieldsWebContentsObserverManagedPolicyBrowserTest
    : public BraveShieldsWebContentsObserverBrowserTest {
 public:
  BraveShieldsWebContentsObserverManagedPolicyBrowserTest() = default;

  void SetUpOnMainThread() override {
    BraveShieldsWebContentsObserverBrowserTest::SetUpOnMainThread();
  }

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
    policy::PolicyMap policies;

    // Set JavaScript blocked for URLs policy
    policies.Set(policy::key::kJavaScriptAllowedForUrls,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_PLATFORM, base::Value(), nullptr);
    auto blocked_list = base::Value::List().Append("http://a.com");
    policies.Set(policy::key::kJavaScriptBlockedForUrls,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_PLATFORM,
                 base::Value(std::move(blocked_list)), nullptr);
    provider_.UpdateChromePolicy(policies);
  }

 private:
  policy::MockConfigurationPolicyProvider provider_;
};

IN_PROC_BROWSER_TEST_F(BraveShieldsWebContentsObserverManagedPolicyBrowserTest,
                       JavaScriptBlockedEvents) {
  auto a_com_url = GURL("http://a.com");
  auto b_com_url = GURL("http://b.com");

  // Verify that the policy is applied correctly
  ContentSetting a_com_javascript_setting =
      content_settings()->GetContentSetting(a_com_url, a_com_url,
                                            ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, a_com_javascript_setting);
  ContentSetting b_com_javascript_setting =
      content_settings()->GetContentSetting(b_com_url, b_com_url,
                                            ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, b_com_javascript_setting);

  // Navigate to the to the a.com URL which has JavaScript blocked by policy
  EXPECT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("a.com", "/load_js.html")));
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_EQ(brave_shields_web_contents_observer()->block_javascript_count(), 5);
  EXPECT_EQ(GetBlockedJsList().size(), 3u);
}

IN_PROC_BROWSER_TEST_F(BraveShieldsWebContentsObserverBrowserTest,
                       JavaScriptBlockedEvents) {
  const GURL& url = GURL("a.com");

  // Start with JavaScript blocking initially disabled.
  ContentSetting block_javascript_setting =
      content_settings()->GetContentSetting(url, url,
                                            ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, block_javascript_setting);

  // Load a simple HTML that attempts to load some JavaScript without blocking.
  EXPECT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("a.com", "/load_js.html")));
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_EQ(brave_shields_web_contents_observer()->block_javascript_count(), 0);
  EXPECT_EQ(GetBlockedJsList().size(), 0u);
  // Enable JavaScript blocking globally now.
  content_settings()->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_BLOCK);
  block_javascript_setting = content_settings()->GetContentSetting(
      url, url, ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, block_javascript_setting);

  // Reload the test page now that JavaScript has been blocked.
  brave_shields_web_contents_observer()->Reset();
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_GT(brave_shields_web_contents_observer()->block_javascript_count(), 0);
  EXPECT_EQ(GetBlockedJsList().size(), 3u);

  // Disable JavaScript blocking again now.
  content_settings()->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_ALLOW);
  block_javascript_setting = content_settings()->GetContentSetting(
      url, url, ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, block_javascript_setting);

  // Reload the test page now that JavaScript has been allowed again.
  // Do it twice, because first reload will still trigger blocked events as
  // renderer caches AllowScript results in
  // ContentSettingsAgentImpl::cached_script_permissions_.
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));

  brave_shields_web_contents_observer()->Reset();
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_EQ(brave_shields_web_contents_observer()->block_javascript_count(), 0);
  EXPECT_EQ(GetBlockedJsList().size(), 0u);
}

IN_PROC_BROWSER_TEST_F(BraveShieldsWebContentsObserverBrowserTest,
                       EmbeddedJavaScriptTriggersBlockedEvent) {
  // Enable JavaScript blocking globally.
  content_settings()->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_BLOCK);

  // Load a simple HTML that attempts to run some JavaScript.
  EXPECT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("a.com", "/embedded_js.html")));
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_GT(brave_shields_web_contents_observer()->block_javascript_count(), 0);
  EXPECT_EQ(GetBlockedJsList().size(), 1u);
}

IN_PROC_BROWSER_TEST_F(BraveShieldsWebContentsObserverBrowserTest,
                       JavaScriptAllowedEvents) {
  const GURL& url = GURL("a.com");

  // Start with JavaScript blocking initially disabled.
  ContentSetting block_javascript_setting =
      content_settings()->GetContentSetting(url, url,
                                            ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, block_javascript_setting);

  // Load a simple HTML that attempts to load some JavaScript without blocking.
  EXPECT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("a.com", "/load_js.html")));
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_EQ(brave_shields_web_contents_observer()->block_javascript_count(), 0);

  // Enable JavaScript blocking globally now.
  content_settings()->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_BLOCK);
  block_javascript_setting = content_settings()->GetContentSetting(
      url, url, ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, block_javascript_setting);

  // Reload the test page now that JavaScript has been blocked.
  brave_shields_web_contents_observer()->Reset();
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_GT(brave_shields_web_contents_observer()->block_javascript_count(), 0);
  auto blocked_list = GetBlockedJsList();
  EXPECT_EQ(blocked_list.size(), 3u);

  // Allow One Script
  brave_shields_web_contents_observer()->AllowScriptsOnce(
      std::vector<std::string>({blocked_list.back().spec()}));
  ClearAllResourcesList();
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_EQ(GetBlockedJsList().size(), 2u);
  EXPECT_EQ(GetAllowedJsList().size(), 1u);

  blocked_list.pop_back();
  EXPECT_EQ(blocked_list.size(), 2u);

  // Allow Second Script
  brave_shields_web_contents_observer()->AllowScriptsOnce(
      std::vector<std::string>({blocked_list.back().spec()}));
  ClearAllResourcesList();
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_EQ(GetBlockedJsList().size(), 1u);
  EXPECT_EQ(GetAllowedJsList().size(), 2u);

  // Block one of allowed scripts.
  brave_shields_web_contents_observer()->BlockAllowedScripts(
      {blocked_list.back().spec()});
  ClearAllResourcesList();
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_EQ(GetBlockedJsList().size(), 2u);
  EXPECT_EQ(GetAllowedJsList().size(), 1u);

  brave_shields_web_contents_observer()->BlockAllowedScripts(
      {url::Origin::Create(blocked_list.back()).Serialize()});
  ClearAllResourcesList();
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_EQ(GetBlockedJsList().size(), 3u);
  EXPECT_EQ(GetAllowedJsList().size(), 0u);

  // Disable JavaScript blocking again now.
  content_settings()->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_ALLOW);
  block_javascript_setting = content_settings()->GetContentSetting(
      url, url, ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, block_javascript_setting);

  // Reload the test page now that JavaScript has been allowed again.
  // Do it twice, because first reload will still trigger blocked events as
  // renderer caches AllowScript results in
  // ContentSettingsAgentImpl::cached_script_permissions_.
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));

  brave_shields_web_contents_observer()->Reset();
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_EQ(brave_shields_web_contents_observer()->block_javascript_count(), 0);
}

IN_PROC_BROWSER_TEST_F(BraveShieldsWebContentsObserverBrowserTest,
                       JavaScriptAllowedDataUrls) {
  const GURL& url = GURL("a.com");

  // Start with JavaScript blocking initially disabled.
  ContentSetting block_javascript_setting =
      content_settings()->GetContentSetting(url, url,
                                            ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, block_javascript_setting);

  // Enable JavaScript blocking globally now.
  content_settings()->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_BLOCK);
  block_javascript_setting = content_settings()->GetContentSetting(
      url, url, ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, block_javascript_setting);

  // Load a simple HTML that attempts to load some JavaScript with data urls.
  auto page_url =
      embedded_test_server()->GetURL("a.com", "/load_js_dataurls.html");
  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), page_url));
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_EQ(brave_shields_web_contents_observer()->block_javascript_count(), 4);
  brave_shields_web_contents_observer()->Reset();
  // Allow subframe script and check we still block his data urls.
  std::string subframe_script =
      url::Origin::Create(page_url).Serialize() + "/load_js_dataurls.js";
  brave_shields_web_contents_observer()->AllowScriptsOnce(
      std::vector<std::string>({subframe_script}));
  ClearAllResourcesList();
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));
  EXPECT_EQ(GetBlockedJsList().size(), 1u);
  EXPECT_EQ(GetAllowedJsList().size(), 1u);
  EXPECT_EQ(brave_shields_web_contents_observer()->block_javascript_count(), 3);
  brave_shields_web_contents_observer()->Reset();

  // Allow all scripts for domain.
  brave_shields_web_contents_observer()->AllowScriptsOnce(
      std::vector<std::string>({url::Origin::Create(page_url).Serialize()}));
  ClearAllResourcesList();
  GetWebContents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(GetWebContents()));

  EXPECT_EQ(GetAllowedJsList().size(), 2u);
  EXPECT_EQ(brave_shields_web_contents_observer()->block_javascript_count(), 0);
}

}  // namespace brave_shields
