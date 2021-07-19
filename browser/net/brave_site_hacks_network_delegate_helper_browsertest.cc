/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/base64url.h"
#include "base/cxx17_backports.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"

class BraveSiteHacksNetworkDelegateBrowserTest : public InProcessBrowserTest {
 public:
  BraveSiteHacksNetworkDelegateBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    brave::RegisterPathProvider();
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir_);
    https_server_.ServeFilesFromDirectory(test_data_dir_);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    content::SetupCrossSiteRedirector(&https_server_);

    ASSERT_TRUE(https_server_.Start());

    simple_landing_url_ = https_server_.GetURL("a.com", "/simple.html");
    redirect_to_cross_site_landing_url_ =
        https_server_.GetURL("redir.b.com", "/cross-site/a.com/simple.html");
    redirect_to_same_site_landing_url_ =
        https_server_.GetURL("redir.a.com", "/cross-site/a.com/simple.html");

    cross_site_url_ = https_server_.GetURL("b.com", "/navigate-to-site.html");
    same_site_url_ =
        https_server_.GetURL("sub.a.com", "/navigate-to-site.html");
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    // This is needed to load pages from "domain.com" without an interstitial.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  const net::EmbeddedTestServer& https_server() { return https_server_; }

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

  GURL landing_url(const base::StringPiece& query, const GURL& landing_url) {
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
  const GURL& redirect_to_cross_site_url() {
    return redirect_to_cross_site_url_;
  }
  const GURL& same_site_url() { return same_site_url_; }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void NavigateToURLAndWaitForRedirects(const GURL& original_url,
                                        const GURL& landing_url) {
    ui_test_utils::UrlLoadObserver load_complete(
        landing_url, content::NotificationService::AllSources());
    ui_test_utils::NavigateToURL(browser(), original_url);
    EXPECT_EQ(contents()->GetMainFrame()->GetLastCommittedURL(), original_url);
    load_complete.Wait();

    EXPECT_EQ(contents()->GetLastCommittedURL(), landing_url);
  }

 private:
  GURL cross_site_url_;
  GURL redirect_to_cross_site_landing_url_;
  GURL redirect_to_cross_site_url_;
  GURL redirect_to_same_site_landing_url_;
  GURL same_site_url_;
  GURL simple_landing_url_;
  base::FilePath test_data_dir_;

  net::test_server::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveSiteHacksNetworkDelegateBrowserTest,
                       QueryStringFilterCrossSite) {
  const std::string inputs[] = {
      "", "foo=bar", "fbclid=1", "fbclid=2&key=value", "key=value&fbclid=3",
  };
  const std::string outputs[] = {
      // URLs without trackers should be untouched.
      "",
      "foo=bar",
      // URLs with trackers should have those removed.
      "",
      "key=value",
      "key=value",
  };

  constexpr size_t input_count = base::size(inputs);
  static_assert(input_count == base::size(outputs),
                "Inputs and outputs must have the same number of elements.");

  for (size_t i = 0; i < input_count; i++) {
    NavigateToURLAndWaitForRedirects(
        url(landing_url(inputs[i], simple_landing_url()), cross_site_url()),
        landing_url(outputs[i], simple_landing_url()));
  }
}

IN_PROC_BROWSER_TEST_F(BraveSiteHacksNetworkDelegateBrowserTest,
                       QueryStringFilterShieldsDown) {
  const std::string inputs[] = {
      "", "foo=bar", "fbclid=1", "fbclid=2&key=value", "key=value&fbclid=3",
  };

  constexpr size_t input_count = base::size(inputs);

  for (size_t i = 0; i < input_count; i++) {
    const GURL dest_url = landing_url(inputs[i], simple_landing_url());
    brave_shields::SetBraveShieldsEnabled(content_settings(), false, dest_url);
    NavigateToURLAndWaitForRedirects(url(dest_url, cross_site_url()), dest_url);
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

  constexpr size_t input_count = sizeof(inputs) / sizeof(std::string);

  for (size_t i = 0; i < input_count; i++) {
    NavigateToURLAndWaitForRedirects(
        url(landing_url(inputs[i], simple_landing_url()), same_site_url()),
        landing_url(inputs[i], simple_landing_url()));
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

  constexpr size_t input_count = sizeof(inputs) / sizeof(std::string);

  for (size_t i = 0; i < input_count; i++) {
    // Same-site navigations to a same-site redirect are exempted from the query
    // filter.
    NavigateToURLAndWaitForRedirects(
        url(landing_url(inputs[i], redirect_to_same_site_landing_url()),
            same_site_url()),
        landing_url(inputs[i], simple_landing_url()));
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

  constexpr size_t input_count = base::size(inputs);
  static_assert(input_count == base::size(outputs),
                "Inputs and outputs must have the same number of elements.");

  for (size_t i = 0; i < input_count; i++) {
    // Direct navigations go through the query filter.
    GURL input = landing_url(inputs[i], simple_landing_url());
    GURL output = landing_url(outputs[i], simple_landing_url());
    ui_test_utils::NavigateToURL(browser(), input);
    EXPECT_EQ(contents()->GetLastCommittedURL(), output);
  }
}
