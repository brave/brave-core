/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "brave/browser/extensions/api/brave_shields_api.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/api/content_settings/content_settings_api.h"
#include "chrome/browser/extensions/api/content_settings/content_settings_service.h"
#include "chrome/browser/extensions/api/content_settings/content_settings_store.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_builder.h"
#include "net/dns/mock_host_resolver.h"
#include "url/origin.h"

namespace extensions {

using extension_function_test_utils::RunFunctionAndReturnError;
using extension_function_test_utils::RunFunctionAndReturnSingleResult;
using extensions::api::BraveShieldsAllowScriptsOnceFunction;

class BraveShieldsAPIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
    extension_ = extensions::ExtensionBuilder("Test").Build();
    content_settings_ =
        HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  scoped_refptr<const extensions::Extension> extension() { return extension_; }

  HostContentSettingsMap* content_settings() const { return content_settings_; }

  void BlockScripts() {
    content_settings_->SetContentSettingCustomScope(
        ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
        ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_BLOCK);
  }

  bool NavigateToURLUntilLoadStop(const std::string& origin,
                                  const std::string& path) {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL(origin, path)));

    return WaitForLoadStop(active_contents());
  }

  void AllowScriptOriginOnce(const std::string& origin) {
    // run extension function to temporarily allow origin
    scoped_refptr<BraveShieldsAllowScriptsOnceFunction> function(
        new BraveShieldsAllowScriptsOnceFunction());
    function->set_extension(extension().get());
    function->set_has_callback(true);

    const GURL url(embedded_test_server()->GetURL(origin, "/simple.js"));
    url::Origin allow_origin = url::Origin::Create(url);
    int tabId = extensions::ExtensionTabUtil::GetTabId(active_contents());

    RunFunctionAndReturnSingleResult(function.get(),
                                     "[[\"" + allow_origin.Serialize() +
                                         "\"], " + std::to_string(tabId) + "]",
                                     browser());

    // reload page with a.test temporarily allowed
    active_contents()->GetController().Reload(content::ReloadType::NORMAL,
                                              true);
  }

  void AllowScriptOriginAndDataURLOnce(const std::string& origin,
                                       const std::string& dataURL) {
    scoped_refptr<BraveShieldsAllowScriptsOnceFunction> function(
        new BraveShieldsAllowScriptsOnceFunction());
    function->set_extension(extension().get());
    function->set_has_callback(true);

    const GURL url(embedded_test_server()->GetURL(origin, "/simple.js"));
    url::Origin allow_origin = url::Origin::Create(url);

    int tabId = extensions::ExtensionTabUtil::GetTabId(active_contents());
    RunFunctionAndReturnSingleResult(function.get(),
                                     "[[\"" + allow_origin.Serialize() +
                                         "\",\"" + dataURL + "\"], " +
                                         std::to_string(tabId) + "]",
                                     browser());

    // reload page with dataURL temporarily allowed
    active_contents()->GetController().Reload(content::ReloadType::NORMAL,
                                              true);
  }

  // Returns all the active RenderFrameHosts (e.g., not a prerendered or
  // back-forward cached page) for the test's active WebContents, so that we
  // don't get mixed with other RFHs that will be available as well now that
  // BFCache is enabled by default even for pages injecting scripts.
  std::vector<content::RenderFrameHost*> GetActiveRenderFrameHosts() {
    DCHECK(active_contents());
    std::vector<content::RenderFrameHost*> active_frames;
    for (content::RenderFrameHost* rfh :
         CollectAllRenderFrameHosts(active_contents())) {
      if (rfh->IsActive())
        active_frames.push_back(rfh);
    }
    return active_frames;
  }

 private:
  raw_ptr<HostContentSettingsMap> content_settings_ = nullptr;
  scoped_refptr<const extensions::Extension> extension_;
};

IN_PROC_BROWSER_TEST_F(BraveShieldsAPIBrowserTest, AllowScriptsOnce) {
  BlockScripts();

  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("a.test", "/load_js_from_origins.html"));
  EXPECT_EQ(GetActiveRenderFrameHosts().size(), 1u)
      << "All script loadings should be blocked.";

  AllowScriptOriginOnce("a.test");

  EXPECT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(GetActiveRenderFrameHosts().size(), 2u)
      << "Scripts from a.test should be temporarily allowed.";

  // reload page again
  active_contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(GetActiveRenderFrameHosts().size(), 2u)
      << "Scripts from a.test should be temporarily allowed after reload.";

  // same doc navigation
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(
                     "a.test", "/load_js_from_origins.html#foo")));
  EXPECT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(GetActiveRenderFrameHosts().size(), 2u)
      << "Scripts from a.test should be temporarily allowed for same doc "
         "navigation.";

  // navigate to a different origin
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("b.test", "/load_js_from_origins.html")));
  EXPECT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(GetActiveRenderFrameHosts().size(), 1u)
      << "All script loadings should be blocked after navigating away.";
}

IN_PROC_BROWSER_TEST_F(BraveShieldsAPIBrowserTest, AllowScriptsOnceDataURL) {
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("a.test", "/load_js_from_origins.html"));
  EXPECT_EQ(GetActiveRenderFrameHosts().size(), 4u)
      << "All script loadings should not be blocked by default.";

  BlockScripts();
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("a.test", "/load_js_from_origins.html"));
  EXPECT_EQ(GetActiveRenderFrameHosts().size(), 1u)
      << "All script loadings should be blocked.";

  AllowScriptOriginAndDataURLOnce(
      "a.test",
      "data:application/javascript;base64,"
      "dmFyIGZyYW1lID0gZG9jdW1lbnQuY3JlYXRlRWxlbWVudCgnaWZyYW1lJyk7CmRvY3VtZW"
      "50LmJvZHkuYXBwZW5kQ2hpbGQoZnJhbWUpOw==");

  EXPECT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(GetActiveRenderFrameHosts().size(), 3u)
      << "Scripts from a.test and data URL should be temporarily allowed.";
}

IN_PROC_BROWSER_TEST_F(BraveShieldsAPIBrowserTest, AllowScriptsOnceIframe) {
  BlockScripts();

  EXPECT_TRUE(NavigateToURLUntilLoadStop("a.com", "/remote_iframe.html"));
  EXPECT_EQ(GetActiveRenderFrameHosts().size(), 2u)
      << "All script loadings should be blocked.";

  AllowScriptOriginOnce("b.com");

  EXPECT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(GetActiveRenderFrameHosts().size(), 3u)
      << "Scripts from b.com should be temporarily allowed.";
}

constexpr char kJavascriptSetParams[] =
    "[\"block\", \"https://www.brave.com/\"]";
constexpr char kJavascriptGetParams[] = "[\"https://www.brave.com/\"]";

const GURL& GetBraveURL() {
  static const GURL kBraveURL("https://www.brave.com");
  return kBraveURL;
}

// Test javascript content setting works properly via braveShields api.
IN_PROC_BROWSER_TEST_F(BraveShieldsAPIBrowserTest,
                       GetNoScriptControlTypeFunction) {
  // Default content settings for javascript is allow.
  scoped_refptr<api::BraveShieldsGetNoScriptControlTypeFunction> get_function(
      new api::BraveShieldsGetNoScriptControlTypeFunction());
  get_function->set_extension(extension().get());
  std::unique_ptr<base::Value> value;
  value = RunFunctionAndReturnSingleResult(get_function.get(),
                                           kJavascriptGetParams, browser());
  EXPECT_EQ(value->GetString(), std::string("allow"));
}

IN_PROC_BROWSER_TEST_F(BraveShieldsAPIBrowserTest,
                       SetNoScriptControlTypeFunction) {
  // Block javascript.
  scoped_refptr<api::BraveShieldsSetNoScriptControlTypeFunction> set_function(
      new api::BraveShieldsSetNoScriptControlTypeFunction());
  set_function->set_extension(extension().get());
  RunFunctionAndReturnSingleResult(set_function.get(), kJavascriptSetParams,
                                   browser());

  // Check Block is set.
  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile())
          ->GetContentSetting(GetBraveURL(), GURL(),
                              ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(setting, CONTENT_SETTING_BLOCK);
}

// Checks shields configuration is persisted across the sessions.
IN_PROC_BROWSER_TEST_F(BraveShieldsAPIBrowserTest,
                       PRE_ShieldSettingsPersistTest) {
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(
          GetBraveURL(), GURL(),
          ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
          CONTENT_SETTING_ALLOW);

  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile())
          ->GetContentSetting(
              GetBraveURL(), GURL(),
              ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);
  EXPECT_EQ(setting, CONTENT_SETTING_ALLOW);
}

IN_PROC_BROWSER_TEST_F(BraveShieldsAPIBrowserTest, ShieldSettingsPersistTest) {
  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile())
          ->GetContentSetting(
              GetBraveURL(), GURL(),
              ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);
  EXPECT_EQ(setting, CONTENT_SETTING_ALLOW);
}

}  // namespace extensions
