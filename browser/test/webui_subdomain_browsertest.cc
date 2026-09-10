/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/strings/strcat.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/sessions/session_restore_test_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/security_principal.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/webui_config.h"
#include "content/public/browser/webui_config_map.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/scoped_web_ui_controller_factory_registration.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/web_ui_browsertest_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/window_open_disposition.h"
#include "ui/webui/untrusted_web_ui_controller.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

// A chrome-untrusted:// host whose config opts into serving its subdomains.
constexpr char kSubdomainsHost[] = "subdomains-test";
// A chrome-untrusted:// host whose config keeps the default (no subdomains).
constexpr char kNoSubdomainsHost[] = "no-subdomains-test";

// content/test/data/title2.html, which has a well known title.
constexpr char kTestPage[] = "title2.html";
constexpr char16_t kTestPageTitle[] = u"Title Of Awesomeness";

// Serves `content/test/data` for the origin that was actually requested.
// Untrusted data sources are keyed by origin, so a config serving subdomains
// needs one data source per subdomain, not one for the parent host.
class TestUntrustedController : public ui::UntrustedWebUIController {
 public:
  TestUntrustedController(content::WebUI* web_ui, const GURL& url)
      : ui::UntrustedWebUIController(web_ui) {
    content::AddUntrustedDataSource(
        web_ui->GetWebContents()->GetBrowserContext(), std::string(url.host()));
  }
  ~TestUntrustedController() override = default;
};

class TestUntrustedConfig : public content::WebUIConfig {
 public:
  TestUntrustedConfig(std::string_view host, bool should_handle_subdomains)
      : content::WebUIConfig(content::kChromeUIUntrustedScheme, host),
        should_handle_subdomains_(should_handle_subdomains) {}
  ~TestUntrustedConfig() override = default;

  // content::WebUIConfig:
  bool ShouldHandleSubdomains() const override {
    return should_handle_subdomains_;
  }

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override {
    controller_urls_.push_back(url);
    return std::make_unique<TestUntrustedController>(web_ui, url);
  }

  // The URLs this config was asked to create a controller for, in order.
  const std::vector<GURL>& controller_urls() const { return controller_urls_; }

 private:
  const bool should_handle_subdomains_;
  std::vector<GURL> controller_urls_;
};

GURL PageURL(std::string_view host) {
  return content::GetChromeUntrustedUIURL(base::StrCat({host, "/", kTestPage}));
}

GURL PageURL(std::string_view subdomain, std::string_view host) {
  return PageURL(base::StrCat({subdomain, ".", host}));
}

constexpr char kLocalStorage[] = "localStorage";
constexpr char kSessionStorage[] = "sessionStorage";

// `EvalJs()` results can't be compared against `nullptr`, so the storage
// helpers below stringify their result and a missing key reads back as this.
constexpr char kUnset[] = "null";

std::string SetStorage(std::string_view storage, std::string_view value) {
  return base::StrCat({storage, ".setItem('key', ",
                       content::JsReplace("$1", value), "), 'ok'"});
}

std::string GetStorage(std::string_view storage) {
  return base::StrCat({"String(", storage, ".getItem('key'))"});
}

}  // namespace

// Tests the end-to-end behaviour of chrome-untrusted:// WebUIs whose config
// opts into serving subdomains via `WebUIConfig::ShouldHandleSubdomains()`.
// Each subdomain must behave as a completely ordinary, separate origin.
class WebUISubdomainBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    auto subdomains_config = std::make_unique<TestUntrustedConfig>(
        kSubdomainsHost, /*should_handle_subdomains=*/true);
    subdomains_config_ = subdomains_config.get();
    subdomains_registration_ =
        std::make_unique<content::ScopedWebUIConfigRegistration>(
            std::move(subdomains_config));

    no_subdomains_registration_ =
        std::make_unique<content::ScopedWebUIConfigRegistration>(
            std::make_unique<TestUntrustedConfig>(
                kNoSubdomainsHost, /*should_handle_subdomains=*/false));
  }

  void TearDownOnMainThread() override {
    subdomains_config_ = nullptr;
    no_subdomains_registration_.reset();
    subdomains_registration_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

 protected:
  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  // Navigates the active tab and asserts the page at `url` actually loaded.
  content::WebContents* NavigateActiveTab(const GURL& url) {
    content::WebContents* web_contents = GetActiveWebContents();
    content::TestNavigationObserver observer(web_contents);
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    observer.Wait();
    EXPECT_TRUE(observer.last_navigation_succeeded())
        << "navigating to " << url;
    EXPECT_EQ(url, web_contents->GetLastCommittedURL());
    return web_contents;
  }

  // Opens `url` in a new foreground tab and asserts the page actually loaded.
  content::WebContents* NavigateNewTab(const GURL& url) {
    content::TestNavigationObserver observer(url);
    observer.StartWatchingNewWebContents();
    EXPECT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
    observer.Wait();
    EXPECT_TRUE(observer.last_navigation_succeeded())
        << "navigating to " << url;
    content::WebContents* web_contents = GetActiveWebContents();
    EXPECT_EQ(url, web_contents->GetLastCommittedURL());
    return web_contents;
  }

  // Navigates the active tab to `url`, returning whether the page loaded rather
  // than committing an error page. `ui_test_utils::NavigateToURL()` can't be
  // used for this, as it reports success for committed error pages too.
  [[nodiscard]] bool NavigateActiveTabAndGetSuccess(const GURL& url) {
    content::TestNavigationObserver observer(GetActiveWebContents());
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    observer.Wait();
    return observer.last_navigation_succeeded();
  }

  // Session restore defers loading tabs, so force the tab at `index` to load
  // before inspecting its contents.
  content::WebContents* LoadRestoredTabAt(int index) {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetWebContentsAt(index);
    if (web_contents->GetController().NeedsReload()) {
      content::TestNavigationObserver observer(web_contents);
      web_contents->GetController().LoadIfNecessary();
      observer.Wait();
      EXPECT_TRUE(observer.last_navigation_succeeded())
          << "restoring tab " << index;
    } else {
      EXPECT_TRUE(content::WaitForLoadStop(web_contents));
    }
    return web_contents;
  }

  // Closes the browser and restores the last session, leaving `browser()`
  // pointing at the restored browser.
  void CloseBrowserAndRestoreSession() {
    Profile* const profile = GetProfile();
    const ScopedKeepAlive scoped_keep_alive(KeepAliveOrigin::SESSION_RESTORE,
                                            KeepAliveRestartOption::DISABLED);
    ScopedProfileKeepAlive profile_keep_alive(
        profile, ProfileKeepAliveOrigin::kSessionRestore);
    CloseBrowserSynchronously(browser());

    ui_test_utils::BrowserCreatedObserver browser_created_observer;
    SessionRestoreTestHelper session_restore_test_helper;
    chrome::OpenWindowWithRestoredTabs(profile);
    if (SessionRestore::IsRestoring(profile)) {
      session_restore_test_helper.Wait();
    }
    SetBrowser(browser_created_observer.Wait());
  }

  TestUntrustedConfig& subdomains_config() { return *subdomains_config_; }

 private:
  raw_ptr<TestUntrustedConfig> subdomains_config_ = nullptr;
  std::unique_ptr<content::ScopedWebUIConfigRegistration>
      subdomains_registration_;
  std::unique_ptr<content::ScopedWebUIConfigRegistration>
      no_subdomains_registration_;
};

// A subdomain of an opted-in host loads, and both the WebUI and the page see
// the subdomain URL rather than the parent host the config was registered for.
IN_PROC_BROWSER_TEST_F(WebUISubdomainBrowserTest, SubdomainSeesItsOwnURL) {
  const GURL url = PageURL("instance-a", kSubdomainsHost);
  content::WebContents* web_contents = NavigateActiveTab(url);

  EXPECT_EQ(kTestPageTitle, web_contents->GetTitle());
  // The config was asked to create a controller for the subdomain URL, not for
  // the parent host it was registered under.
  EXPECT_THAT(subdomains_config().controller_urls(), testing::ElementsAre(url));
  EXPECT_TRUE(web_contents->GetWebUI());

  // The document is served from a real origin of its own.
  const url::Origin origin = url::Origin::Create(url);
  ASSERT_FALSE(origin.opaque());
  EXPECT_EQ(url.spec(), content::EvalJs(web_contents, "location.href"));
  EXPECT_EQ(origin.Serialize(), content::EvalJs(web_contents, "self.origin"));
}

// The parent host itself keeps working when the config opts into subdomains.
IN_PROC_BROWSER_TEST_F(WebUISubdomainBrowserTest, ParentHostStillLoads) {
  const GURL url = PageURL(kSubdomainsHost);
  content::WebContents* web_contents = NavigateActiveTab(url);

  EXPECT_EQ(kTestPageTitle, web_contents->GetTitle());
  EXPECT_EQ(url.spec(), content::EvalJs(web_contents, "location.href"));
}

// localStorage is keyed by origin, so it must not be shared between two
// subdomains, nor between a subdomain and its parent host. This is the whole
// point of serving each instance from its own subdomain.
IN_PROC_BROWSER_TEST_F(WebUISubdomainBrowserTest, LocalStorageIsNotShared) {
  content::WebContents* web_contents =
      NavigateActiveTab(PageURL("instance-a", kSubdomainsHost));
  ASSERT_EQ("ok", content::EvalJs(web_contents,
                                  SetStorage(kLocalStorage, "value-a")));

  web_contents = NavigateActiveTab(PageURL("instance-b", kSubdomainsHost));
  EXPECT_EQ(kUnset, content::EvalJs(web_contents, GetStorage(kLocalStorage)));
  ASSERT_EQ("ok", content::EvalJs(web_contents,
                                  SetStorage(kLocalStorage, "value-b")));

  web_contents = NavigateActiveTab(PageURL(kSubdomainsHost));
  EXPECT_EQ(kUnset, content::EvalJs(web_contents, GetStorage(kLocalStorage)));

  // Each origin still sees its own value.
  web_contents = NavigateActiveTab(PageURL("instance-a", kSubdomainsHost));
  EXPECT_EQ("value-a",
            content::EvalJs(web_contents, GetStorage(kLocalStorage)));
  web_contents = NavigateActiveTab(PageURL("instance-b", kSubdomainsHost));
  EXPECT_EQ("value-b",
            content::EvalJs(web_contents, GetStorage(kLocalStorage)));
}

// sessionStorage is keyed by origin *and* tab, so it must not leak between
// subdomains in the same tab, nor between tabs showing the same subdomain.
IN_PROC_BROWSER_TEST_F(WebUISubdomainBrowserTest, SessionStorageIsNotShared) {
  const GURL url_a = PageURL("instance-a", kSubdomainsHost);
  const GURL url_b = PageURL("instance-b", kSubdomainsHost);

  content::WebContents* first_tab = NavigateActiveTab(url_a);
  ASSERT_EQ("ok",
            content::EvalJs(first_tab, SetStorage(kSessionStorage, "value-a")));

  // A different subdomain in the same tab does not see it.
  ASSERT_EQ(first_tab, NavigateActiveTab(url_b));
  EXPECT_EQ(kUnset, content::EvalJs(first_tab, GetStorage(kSessionStorage)));

  // Going back to the first subdomain in the same tab does.
  ASSERT_EQ(first_tab, NavigateActiveTab(url_a));
  EXPECT_EQ("value-a", content::EvalJs(first_tab, GetStorage(kSessionStorage)));

  // A second tab on the same subdomain gets its own sessionStorage.
  content::WebContents* second_tab = NavigateNewTab(url_a);
  ASSERT_NE(first_tab, second_tab);
  EXPECT_EQ(kUnset, content::EvalJs(second_tab, GetStorage(kSessionStorage)));
}

// Subdomains are distinct sites, so they must end up in distinct renderer
// processes - both from each other and from the parent host.
IN_PROC_BROWSER_TEST_F(WebUISubdomainBrowserTest,
                       SubdomainsAreProcessIsolated) {
  const GURL url_a = PageURL("instance-a", kSubdomainsHost);
  const GURL url_b = PageURL("instance-b", kSubdomainsHost);
  const GURL url_parent = PageURL(kSubdomainsHost);

  content::WebContents* tab_a = NavigateNewTab(url_a);
  content::WebContents* tab_b = NavigateNewTab(url_b);
  content::WebContents* tab_parent = NavigateNewTab(url_parent);

  content::SiteInstance* site_a =
      tab_a->GetPrimaryMainFrame()->GetSiteInstance();
  content::SiteInstance* site_b =
      tab_b->GetPrimaryMainFrame()->GetSiteInstance();
  content::SiteInstance* site_parent =
      tab_parent->GetPrimaryMainFrame()->GetSiteInstance();

  EXPECT_NE(site_a->GetProcess(), site_b->GetProcess());
  EXPECT_NE(site_a->GetProcess(), site_parent->GetProcess());
  EXPECT_NE(site_b->GetProcess(), site_parent->GetProcess());

  // Each subdomain is a WebUI site of its own, requiring a dedicated process
  // keyed on the full subdomain rather than on the parent host.
  for (auto* site_instance : {site_a, site_b, site_parent}) {
    EXPECT_TRUE(site_instance->RequiresDedicatedProcess());
    EXPECT_TRUE(site_instance->GetSecurityPrincipal().IsWebUI());
  }
  EXPECT_EQ(url_a.host(), site_a->GetSecurityPrincipal().GetHost());
  EXPECT_EQ(url_b.host(), site_b->GetSecurityPrincipal().GetHost());
  EXPECT_EQ(url_parent.host(), site_parent->GetSecurityPrincipal().GetHost());

  // chrome-untrusted:// never gets WebUI bindings, subdomain or not.
  EXPECT_TRUE(tab_a->GetPrimaryMainFrame()->GetEnabledBindings().empty());
  EXPECT_TRUE(tab_b->GetPrimaryMainFrame()->GetEnabledBindings().empty());

  // Navigating between two subdomains within a single tab also swaps processes,
  // even though both subdomains share a WebUI type (their common config).
  const int process_before =
      tab_a->GetPrimaryMainFrame()->GetProcess()->GetDeprecatedID();
  ASSERT_EQ(tab_a, browser()->tab_strip_model()->GetWebContentsAt(1));
  browser()->tab_strip_model()->ActivateTabAt(1);
  ASSERT_EQ(tab_a, NavigateActiveTab(url_b));
  EXPECT_NE(process_before,
            tab_a->GetPrimaryMainFrame()->GetProcess()->GetDeprecatedID());
  EXPECT_FALSE(tab_a->IsCrashed());
}

// A config that has not opted in must not be reachable through a subdomain,
// and asking for one must fail cleanly rather than crash the browser.
IN_PROC_BROWSER_TEST_F(WebUISubdomainBrowserTest,
                       SubdomainOfNonOptedInHostDoesNotLoad) {
  // The host itself is fine.
  ASSERT_TRUE(NavigateActiveTab(PageURL(kNoSubdomainsHost)));

  content::WebContents* web_contents = GetActiveWebContents();
  EXPECT_FALSE(
      NavigateActiveTabAndGetSuccess(PageURL("instance-a", kNoSubdomainsHost)));
  EXPECT_FALSE(web_contents->IsCrashed());
  EXPECT_FALSE(web_contents->GetWebUI());

  // The browser is still usable afterwards.
  EXPECT_EQ(
      kTestPageTitle,
      NavigateActiveTab(PageURL("instance-a", kSubdomainsHost))->GetTitle());
}

// Only single-label subdomains resolve: `a.b.host` has no config registered for
// its direct parent `b.host`, so it must fail cleanly.
IN_PROC_BROWSER_TEST_F(WebUISubdomainBrowserTest,
                       MultiLabelSubdomainDoesNotLoad) {
  const GURL url = content::GetChromeUntrustedUIURL(
      base::StrCat({"a.b.", kSubdomainsHost, "/", kTestPage}));
  content::WebContents* web_contents = GetActiveWebContents();

  EXPECT_FALSE(NavigateActiveTabAndGetSuccess(url));
  EXPECT_FALSE(web_contents->IsCrashed());
  EXPECT_FALSE(web_contents->GetWebUI());
  EXPECT_TRUE(subdomains_config().controller_urls().empty());
}

// History and reloads keep working on a subdomain: each entry re-resolves the
// config for its own origin, and the renderer stays alive.
IN_PROC_BROWSER_TEST_F(WebUISubdomainBrowserTest, ReloadAndHistoryWork) {
  const GURL url_a = PageURL("instance-a", kSubdomainsHost);
  const GURL url_b = PageURL("instance-b", kSubdomainsHost);

  content::WebContents* web_contents = NavigateActiveTab(url_a);
  ASSERT_EQ(web_contents, NavigateActiveTab(url_b));

  // Going back and forward crosses origins, so each entry has to re-resolve.
  ASSERT_TRUE(content::HistoryGoBack(web_contents));
  EXPECT_EQ(url_a, web_contents->GetLastCommittedURL());
  ASSERT_TRUE(content::HistoryGoForward(web_contents));
  EXPECT_EQ(url_b, web_contents->GetLastCommittedURL());

  // Reloading re-creates the WebUI, and therefore re-registers the data source
  // for the subdomain's origin.
  {
    content::TestNavigationObserver observer(web_contents);
    web_contents->GetController().Reload(content::ReloadType::NORMAL,
                                         /*check_for_repost=*/false);
    observer.Wait();
    EXPECT_TRUE(observer.last_navigation_succeeded());
  }
  EXPECT_EQ(url_b, web_contents->GetLastCommittedURL());
  EXPECT_EQ(kTestPageTitle, web_contents->GetTitle());

  // A same-document navigation stays within the subdomain's origin.
  ASSERT_TRUE(content::ExecJs(web_contents, "history.pushState({}, '', '#x')"));
  EXPECT_EQ(url_b.spec() + "#x", web_contents->GetLastCommittedURL().spec());
  EXPECT_FALSE(web_contents->IsCrashed());
}

// Subdomain URLs round-trip through the session service, so restoring a session
// brings each instance back on its own origin with its own storage.
IN_PROC_BROWSER_TEST_F(WebUISubdomainBrowserTest, SessionRestore) {
  const GURL url_a = PageURL("instance-a", kSubdomainsHost);
  const GURL url_b = PageURL("instance-b", kSubdomainsHost);

  ASSERT_EQ("ok", content::EvalJs(NavigateActiveTab(url_a),
                                  SetStorage(kLocalStorage, "value-a")));
  ASSERT_EQ("ok", content::EvalJs(NavigateNewTab(url_b),
                                  SetStorage(kLocalStorage, "value-b")));
  ASSERT_EQ(2, browser()->tab_strip_model()->count());

  CloseBrowserAndRestoreSession();

  ASSERT_EQ(2, browser()->tab_strip_model()->count());

  content::WebContents* restored_a = LoadRestoredTabAt(0);
  EXPECT_EQ(url_a, restored_a->GetLastCommittedURL());
  EXPECT_EQ(kTestPageTitle, restored_a->GetTitle());
  EXPECT_EQ("value-a", content::EvalJs(restored_a, GetStorage(kLocalStorage)));

  content::WebContents* restored_b = LoadRestoredTabAt(1);
  EXPECT_EQ(url_b, restored_b->GetLastCommittedURL());
  EXPECT_EQ("value-b", content::EvalJs(restored_b, GetStorage(kLocalStorage)));

  // The restored instances are still isolated from each other.
  EXPECT_NE(restored_a->GetPrimaryMainFrame()->GetProcess(),
            restored_b->GetPrimaryMainFrame()->GetProcess());
}
