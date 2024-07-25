/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/base64url.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#endif

class BraveSiteHacksNetworkDelegateBrowserTest : public InProcessBrowserTest {
 public:
  BraveSiteHacksNetworkDelegateBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir_);
    https_server_.ServeFilesFromDirectory(test_data_dir_);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    content::SetupCrossSiteRedirector(&https_server_);

    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &BraveSiteHacksNetworkDelegateBrowserTest::HandleRequest,
        base::Unretained(this)));

    ASSERT_TRUE(https_server_.Start());

    simple_landing_url_ = https_server_.GetURL("a.com", "/simple.html");
    redirect_to_cross_site_landing_url_ =
        https_server_.GetURL("redir.b.com", "/cross-site/a.com/simple.html");
    redirect_to_same_site_landing_url_ =
        https_server_.GetURL("redir.a.com", "/cross-site/a.com/simple.html");

    cross_site_url_ = https_server_.GetURL("b.com", "/navigate-to-site.html");
    cross_site_post_url_ = https_server_.GetURL("b.com", "/post-to-site.html");
    same_site_url_ =
        https_server_.GetURL("sub.a.com", "/navigate-to-site.html");

    onion_url_ = https_server_.GetURL("foobar.onion", "/navigate-to-site.html");
    onion_post_url_ =
        https_server_.GetURL("foobar.onion", "/post-to-site.html");
    reflect_referrer_cross_origin_url_ =
        https_server_.GetURL("a.com", "/reflect-referrer.html");
    reflect_referrer_cross_origin_redirect_url_ = https_server_.GetURL(
        "foobar.onion",
        "/server-redirect-307?" + reflect_referrer_cross_origin_url_.spec());
    reflect_referrer_same_origin_url_ =
        https_server_.GetURL("foobar.onion", "/reflect-referrer.html");
    reflect_referrer_same_origin_redirect_url_ = https_server_.GetURL(
        "foobar.onion",
        "/server-redirect-307?" + reflect_referrer_same_origin_url_.spec());
    images_url_ = https_server_.GetURL("foobar.onion", "/referrer_images.html");

    iframe_inner_url_ = https_server_.GetURL("a.com", "/reflect-referrer.html");
    iframe_outer_url_ = https_server_.GetURL("a.com", "/iframe_load.html");
    onion_iframe_inner_url_ =
        https_server_.GetURL("foobar.onion", "/reflect-referrer.html");
    onion_iframe_outer_url_ =
        https_server_.GetURL("foobar.onion", "/iframe_load.html");
  }

  void HandleRequest(const net::test_server::HttpRequest& request) {
    base::AutoLock auto_lock(last_headers_lock_);

    auto referrer_it = request.headers.find("Referer");
    if (referrer_it == request.headers.end()) {
      last_referrer_[request.GetURL()] = "";
    } else {
      last_referrer_[request.GetURL()] = referrer_it->second;
    }

    auto origin_it = request.headers.find("Origin");
    if (origin_it == request.headers.end()) {
      last_origin_[request.GetURL()] = "";
    } else {
      last_origin_[request.GetURL()] = origin_it->second;
    }
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  GURL url(const GURL& destination_url, const GURL& navigation_url) {
    std::string encoded_destination;
    base::Base64UrlEncode(destination_url.spec(),
                          base::Base64UrlEncodePolicy::OMIT_PADDING,
                          &encoded_destination);
    const std::string query =
        base::StringPrintf("url=%s", encoded_destination.c_str());
    GURL::Replacements replacement;
    replacement.SetQueryStr(query);
    return navigation_url.ReplaceComponents(replacement);
  }

  GURL landing_url(const std::string_view query, const GURL& landing_url) {
    GURL::Replacements replacement;
    if (!query.empty()) {
      replacement.SetQueryStr(query);
    }
    return landing_url.ReplaceComponents(replacement);
  }

  const GURL& redirect_to_cross_site_landing_url() {
    return redirect_to_cross_site_landing_url_;
  }
  const GURL& redirect_to_same_site_landing_url() {
    return redirect_to_same_site_landing_url_;
  }
  const GURL& simple_landing_url() { return simple_landing_url_; }

  const GURL& cross_site_url() { return cross_site_url_; }
  const GURL& cross_site_post_url() { return cross_site_post_url_; }
  const GURL& redirect_to_cross_site_url() {
    return redirect_to_cross_site_url_;
  }
  const GURL& same_site_url() { return same_site_url_; }

  const GURL& onion_url() { return onion_url_; }
  const GURL& onion_post_url() { return onion_post_url_; }
  const GURL& reflect_referrer_cross_origin_url() {
    return reflect_referrer_cross_origin_url_;
  }
  const GURL& reflect_referrer_cross_origin_redirect_url() {
    return reflect_referrer_cross_origin_redirect_url_;
  }
  const GURL& reflect_referrer_same_origin_url() {
    return reflect_referrer_same_origin_url_;
  }
  const GURL& reflect_referrer_same_origin_redirect_url() {
    return reflect_referrer_same_origin_redirect_url_;
  }

  const GURL& images_url() { return images_url_; }
  GURL image_url(const std::string& number) {
    GURL::Replacements replacements;
    replacements.SetPathStr("/logo-referrer.png");
    replacements.SetQueryStr(number);
    return images_url().ReplaceComponents(replacements);
  }

  const GURL& iframe_inner_url() { return iframe_inner_url_; }
  const GURL& iframe_outer_url() { return iframe_outer_url_; }
  const GURL& onion_iframe_inner_url() { return onion_iframe_inner_url_; }
  const GURL& onion_iframe_outer_url() { return onion_iframe_outer_url_; }

  const std::string& last_referrer(const GURL& url) {
    base::AutoLock auto_lock(last_headers_lock_);
    GURL::Replacements replacements;
    replacements.SetHostStr("127.0.0.1");
    const GURL internal_url = url.ReplaceComponents(replacements);
    return last_referrer_[internal_url];
  }

  const std::string& last_origin(const GURL& url) {
    base::AutoLock auto_lock(last_headers_lock_);
    GURL::Replacements replacements;
    replacements.SetHostStr("127.0.0.1");
    const GURL internal_url = url.ReplaceComponents(replacements);
    return last_origin_[internal_url];
  }

  content::WebContents* contents(Browser* browser) {
    return browser->tab_strip_model()->GetActiveWebContents();
  }

  void NavigateToURLAndWaitForRedirects(Browser* browser,
                                        const GURL& original_url,
                                        const GURL& landing_url) {
    ui_test_utils::UrlLoadObserver load_complete(landing_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser, original_url));
    load_complete.Wait();
    EXPECT_EQ(contents(browser)->GetLastCommittedURL(), landing_url);
  }

 private:
  GURL cross_site_url_;
  GURL cross_site_post_url_;
  GURL redirect_to_cross_site_landing_url_;
  GURL redirect_to_cross_site_url_;
  GURL redirect_to_same_site_landing_url_;
  GURL same_site_url_;
  GURL simple_landing_url_;

  GURL onion_url_;
  GURL onion_post_url_;
  GURL reflect_referrer_cross_origin_url_;
  GURL reflect_referrer_cross_origin_redirect_url_;
  GURL reflect_referrer_same_origin_url_;
  GURL reflect_referrer_same_origin_redirect_url_;
  GURL images_url_;
  GURL iframe_inner_url_;
  GURL iframe_outer_url_;
  GURL onion_iframe_inner_url_;
  GURL onion_iframe_outer_url_;
  std::map<GURL, std::string> last_referrer_;
  std::map<GURL, std::string> last_origin_;
  mutable base::Lock last_headers_lock_;

  base::FilePath test_data_dir_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveSiteHacksNetworkDelegateBrowserTest,
                       QueryStringFilterCrossSite) {
  const std::string inputs[] = {"",
                                "foo=bar",
                                "fbclid=1",
                                "fbclid=2&key=value",
                                "key=value&fbclid=3",
                                "mkt_tok=xyz&foo=bar",
                                "mkt_tok=xyz&foo=bar&mkt_unsubscribe=1"};
  const std::string outputs[] = {
      // URLs without trackers should be untouched.
      "",
      "foo=bar",
      // URLs with trackers should have those removed.
      "",
      "key=value",
      "key=value",
      // URLs with conditional trackers should have those removed
      // only at the right time.
      "foo=bar",
      "mkt_tok=xyz&foo=bar&mkt_unsubscribe=1",
  };

  constexpr size_t input_count = std::size(inputs);
  static_assert(input_count == std::size(outputs),
                "Inputs and outputs must have the same number of elements.");

  for (size_t i = 0; i < input_count; i++) {
    NavigateToURLAndWaitForRedirects(
        browser(),
        url(landing_url(inputs[i], simple_landing_url()), cross_site_url()),
        landing_url(outputs[i], simple_landing_url()));
  }
}

IN_PROC_BROWSER_TEST_F(BraveSiteHacksNetworkDelegateBrowserTest,
                       QueryStringCrossSitePost) {
  NavigateToURLAndWaitForRedirects(
      browser(),
      url(landing_url("fbclid=1", simple_landing_url()), cross_site_post_url()),
      landing_url("fbclid=1", simple_landing_url()));
}

IN_PROC_BROWSER_TEST_F(BraveSiteHacksNetworkDelegateBrowserTest,
                       QueryStringFilterShieldsDown) {
  const std::string inputs[] = {
      "", "foo=bar", "fbclid=1", "fbclid=2&key=value", "key=value&fbclid=3",
  };

  for (const auto& input : inputs) {
    const GURL dest_url = landing_url(input, simple_landing_url());
    brave_shields::SetBraveShieldsEnabled(content_settings(), false, dest_url);
    NavigateToURLAndWaitForRedirects(browser(), url(dest_url, cross_site_url()),
                                     dest_url);
  }
}

IN_PROC_BROWSER_TEST_F(BraveSiteHacksNetworkDelegateBrowserTest,
                       QueryStringFilterSameSite) {
  const std::string inputs[] = {
      "fbclid=1",
      "fbclid=2&key=value",
      "key=value&fbclid=3",
  };
  // Same-site requests should be untouched.

  for (const auto& input : inputs) {
    NavigateToURLAndWaitForRedirects(
        browser(),
        url(landing_url(input, simple_landing_url()), same_site_url()),
        landing_url(input, simple_landing_url()));
  }
}

IN_PROC_BROWSER_TEST_F(BraveSiteHacksNetworkDelegateBrowserTest,
                       QueryStringFilterCrossSiteRedirect) {
  const std::string inputs[] = {
      "",
      "fbclid=1",
  };
  const std::string outputs[] = {
      // URLs without trackers should be untouched.
      "",
      // URLs with trackers should have those removed.
      "",
  };

  constexpr size_t input_count = sizeof(inputs) / sizeof(std::string);
  static_assert(input_count == sizeof(outputs) / sizeof(std::string),
                "Inputs and outputs must have the same number of elements.");

  for (size_t i = 0; i < input_count; i++) {
    // Same-site navigations to a cross-site redirect go through the query
    // filter.
    NavigateToURLAndWaitForRedirects(
        browser(),
        url(landing_url(inputs[i], redirect_to_cross_site_landing_url()),
            same_site_url()),
        landing_url(outputs[i], simple_landing_url()));
  }
}

IN_PROC_BROWSER_TEST_F(BraveSiteHacksNetworkDelegateBrowserTest,
                       QueryStringFilterSameSiteRedirect) {
  const std::string inputs[] = {
      "",
      "fbclid=1",
  };

  for (const auto& input : inputs) {
    // Same-site navigations to a same-site redirect are exempted from the query
    // filter.
    NavigateToURLAndWaitForRedirects(
        browser(),
        url(landing_url(input, redirect_to_same_site_landing_url()),
            same_site_url()),
        landing_url(input, simple_landing_url()));
  }
}

IN_PROC_BROWSER_TEST_F(BraveSiteHacksNetworkDelegateBrowserTest,
                       QueryStringFilterDirectNavigation) {
  const std::string inputs[] = {
      "",
      "abc=1",
      "fbclid=1",
  };
  const std::string outputs[] = {
      // URLs without trackers should be untouched.
      "",
      "abc=1",
      // URLs with trackers should have those removed.
      "",
  };

  constexpr size_t input_count = std::size(inputs);
  static_assert(input_count == std::size(outputs),
                "Inputs and outputs must have the same number of elements.");

  for (size_t i = 0; i < input_count; i++) {
    // Direct navigations go through the query filter.
    GURL input = landing_url(inputs[i], simple_landing_url());
    GURL output = landing_url(outputs[i], simple_landing_url());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), input));
    EXPECT_EQ(contents(browser())->GetLastCommittedURL(), output);
  }
}

#if BUILDFLAG(ENABLE_TOR)
IN_PROC_BROWSER_TEST_F(BraveSiteHacksNetworkDelegateBrowserTest,
                       OnionReferrers) {
  net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(true);
  tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(true);
  Browser* tor_browser =
      TorProfileManager::SwitchToTorProfile(browser()->profile());

  // Same-origin navigations
  {
    const GURL dest_url = reflect_referrer_same_origin_url();
    const GURL same_origin_test_url = url(dest_url, onion_url());
    NavigateToURLAndWaitForRedirects(tor_browser, same_origin_test_url,
                                     dest_url);
    EXPECT_EQ(last_referrer(dest_url), same_origin_test_url.spec());
    EXPECT_EQ(last_origin(dest_url), "");

    // Redirect
    const GURL intermediate_url = reflect_referrer_same_origin_redirect_url();
    const GURL same_origin_redirect_test_url =
        url(intermediate_url, onion_url());
    NavigateToURLAndWaitForRedirects(tor_browser, same_origin_redirect_test_url,
                                     dest_url);
    EXPECT_EQ(last_referrer(dest_url), same_origin_redirect_test_url.spec());
    EXPECT_EQ(last_origin(dest_url), "");
  }
  {
    // POST
    const GURL dest_url = reflect_referrer_same_origin_url();
    const GURL same_origin_test_url = url(dest_url, onion_post_url());
    NavigateToURLAndWaitForRedirects(tor_browser, same_origin_test_url,
                                     dest_url);
    EXPECT_EQ(last_referrer(dest_url), same_origin_test_url.spec());
    std::string full_origin =
        url::Origin::Create(same_origin_test_url).GetURL().spec();
    full_origin.pop_back();  // CORS headers don't use canonical forms.
    EXPECT_EQ(last_origin(dest_url), full_origin);

    // Redirect
    const GURL intermediate_url = reflect_referrer_same_origin_redirect_url();
    const GURL same_origin_redirect_test_url =
        url(intermediate_url, onion_post_url());
    NavigateToURLAndWaitForRedirects(tor_browser, same_origin_redirect_test_url,
                                     dest_url);
    EXPECT_EQ(last_referrer(dest_url), same_origin_redirect_test_url.spec());
    EXPECT_EQ(last_origin(dest_url), full_origin);
  }

  // Cross-origin navigations
  {
    const GURL dest_url = reflect_referrer_cross_origin_url();
    NavigateToURLAndWaitForRedirects(tor_browser, url(dest_url, onion_url()),
                                     dest_url);
    EXPECT_EQ(last_referrer(dest_url), "");
    EXPECT_EQ(last_origin(dest_url), "");

    // Redirect
    const GURL intermediate_url = reflect_referrer_cross_origin_redirect_url();
    NavigateToURLAndWaitForRedirects(
        tor_browser, url(intermediate_url, onion_url()), dest_url);
    EXPECT_EQ(last_referrer(dest_url), "");
    EXPECT_EQ(last_origin(dest_url), "");
  }
  {
    // POST
    const GURL dest_url = reflect_referrer_cross_origin_url();
    NavigateToURLAndWaitForRedirects(tor_browser,
                                     url(dest_url, onion_post_url()), dest_url);
    EXPECT_EQ(last_referrer(dest_url), "");
    EXPECT_EQ(last_origin(dest_url), "null");

    // Redirect
    const GURL intermediate_url = reflect_referrer_cross_origin_redirect_url();
    NavigateToURLAndWaitForRedirects(
        tor_browser, url(intermediate_url, onion_post_url()), dest_url);
    EXPECT_EQ(last_referrer(dest_url), "");
    EXPECT_EQ(last_origin(dest_url), "null");
  }

  NavigateToURLAndWaitForRedirects(tor_browser, images_url(), images_url());

  // Same-origin sub-requests
  std::string full_origin = url::Origin::Create(images_url()).GetURL().spec();
  full_origin.pop_back();  // CORS headers don't use canonical forms.
  EXPECT_EQ(last_referrer(image_url("1")), images_url().spec());
  EXPECT_EQ(last_origin(image_url("1")), "");  // nocors
  EXPECT_EQ(last_referrer(image_url("2")), images_url().spec());
  EXPECT_EQ(last_origin(image_url("2")), full_origin);
  // Redirects
  EXPECT_EQ(last_referrer(image_url("3")), images_url().spec());
  EXPECT_EQ(last_origin(image_url("3")), "");  // nocors
  EXPECT_EQ(last_referrer(image_url("4")), images_url().spec());
  EXPECT_EQ(last_origin(image_url("4")), full_origin);

  // Cross-origin sub-requests
  EXPECT_EQ(last_referrer(image_url("5")), "");
  EXPECT_EQ(last_origin(image_url("5")), "");  // nocors
  EXPECT_EQ(last_referrer(image_url("6")), "");
  EXPECT_EQ(last_origin(image_url("6")), "null");
  // Redirects
  EXPECT_EQ(last_referrer(image_url("7")), "");
  EXPECT_EQ(last_origin(image_url("7")), "");  // nocors
  EXPECT_EQ(last_referrer(image_url("8")), "");
  EXPECT_EQ(last_origin(image_url("8")), "null");
}

IN_PROC_BROWSER_TEST_F(BraveSiteHacksNetworkDelegateBrowserTest,
                       OnionAncestorOrigins) {
  net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(true);
  tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(true);
  Browser* tor_browser =
      TorProfileManager::SwitchToTorProfile(browser()->profile());

  // Same-origin .onion iframes
  {
    const GURL inner_url = onion_iframe_inner_url();
    const GURL same_origin_test_url = url(inner_url, onion_iframe_outer_url());
    NavigateToURLAndWaitForRedirects(tor_browser, same_origin_test_url,
                                     same_origin_test_url);
    EXPECT_EQ(last_referrer(inner_url), same_origin_test_url.spec());

    content::RenderFrameHost* inner_frame =
        ChildFrameAt(contents(tor_browser)->GetPrimaryMainFrame(), 0);
    std::string real_value =
        content::EvalJs(inner_frame, "getAncestors()").ExtractString();
    std::string onion_origin =
        url::Origin::Create(onion_iframe_outer_url()).GetURL().spec();
    onion_origin.pop_back();  // remove trailing slash
    EXPECT_EQ(real_value, "[" + onion_origin + "]");
  }

  // Cross-origin .onion outer iframe
  {
    const GURL inner_url = iframe_inner_url();
    const GURL cross_origin_test_url = url(inner_url, onion_iframe_outer_url());
    NavigateToURLAndWaitForRedirects(tor_browser, cross_origin_test_url,
                                     cross_origin_test_url);
    EXPECT_EQ(last_referrer(inner_url), "");

    content::RenderFrameHost* inner_frame =
        ChildFrameAt(contents(tor_browser)->GetPrimaryMainFrame(), 0);
    std::string real_value =
        content::EvalJs(inner_frame, "getAncestors()").ExtractString();
    EXPECT_EQ(real_value, "[\"null\"]");
  }

  // Cross-origin .onion inner iframe
  {
    const GURL inner_url = onion_iframe_inner_url();
    const GURL cross_origin_test_url = url(inner_url, iframe_outer_url());
    NavigateToURLAndWaitForRedirects(tor_browser, cross_origin_test_url,
                                     cross_origin_test_url);
    std::string iframe_origin =
        url::Origin::Create(iframe_outer_url()).GetURL().spec();
    EXPECT_EQ(last_referrer(inner_url), iframe_origin);

    content::RenderFrameHost* inner_frame =
        ChildFrameAt(contents(tor_browser)->GetPrimaryMainFrame(), 0);
    std::string real_value =
        content::EvalJs(inner_frame, "getAncestors()").ExtractString();
    iframe_origin.pop_back();  // remove trailing slash
    EXPECT_EQ(real_value, "[" + iframe_origin + "]");
  }
}
#endif
