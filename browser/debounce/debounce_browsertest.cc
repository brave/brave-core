/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/base64url.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/content/test/engine_test_observer.h"
#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "brave/components/debounce/core/browser/debounce_component_installer.h"
#include "brave/components/debounce/core/common/features.h"
#include "brave/components/debounce/core/common/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace {
constexpr char kTestDataDirectory[] = "debounce-data";
static base::NoDestructor<std::string> gLastSiteForCookies("");
}  // namespace

using brave_shields::ControlType;
using brave_shields::SetCosmeticFilteringControlType;
using debounce::DebounceComponentInstaller;

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

  raw_ptr<DebounceComponentInstaller> component_installer_;
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
      content::FrameTreeNodeId frame_tree_node_id,
      std::optional<int64_t> navigation_id) override {
    std::vector<std::unique_ptr<blink::URLLoaderThrottle>> throttles =
        BraveContentBrowserClient::CreateURLLoaderThrottles(
            request, browser_context, wc_getter, navigation_ui_data,
            frame_tree_node_id, navigation_id);
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

  void ToggleDebouncePref(bool on) {
    browser()->profile()->GetPrefs()->SetBoolean(
        debounce::prefs::kDebounceEnabled, on);
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

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  bool IsShowingInterstitial() {
    return chrome_browser_interstitials::IsShowingInterstitial(web_contents());
  }

  void NavigateTo(const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    content::RenderFrameHost* frame = web_contents()->GetPrimaryMainFrame();
    ASSERT_TRUE(WaitForRenderFrameReady(frame));
  }

  void NavigateToURLAndWaitForRedirects(const GURL& original_url,
                                        const GURL& landing_url) {
    ui_test_utils::UrlLoadObserver load_complete(landing_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), original_url));
    load_complete.Wait();
    EXPECT_EQ(web_contents()->GetLastCommittedURL(), landing_url);
  }

  void InitAdBlockForDebounce() {
    auto source_provider =
        std::make_unique<brave_shields::TestFiltersProvider>("||blocked.com^");
    g_brave_browser_process->ad_block_service()->UseSourceProviderForTest(
        source_provider.get());
    source_providers_.push_back(std::move(source_provider));
    auto* engine =
        g_brave_browser_process->ad_block_service()->default_engine_.get();
    EngineTestObserver engine_observer(engine);
    engine_observer.Wait();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::vector<std::unique_ptr<brave_shields::TestFiltersProvider>>
      source_providers_;
};

// Test simple redirection by query parameter.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, Redirect) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
  GURL base_url = embedded_test_server()->GetURL("simple.a.com", "/");
  GURL landing_url = embedded_test_server()->GetURL("simple.b.com", "/");
  GURL original_url = add_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

// Check that URLs ending with a '.' are properly debounced.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, URLThatEndsWithADot) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
  GURL base_url = embedded_test_server()->GetURL("simple.a.com.", "/");
  GURL landing_url = embedded_test_server()->GetURL("simple.b.com", "/");
  GURL original_url = add_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

// Test with pref off
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, DisabledIfPrefOff) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(false);
  web_contents()->GetController().Reload(content::ReloadType::NORMAL, false);
  GURL base_url = embedded_test_server()->GetURL("simple.a.com", "/");
  GURL landing_url = embedded_test_server()->GetURL("simple.b.com", "/");
  GURL original_url = add_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, original_url);
}

IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, BackForward) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);

  // starting page for back/foward
  GURL start_url = embedded_test_server()->GetURL("z.com", "/");
  NavigateToURLAndWaitForRedirects(start_url, start_url);

  // debounce
  GURL base_url = embedded_test_server()->GetURL("simple.a.com", "/");
  GURL landing_url = embedded_test_server()->GetURL("simple.b.com", "/");
  GURL original_url = add_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);

  // back
  web_contents()->GetController().GoBack();
  WaitForLoadStop(web_contents());
  EXPECT_EQ(web_contents()->GetLastCommittedURL(), start_url);

  // forward
  web_contents()->GetController().GoForward();
  WaitForLoadStop(web_contents());
  EXPECT_EQ(web_contents()->GetLastCommittedURL(), landing_url);
}

// Test base64-encoded redirection by query parameter.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, Base64Redirect) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
  GURL base_url = embedded_test_server()->GetURL("base64.a.com", "/");
  GURL landing_url = embedded_test_server()->GetURL("base64.b.com", "/");
  GURL original_url = add_base64_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, DoubleRedirect) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
  GURL url_z = embedded_test_server()->GetURL("z.com", "/");
  GURL url_b = add_redirect_param(
      embedded_test_server()->GetURL("double.b.com", "/"), url_z);
  GURL url_a = add_redirect_param(
      embedded_test_server()->GetURL("double.a.com", "/"), url_b);
  NavigateToURLAndWaitForRedirects(url_a, url_z);

  url_z = embedded_test_server()->GetURL("z.com", "/");
  url_b = add_redirect_param(
      embedded_test_server()->GetURL("double.a.com", "/"), url_z);
  url_a = add_redirect_param(
      embedded_test_server()->GetURL("double.b.com", "/"), url_b);
  NavigateToURLAndWaitForRedirects(url_a, url_z);
}

// Test a long redirect chain.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, QuadRedirect) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
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
// tracker's domain. This should NOT be debounced, because the tracker and
// the final URL share an eTLD+1.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, SameSiteTracker) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
  GURL final_url = embedded_test_server()->GetURL("z.com", "/");
  GURL intermediate_url = add_redirect_param(
      embedded_test_server()->GetURL("tracker.z.com", "/"), final_url);
  GURL start_url = add_redirect_param(
      embedded_test_server()->GetURL("origin.h.com", "/"), intermediate_url);
  NavigateToURLAndWaitForRedirects(start_url, intermediate_url);
}

// Test a long redirect chain that bounces through the original URL's domain,
// and verify the SiteForCookies used for the debounced request.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, RedirectThroughOriginalSite) {
  SpyContentBrowserClient browser_client;
  auto* old_client = content::SetBrowserClientForTesting(&browser_client);

  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
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

IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, RedirectLoop) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
  GURL finish_url = embedded_test_server()->GetURL("double.a.com", "/");
  GURL loop_url = add_redirect_param(
      embedded_test_server()->GetURL("double.b.com", "/"), finish_url);
  GURL start_url = add_redirect_param(
      embedded_test_server()->GetURL("double.b.com", "/"),
      add_redirect_param(embedded_test_server()->GetURL("double.a.com", "/"),
                         loop_url));
  NavigateToURLAndWaitForRedirects(start_url, loop_url);
}

// Test wildcard URL patterns by constructing a URL that should be
// debounced because it matches a wildcard include pattern.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, WildcardInclude) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
  GURL landing_url = embedded_test_server()->GetURL("z.com", "/");
  GURL start_url = add_redirect_param(
      embedded_test_server()->GetURL("included.c.com", "/"), landing_url);
  NavigateToURLAndWaitForRedirects(start_url, landing_url);
}

// Test that unknown actions are ignored.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, UnknownAction) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
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
  ToggleDebouncePref(true);
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
  ToggleDebouncePref(true);
  GURL landing_url = embedded_test_server()->GetURL("z.com", "/");
  GURL start_url = add_redirect_param(
      embedded_test_server()->GetURL("included.f.com", "/"), landing_url);
  NavigateToURLAndWaitForRedirects(start_url, start_url);
}

// Test that extra keys in a rule are ignored and the rule is still
// processed and applied.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, IgnoreExtraKeys) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
  GURL base_url = embedded_test_server()->GetURL("simple.g.com", "/");
  GURL landing_url = embedded_test_server()->GetURL("z.com", "/");
  GURL original_url = add_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

// Test that URLs in private registries are treated the same as all other URLs.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, ExcludePrivateRegistries) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
  GURL base_url = embedded_test_server()->GetURL("example.blogspot.com", "/");
  GURL landing_url = embedded_test_server()->GetURL("z.com", "/");
  GURL original_url = add_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

// Test that debouncing rule is skipped if the hostname of the new url as
// extracted via our simple parser doesn't match the host as parsed via GURL
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, IgnoreHostnameMismatch) {
  ASSERT_TRUE(InstallMockExtension());
  ToggleDebouncePref(true);
  // The destination decodes to http://evil.com\\@apps.apple.com
  // If you paste that in Chrome or Brave, the backslashes are changed
  // to slashes and you end up on http://evil.com//@apps.apple.com
  GURL original_url = embedded_test_server()->GetURL(
      "simple.a.com", "/?url=http%3A%2F%2Fevil.com%5C%5C%40apps.apple.com");
  NavigateToURLAndWaitForRedirects(original_url, original_url);
}

// Test that debounceable URLs on domain block list are debounced instead.
IN_PROC_BROWSER_TEST_F(DebounceBrowserTest, DebounceBeforeDomainBlock) {
  GURL base_url = embedded_test_server()->GetURL("blocked.com", "/");
  GURL landing_url = embedded_test_server()->GetURL("debounced.com", "/");
  GURL original_url = add_redirect_param(base_url, landing_url);

  // Install adblock, turn on aggressive blocking for this URL, then attempt to
  // navigate to it. This should be interrupted by the domain block
  // interstitial.
  InitAdBlockForDebounce();
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK,
                                  original_url);
  NavigateTo(original_url);
  ASSERT_TRUE(IsShowingInterstitial());

  // Now install debounce and navigate to the same URL. This should debounce the
  // URL without showing the domain block interstitial.
  ASSERT_TRUE(InstallMockExtension());
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
  ASSERT_FALSE(IsShowingInterstitial());
}
