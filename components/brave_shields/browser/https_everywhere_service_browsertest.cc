/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/base64.h"
#include "base/path_service.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_shields/https_everywhere_component_installer.h"
#include "brave/components/brave_shields/browser/ad_block_component_filters_provider.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "brave/components/brave_shields/browser/test_filters_provider.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using extensions::ExtensionBrowserTest;

const char kHTTPSEverywhereComponentTestId[] =
    "bhlmpjhncoojbkemjkeppfahkglffilp";

const char kHTTPSEverywhereComponentTestBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA3tAm7HooTNVGQ9cm7Yuc"
    "M9sLM/V38JOXzdj7z9dyDIfO64N69Gr5dn3XRzLuD+Pyzpl8MzfY/tIbWNSw3I2a"
    "8YcEPmyHl2L4HByKTm+eJ02ArhtkgtZKjiTDc84KQcsTBHqINkMUQYeUN3VW1lz2"
    "yuZJrGlqlKCmQq7iRjCSUFu/C9mbJghTF8aKqmLbuf/pUXLpXFCRhCfaeabPqZP4"
    "e9efRk7lsOraJMhF1Gcx0iubObKxl6Ov19e4nreYpw7Vp0fHodLzh0YxssLgNhTb"
    "txtjWrJaXB5wghi1G0coTy6TgTXxoU9OU70eyf6PgdW4ZcaBIyM3tY6tme4zukvv"
    "3wIDAQAB";

class HTTPSEverywhereServiceTest : public ExtensionBrowserTest {
 public:
  HTTPSEverywhereServiceTest() = default;

  void SetUpCommon() {
    InitEmbeddedTestServer();
    InitService();
    ExtensionBrowserTest::SetUp();
  }

  void SetUp() override {
    feature_list_.InitAndDisableFeature(net::features::kBraveHttpsByDefault);
    SetUpCommon();
  }

  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void PreRunTestOnMainThread() override {
    ExtensionBrowserTest::PreRunTestOnMainThread();
    WaitForHTTPSEverywhereServiceThread();
    WaitForAdBlockServiceThreads();
    ASSERT_TRUE(
      g_brave_browser_process->https_everywhere_service()->IsInitialized());
  }

  void InitEmbeddedTestServer() {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void InitService() {
    brave_shields::HTTPSEverywhereService::SetIgnorePortForTest(true);
    brave_shields::SetHTTPSEverywhereComponentIdAndBase64PublicKeyForTest(
        kHTTPSEverywhereComponentTestId,
        kHTTPSEverywhereComponentTestBase64PublicKey);
  }

  void GetTestDataDir(base::FilePath* test_data_dir) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir);
  }

  bool InstallHTTPSEverywhereExtension() {
    base::FilePath test_data_dir;
    GetTestDataDir(&test_data_dir);
    const extensions::Extension* httpse_extension =
        InstallExtension(test_data_dir.AppendASCII("https-everywhere-data"), 1);
    if (!httpse_extension)
      return false;

    g_brave_browser_process->https_everywhere_service()->InitDB(
        httpse_extension->path());
    WaitForHTTPSEverywhereServiceThread();

    return true;
  }

  void WaitForHTTPSEverywhereServiceThread() {
    scoped_refptr<base::ThreadTestHelper> io_helper(new base::ThreadTestHelper(
        g_brave_browser_process->https_everywhere_service()->GetTaskRunner()));
    ASSERT_TRUE(io_helper->Run());
  }

  void WaitForAdBlockServiceThreads() {
    scoped_refptr<base::ThreadTestHelper> tr_helper(new base::ThreadTestHelper(
        g_brave_browser_process->ad_block_service()->GetTaskRunner()));
    ASSERT_TRUE(tr_helper->Run());
  }

  std::vector<std::unique_ptr<brave_shields::TestFiltersProvider>>
      source_providers_;

  void UpdateCustomAdBlockInstanceWithRules(const std::string& rules,
                                            const std::string& resources) {
    auto source_provider =
        std::make_unique<brave_shields::TestFiltersProvider>(rules, resources);

    brave_shields::AdBlockService* ad_block_service =
        g_brave_browser_process->ad_block_service();
    ad_block_service->UseCustomSourceProvidersForTest(source_provider.get(),
                                                      source_provider.get());

    source_providers_.push_back(std::move(source_provider));

    WaitForAdBlockServiceThreads();
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
};

// Load a URL which has an HTTPSE rule and verify we rewrote it.
IN_PROC_BROWSER_TEST_F(HTTPSEverywhereServiceTest, RedirectsKnownSite) {
  ASSERT_TRUE(InstallHTTPSEverywhereExtension());

  GURL url = embedded_test_server()->GetURL("www.digg.com", "/");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(GURL("https://www.digg.com/"), contents->GetLastCommittedURL());
}

// Load a URL which has an HTTPSE rule and an adblock redirect rule - verify
// that the adblock rule takes precedence
IN_PROC_BROWSER_TEST_F(HTTPSEverywhereServiceTest, PreferAdblockRedirect) {
  ASSERT_TRUE(InstallHTTPSEverywhereExtension());

  std::string frame_html =
      "<html><script>"
      "  const customResource = true;"
      "</script></html>";

  std::string resource_base64;
  base::Base64Encode(frame_html, &resource_base64);

  UpdateCustomAdBlockInstanceWithRules(
      "www.digg.com$subdocument,redirect=custom-resource-html",
      R"(
      [
        {
          "name": "custom-resource-html",
          "aliases": [],
          "kind": {
            "mime":"text/html"
          },
          "content": ")" +
          resource_base64 + R"("
        }
      ])");

  GURL url = embedded_test_server()->GetURL("a.com", "/iframe.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  GURL iframe_url = embedded_test_server()->GetURL("www.digg.com", "/");
  const char kIframeID[] = "test";
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(NavigateIframeToURL(contents, kIframeID, iframe_url));
  content::RenderFrameHost* iframe_contents =
      ChildFrameAt(contents->GetPrimaryMainFrame(), 0);
  WaitForLoadStop(contents);

  // The URL should not be modified
  ASSERT_EQ(iframe_url, iframe_contents->GetLastCommittedURL());

  // The `customResource` JS property should be defined
  const GURL resource_url =
      embedded_test_server()->GetURL("example.com", "/example.html");
  ASSERT_EQ(true, EvalJs(iframe_contents, "customResource"));
}

// Load a URL which has no HTTPSE rule and verify we did not rewrite it.
IN_PROC_BROWSER_TEST_F(HTTPSEverywhereServiceTest, NoRedirectsNotKnownSite) {
  ASSERT_TRUE(InstallHTTPSEverywhereExtension());

  GURL url = embedded_test_server()->GetURL("www.brianbondy.com", "/");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  GURL::Replacements clear_port;
  clear_port.ClearPort();

  EXPECT_EQ(GURL("http://www.brianbondy.com/"),
            contents->GetLastCommittedURL().ReplaceComponents(clear_port));
}

// Make sure iframes that should redirect to HTTPS actually redirect and that
// the header is intact.
IN_PROC_BROWSER_TEST_F(HTTPSEverywhereServiceTest, RedirectsKnownSiteInIframe) {
  ASSERT_TRUE(InstallHTTPSEverywhereExtension());
  GURL url = embedded_test_server()->GetURL("a.com", "/iframe.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  GURL iframe_url = embedded_test_server()->GetURL("www.digg.com", "/");
  const char kIframeID[] = "test";
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(NavigateIframeToURL(contents, kIframeID, iframe_url));
  content::RenderFrameHost* iframe_contents =
      ChildFrameAt(contents->GetPrimaryMainFrame(), 0);
  WaitForLoadStop(contents);
  EXPECT_EQ(GURL("https://www.digg.com/"),
            iframe_contents->GetLastCommittedURL());
}

class HTTPSEverywhereServiceTest_HttpsByDefaultEnabled
    : public HTTPSEverywhereServiceTest {
 public:
  HTTPSEverywhereServiceTest_HttpsByDefaultEnabled() = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(net::features::kBraveHttpsByDefault);
    HTTPSEverywhereServiceTest::SetUpCommon();
  }
};

// Verify that HTTPE rules are disabled when HTTPS by Default is enabled
IN_PROC_BROWSER_TEST_F(HTTPSEverywhereServiceTest_HttpsByDefaultEnabled,
                       RedirectsKnownSite) {
  ASSERT_TRUE(InstallHTTPSEverywhereExtension());
  auto* settings_map =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  GURL insecure_url = embedded_test_server()->GetURL("www.digg.com", "/");
  brave_shields::SetHttpsUpgradeControlType(
      settings_map, brave_shields::ControlType::ALLOW, GURL());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), insecure_url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(insecure_url, contents->GetLastCommittedURL());
}
