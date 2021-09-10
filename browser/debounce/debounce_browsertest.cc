/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/base64url.h"
#include "base/path_service.h"
#include "base/scoped_observation.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/debounce/browser/debounce_component_installer.h"
#include "brave/components/debounce/common/features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/base/url_util.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace {
const char kTestDataDirectory[] = "debounce-data";
static base::NoDestructor<std::string> gLastSiteForCookies("");
}  // namespace

namespace debounce {

class DebounceComponentInstallerWaiter
    : public DebounceComponentInstaller::Observer {
 public:
  explicit DebounceComponentInstallerWaiter(
      DebounceComponentInstaller* component_installer)
      : component_installer_(component_installer), scoped_observer_(this) {
    scoped_observer_.Observe(component_installer_);
  }
  DebounceComponentInstallerWaiter(const DebounceComponentInstallerWaiter&) =
      delete;
  DebounceComponentInstallerWaiter& operator=(
      const DebounceComponentInstallerWaiter&) = delete;
  ~DebounceComponentInstallerWaiter() override = default;

  void Wait() { run_loop_.Run(); }

 private:
  // DebounceComponentInstaller::Observer:
  void OnRulesReady(DebounceComponentInstaller* component_installer) override {
    run_loop_.QuitWhenIdle();
  }

  DebounceComponentInstaller* const component_installer_;
  base::RunLoop run_loop_;
  base::ScopedObservation<DebounceComponentInstaller,
                          DebounceComponentInstaller::Observer>
      scoped_observer_{this};
};

// A delegate to spy on requests before they are sent
class SpyThrottle : public blink::URLLoaderThrottle {
 public:
  SpyThrottle() = default;
  ~SpyThrottle() override = default;

  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override {
    *gLastSiteForCookies = request->site_for_cookies.site().Serialize();
  }
};

class SpyContentBrowserClient : public BraveContentBrowserClient {
 public:
  SpyContentBrowserClient() = default;
  ~SpyContentBrowserClient() override = default;

  // ContentBrowserClient overrides:
  std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
  CreateURLLoaderThrottles(
      const network::ResourceRequest& request,
      content::BrowserContext* browser_context,
      const base::RepeatingCallback<content::WebContents*()>& wc_getter,
      content::NavigationUIData* navigation_ui_data,
      int frame_tree_node_id) override {
    std::vector<std::unique_ptr<blink::URLLoaderThrottle>> throttles =
        BraveContentBrowserClient::CreateURLLoaderThrottles(
            request, browser_context, wc_getter, navigation_ui_data,
            frame_tree_node_id);
    throttles.push_back(std::make_unique<SpyThrottle>());
    return throttles;
  }
};

class DebounceBrowserTest : public BaseLocalDataFilesBrowserTest {
 public:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(
        debounce::features::kBraveDebounce);
    BaseLocalDataFilesBrowserTest::SetUp();
  }

  // BaseLocalDataFilesBrowserTest overrides
  const char* test_data_directory() override { return kTestDataDirectory; }
  const char* embedded_test_server_directory() override { return ""; }
  LocalDataFilesObserver* service() override {
    return g_brave_browser_process->debounce_component_installer();
  }

  void WaitForService() override {
    // Wait for debounce download service to load and parse its
    // configuration file.
    debounce::DebounceComponentInstaller* component_installer =
        g_brave_browser_process->debounce_component_installer();
    DebounceComponentInstallerWaiter(component_installer).Wait();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  GURL add_redirect_param(const GURL& original_url, const GURL& landing_url) {
    return net::AppendOrReplaceQueryParameter(original_url, "url",
                                              landing_url.spec());
  }

  GURL add_base64_redirect_param(const GURL& original_url,
                                 const GURL& landing_url) {
    std::string encoded_destination;
    base::Base64UrlEncode(landing_url.spec(),
                          base::Base64UrlEncodePolicy::OMIT_PADDING,
                          &encoded_destination);
    const std::string query =
        base::StringPrintf("url=%s", encoded_destination.c_str());
    GURL::Replacements replacement;
    replacement.SetQueryStr(query);
    return original_url.ReplaceComponents(replacement);
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void NavigateToURLAndWaitForRedirects(const GURL& original_url,
                                        const GURL& landing_url) {
    ui_test_utils::UrlLoadObserver load_complete(
        landing_url, content::NotificationService::AllSources());
    ui_test_utils::NavigateToURL(browser(), original_url);
    load_complete.Wait();
    EXPECT_EQ(web_contents()->GetLastCommittedURL(), landing_url);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Test simple redirection by query parameter.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, Redirect) {
  ASSERT_TRUE(InstallMockExtension());
  GURL base_url = embedded_test_server()->GetURL("simple.a.com", "/");
  GURL landing_url = embedded_test_server()->GetURL("simple.b.com", "/");
  GURL original_url = add_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

// Test base64-encoded redirection by query parameter.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, Base64Redirect) {
  ASSERT_TRUE(InstallMockExtension());
  GURL base_url = embedded_test_server()->GetURL("base64.a.com", "/");
  GURL landing_url = embedded_test_server()->GetURL("base64.b.com", "/");
  GURL original_url = add_base64_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

// Test that debounce rules continue to be processed in order
// by constructing a URL that should be debounced to a second
// URL that should then be debounced to a third URL.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, DoubleRedirect) {
  ASSERT_TRUE(InstallMockExtension());
  GURL url_z = embedded_test_server()->GetURL("z.com", "/");
  GURL url_b = add_redirect_param(
      embedded_test_server()->GetURL("double.b.com", "/"), url_z);
  GURL url_a = add_redirect_param(
      embedded_test_server()->GetURL("double.a.com", "/"), url_b);
  NavigateToURLAndWaitForRedirects(url_a, url_z);
}

// Test a long redirect chain.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, QuadRedirect) {
  ASSERT_TRUE(InstallMockExtension());
  GURL url_z = embedded_test_server()->GetURL("z.com", "/");
  GURL url_d = add_redirect_param(
      embedded_test_server()->GetURL("quad.d.com", "/"), url_z);
  GURL url_c = add_redirect_param(
      embedded_test_server()->GetURL("quad.c.com", "/"), url_d);
  GURL url_b = add_redirect_param(
      embedded_test_server()->GetURL("quad.b.com", "/"), url_c);
  GURL url_a = add_redirect_param(
      embedded_test_server()->GetURL("quad.a.com", "/"), url_b);
  NavigateToURLAndWaitForRedirects(url_a, url_z);
}

// Test a redirect chain that bounces from a tracker to a final URL in the
// tracker's domain.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, SameSiteTracker) {
  ASSERT_TRUE(InstallMockExtension());
  GURL final_url = embedded_test_server()->GetURL("z.com", "/");
  GURL intermediate_url = add_redirect_param(
      embedded_test_server()->GetURL("tracker.z.com", "/"), final_url);
  GURL start_url = add_redirect_param(
      embedded_test_server()->GetURL("origin.h.com", "/"), intermediate_url);
  NavigateToURLAndWaitForRedirects(start_url, final_url);
}

// Test a long redirect chain that bounces through the original URL's domain,
// and verify the SiteForCookies used for the debounced request.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, RedirectThroughOriginalSite) {
  SpyContentBrowserClient browser_client;
  auto* old_client = content::SetBrowserClientForTesting(&browser_client);

  ASSERT_TRUE(InstallMockExtension());
  GURL url_z = embedded_test_server()->GetURL("z.com", "/");
  GURL url_tracker_a = add_redirect_param(
      embedded_test_server()->GetURL("tracker.a.com", "/"), url_z);
  GURL url_d = add_redirect_param(
      embedded_test_server()->GetURL("quad.d.com", "/"), url_tracker_a);
  GURL url_c = add_redirect_param(
      embedded_test_server()->GetURL("quad.c.com", "/"), url_d);
  GURL url_b = add_redirect_param(
      embedded_test_server()->GetURL("quad.b.com", "/"), url_c);
  GURL url_a = add_redirect_param(
      embedded_test_server()->GetURL("quad.a.com", "/"), url_b);
  NavigateToURLAndWaitForRedirects(url_a, url_z);
  EXPECT_EQ(*gLastSiteForCookies, "http://z.com");

  content::SetBrowserClientForTesting(old_client);
}

// Test that debounce rules are not processed twice by constructing
// a URL that should be debounced to a second URL that would be
// debounced to a third URL except that that rule has already been
// processed, so it won't.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, NotDoubleRedirect) {
  ASSERT_TRUE(InstallMockExtension());
  GURL final_url = embedded_test_server()->GetURL("z.com", "/");
  GURL intermediate_url = add_redirect_param(
      embedded_test_server()->GetURL("double.a.com", "/"), final_url);
  GURL start_url = add_redirect_param(
      embedded_test_server()->GetURL("double.b.com", "/"), intermediate_url);
  NavigateToURLAndWaitForRedirects(start_url, intermediate_url);
}

// Test wildcard URL patterns by constructing a URL that should be
// debounced because it matches a wildcard include pattern.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, WildcardInclude) {
  ASSERT_TRUE(InstallMockExtension());
  GURL landing_url = embedded_test_server()->GetURL("z.com", "/");
  GURL start_url = add_redirect_param(
      embedded_test_server()->GetURL("included.c.com", "/"), landing_url);
  NavigateToURLAndWaitForRedirects(start_url, landing_url);
}

// Test that unknown actions are ignored.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, UnknownAction) {
  ASSERT_TRUE(InstallMockExtension());
  GURL landing_url = embedded_test_server()->GetURL("z.com", "/");
  GURL start_url = add_redirect_param(
      embedded_test_server()->GetURL("included.d.com", "/"), landing_url);
  NavigateToURLAndWaitForRedirects(start_url, start_url);
}

// Test URL exclude patterns by constructing a URL that should be debounced
// because it matches a wildcard include pattern, then a second one
// that should not be debounced because it matches an exclude pattern.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, ExcludeOverridesWildcardInclude) {
  ASSERT_TRUE(InstallMockExtension());
  GURL landing_url = embedded_test_server()->GetURL("z.com", "/");
  GURL start_url_1 = add_redirect_param(
      embedded_test_server()->GetURL("included.e.com", "/"), landing_url);
  NavigateToURLAndWaitForRedirects(start_url_1, landing_url);
  GURL start_url_2 = add_redirect_param(
      embedded_test_server()->GetURL("excluded.e.com", "/"), landing_url);
  NavigateToURLAndWaitForRedirects(start_url_2, start_url_2);
}

// Test that debouncing rules only apply if the query parameter matches
// exactly.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, NoParamMatch) {
  ASSERT_TRUE(InstallMockExtension());
  GURL landing_url = embedded_test_server()->GetURL("z.com", "/");
  GURL start_url = add_redirect_param(
      embedded_test_server()->GetURL("included.f.com", "/"), landing_url);
  NavigateToURLAndWaitForRedirects(start_url, start_url);
}

// Test that extra keys in a rule are ignored and the rule is still
// processed and applied.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, IgnoreExtraKeys) {
  ASSERT_TRUE(InstallMockExtension());
  GURL base_url = embedded_test_server()->GetURL("simple.g.com", "/");
  GURL landing_url = embedded_test_server()->GetURL("z.com", "/");
  GURL original_url = add_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

}  // namespace debounce
