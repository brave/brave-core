/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#include <memory>
#include <vector>

#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/omnibox/browser/location_bar_model.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "net/dns/mock_host_resolver.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#endif

class BraveContentBrowserClientTest : public InProcessBrowserTest {
 public:
  void SetUp() override {
    // This is needed because component extensions are not added by default
    // without it.  Theyu are found to interfere with tests otherwise. It's
    // needed for loading the hangouts extension of which there are tests for
    // below.
    extensions::ComponentLoader::EnableBackgroundExtensionsForTesting();
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    browser_content_client_ = std::make_unique<BraveContentBrowserClient>();
    content::SetBrowserClientForTesting(browser_content_client_.get());

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TearDown() override { browser_content_client_.reset(); }

  void NavigateToURLAndWaitForRewrites(content::WebContents* contents,
                                       const GURL& original_url,
                                       const GURL& final_url) {
    ui_test_utils::UrlLoadObserver load_complete(final_url);
    browser()->OpenURL(
        content::OpenURLParams(original_url, content::Referrer(),
                               WindowOpenDisposition::CURRENT_TAB,
                               ui::PAGE_TRANSITION_TYPED, false),
        /*navigation_handle_callback=*/{});
    load_complete.Wait();
    EXPECT_EQ(contents->GetLastCommittedURL(), final_url);
  }

  content::ContentBrowserClient* client() {
    return browser_content_client_.get();
  }

 private:
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, CanLoadChromeURL) {
  std::vector<std::string> pages{
      kWelcomeHost,
  };

  std::vector<std::string> schemes{
      "brave://",
      "chrome://",
  };

  for (const std::string& page : pages) {
    for (const std::string& scheme : schemes) {
      content::WebContents* contents =
          browser()->tab_strip_model()->GetActiveWebContents();
      ASSERT_TRUE(
          ui_test_utils::NavigateToURL(browser(), GURL(scheme + page + "/")));
      ASSERT_TRUE(WaitForLoadStop(contents));

      EXPECT_EQ(base::UTF16ToUTF8(
                    browser()->location_bar_model()->GetFormattedFullURL()),
                ("brave://" + page));
      EXPECT_EQ(contents->GetController()
                    .GetLastCommittedEntry()
                    ->GetVirtualURL()
                    .spec(),
                ("chrome://" + page + "/"));
      EXPECT_EQ(
          contents->GetController().GetLastCommittedEntry()->GetURL().spec(),
          ("chrome://" + page + "/"));
    }
  }
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, CanLoadCustomBravePages) {
  std::vector<std::string> pages{
      "rewards",
  };

  std::vector<std::string> schemes{
      "brave://",
      "chrome://",
  };

  for (const std::string& page : pages) {
    for (const std::string& scheme : schemes) {
      content::WebContents* contents =
          browser()->tab_strip_model()->GetActiveWebContents();
      ASSERT_TRUE(
          ui_test_utils::NavigateToURL(browser(), GURL(scheme + page + "/")));
      ASSERT_TRUE(WaitForLoadStop(contents));

      EXPECT_EQ(base::UTF16ToUTF8(
                    browser()->location_bar_model()->GetFormattedFullURL()),
                ("brave://" + page));
      EXPECT_EQ(contents->GetController()
                    .GetLastCommittedEntry()
                    ->GetVirtualURL()
                    .spec(),
                ("chrome://" + page + "/"));
      EXPECT_EQ(
          contents->GetController().GetLastCommittedEntry()->GetURL().spec(),
          ("chrome://" + page + "/"));
    }
  }
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, CanLoadAboutHost) {
  std::vector<std::string> schemes{
      "chrome://",
      "brave://",
  };

  for (const std::string& scheme : schemes) {
    content::WebContents* contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL(scheme + "about/")));
    ASSERT_TRUE(WaitForLoadStop(contents));

    EXPECT_EQ(base::UTF16ToUTF8(
                  browser()->location_bar_model()->GetFormattedFullURL()),
              "brave://about");
    EXPECT_EQ(contents->GetController()
                  .GetLastCommittedEntry()
                  ->GetVirtualURL()
                  .spec(),
              "chrome://about/");
    EXPECT_EQ(
        contents->GetController().GetLastCommittedEntry()->GetURL().spec(),
        "chrome://chrome-urls/");
  }
}
IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, RewriteChromeSync) {
  std::vector<std::string> schemes{
      "brave://",
      "chrome://",
  };

  for (const std::string& scheme : schemes) {
    content::WebContents* contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    NavigateToURLAndWaitForRewrites(contents,
                                    GURL(scheme + chrome::kBraveUISyncHost),
                                    GURL("chrome://sync"));

    EXPECT_EQ(base::UTF16ToUTF8(
                  browser()->location_bar_model()->GetFormattedFullURL()),
              "brave://sync");
    EXPECT_EQ(
        contents->GetController().GetLastCommittedEntry()->GetVirtualURL(),
        GURL("chrome://sync"));
    EXPECT_EQ(contents->GetController().GetLastCommittedEntry()->GetURL(),
              GURL("chrome://settings/braveSync"));
  }
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, RewriteAdblock) {
  std::vector<std::string> schemes{
      "brave://",
      "chrome://",
  };

  for (const std::string& scheme : schemes) {
    content::WebContents* contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    NavigateToURLAndWaitForRewrites(contents, GURL(scheme + "adblock"),
                                    GURL("chrome://settings/shields/filters"));
    EXPECT_EQ(base::UTF16ToUTF8(
                  browser()->location_bar_model()->GetFormattedFullURL()),
              "brave://settings/shields/filters");
    EXPECT_EQ(browser()->location_bar_model()->GetURL(),
              GURL("chrome://settings/shields/filters"));
    EXPECT_EQ(
        contents->GetController().GetLastCommittedEntry()->GetVirtualURL(),
        GURL("chrome://settings/shields/filters"));
  }
}

#if BUILDFLAG(ENABLE_TOR)
IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, MixedContentForOnion) {
  net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(true);
  tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(true);
  Browser* tor_browser =
      TorProfileManager::SwitchToTorProfile(browser()->profile());

  const GURL onion_url =
      embedded_test_server()->GetURL("test.onion", "/onion.html");
  const GURL onion_upgradable_url =
      embedded_test_server()->GetURL("test.onion", "/onion_upgradable.html");

  ASSERT_EQ("http", onion_url.scheme());
  content::WebContents* contents =
      tor_browser->tab_strip_model()->GetActiveWebContents();
  {
    content::WebContentsConsoleObserver console_observer(contents);
    // filter out noises like "crbug/1173575, non-JS module files deprecated"
    // since we are only interested in mix content error
    console_observer.SetFilter(base::BindLambdaForTesting(
        [](const content::WebContentsConsoleObserver::Message& message) {
          return message.log_level == blink::mojom::ConsoleMessageLevel::kError;
        }));
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(tor_browser, onion_upgradable_url));
    EXPECT_TRUE(console_observer.messages().empty());
  }
  {
    content::WebContentsConsoleObserver console_observer(contents);
    console_observer.SetPattern(
        "Mixed Content: The page at 'http://test.onion*/onion.html' was loaded "
        "over HTTPS, but requested an insecure element "
        "'http://auto_upgradable_to_https.com/image.jpg'. This request was "
        "automatically upgraded to HTTPS*");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(tor_browser, onion_url));
    ASSERT_TRUE(console_observer.Wait());
  }
  auto fetch = [](const std::string& resource) {
    return "fetch('" + resource + "').then((response) => { console.log('" +
           resource + "' + ' ' + response.statusText) })";
  };
  {
    content::WebContentsConsoleObserver console_observer(contents);
    console_observer.SetPattern(
        "Mixed Content: The page at 'http://test.onion*/onion.html' was "
        "loaded over HTTPS, but requested an insecure resource "
        "'http://example.com*'. This request has been blocked; the content "
        "must be served over HTTPS.");
    const GURL resource_url =
        embedded_test_server()->GetURL("example.com", "/logo-referrer.png");
    const std::string kFetchScript = fetch(resource_url.spec());
    ASSERT_FALSE(content::ExecJs(contents, kFetchScript));
    ASSERT_TRUE(console_observer.Wait());
  }
  {
    auto https_server = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server->SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server->AddDefaultHandlers();
    ASSERT_TRUE(https_server->Start());

    content::WebContentsConsoleObserver console_observer(contents);
    const auto resource_url =
        https_server->GetURL("example.a.test", "/echoheader").spec();
    console_observer.SetPattern(resource_url + " OK");
    const std::string kFetchScript = fetch(resource_url);
    ASSERT_TRUE(content::ExecJs(contents, kFetchScript));
    ASSERT_TRUE(console_observer.Wait());
  }
  {
    content::WebContentsConsoleObserver console_observer(contents);
    // logo-referrer.png sets "access-control-allow-origin: *"
    const auto resource_url =
        embedded_test_server()
            ->GetURL("example.onion", "/logo-referrer.png")
            .spec();
    console_observer.SetPattern(resource_url + " OK");
    const std::string kFetchScript = fetch(resource_url);
    ASSERT_TRUE(content::ExecJs(contents, kFetchScript));
    ASSERT_TRUE(console_observer.Wait());
  }
}
#endif

class BraveContentBrowserClientReferrerTest
    : public BraveContentBrowserClientTest {
 public:
  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }
};

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientReferrerTest,
                       DefaultBehaviour) {
  const GURL kRequestUrl("http://request.com/path?query");
  const GURL kDocumentUrl("http://document.com/path?query");
  const GURL kSameSiteRequestUrl("http://sub.document.com/sub/path");
  const GURL kSameOriginRequestUrl("http://document.com/different/path");

  blink::mojom::ReferrerPtr kReferrer = blink::mojom::Referrer::New(
      kDocumentUrl, network::mojom::ReferrerPolicy::kDefault);

  // Cross-origin navigations get an origin.
  blink::mojom::ReferrerPtr referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(), kRequestUrl, kDocumentUrl,
                              &referrer);

  // Creating an Origin off a GURL should generally be avoided, but it's ok in
  // this particular case where we're just testing and using the http protocol.
  GURL document_url_origin = url::Origin::Create(kDocumentUrl).GetURL();
  EXPECT_EQ(referrer->url, document_url_origin);

  // Same-origin navigations get full referrers.
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(), kSameOriginRequestUrl,
                              kDocumentUrl, &referrer);
  EXPECT_EQ(referrer->url, kDocumentUrl);

  // Same-site navigations get truncated referrers.
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(), kSameSiteRequestUrl,
                              kDocumentUrl, &referrer);
  EXPECT_EQ(referrer->url, document_url_origin);

  // Cross-origin iframe navigations get origins.
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(), kRequestUrl, kDocumentUrl,
                              &referrer);
  EXPECT_EQ(referrer->url, document_url_origin);

  // Same-origin iframe navigations get full referrers.
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(), kSameOriginRequestUrl,
                              kDocumentUrl, &referrer);
  EXPECT_EQ(referrer->url, kDocumentUrl);

  // Special rule for extensions.
  const GURL kExtensionUrl("chrome-extension://abc/path?query");
  referrer = kReferrer.Clone();
  referrer->url = kExtensionUrl;
  client()->MaybeHideReferrer(browser()->profile(), kRequestUrl, kExtensionUrl,
                              &referrer);
  EXPECT_EQ(referrer->url, kExtensionUrl);

  // Special rule for Onion services.
  const GURL kOnionUrl("http://lwkjglkejslkgjel.onion/index.html");
  referrer = kReferrer.Clone();
  referrer->url = kOnionUrl;
  client()->MaybeHideReferrer(browser()->profile(), kRequestUrl, kOnionUrl,
                              &referrer);
  EXPECT_EQ(referrer->url, GURL());  // .onion -> normal
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(), kOnionUrl, kDocumentUrl,
                              &referrer);
  EXPECT_EQ(referrer->url, document_url_origin);  // normal -> .onion

  // Allow referrers for certain URL.
  content_settings()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString(document_url_origin.spec() + "*"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_REFERRERS,
      CONTENT_SETTING_ALLOW);
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(), kRequestUrl, kDocumentUrl,
                              &referrer);
  EXPECT_EQ(referrer->url, kDocumentUrl);
}

// Confirm only expected extension has been installed.
IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, CheckExpectedExtensions) {
  // Info: This checkup will not cover on-demand component extension
  // installation.
  std::set<std::string> expected_extensions = {
      brave_extension_id,
      extensions::kWebStoreAppId,
      extension_misc::kPdfExtensionId,
  };

  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser()->profile());
  std::set<std::string> installed_extensions =
      registry->GenerateInstalledExtensionsSet().GetIDs();

  EXPECT_EQ(expected_extensions, installed_extensions);

  const auto* brave_extension =
      registry->GetInstalledExtension(brave_extension_id);

  // Brave Extension background page should be disabled by default.
  EXPECT_FALSE(extensions::BackgroundInfo::HasBackgroundPage(brave_extension));
}
