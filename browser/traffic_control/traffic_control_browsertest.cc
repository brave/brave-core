// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/escape.h"
#include "base/strings/string_split.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/browser/traffic_control/traffic_control_service_factory.h"
#include "brave/browser/traffic_control/traffic_control_tab_utils.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/browser/containers_test_utils.h"
#include "brave/components/containers/core/browser/temporary_container.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "brave/components/traffic_control/core/browser/pref_names.h"
#include "brave/components/traffic_control/core/browser/prefs.h"
#include "brave/components/traffic_control/core/browser/traffic_control_service.h"
#include "brave/components/traffic_control/core/common/features.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace traffic_control {

namespace {

mojom::TrafficRulePtr MakeRule(const std::string& id,
                               bool enabled,
                               const std::string& url_filter,
                               std::optional<std::string> container_id,
                               bool temporary = false) {
  return mojom::TrafficRule::New(
      id, enabled, mojom::Condition::New(url_filter),
      mojom::Target::New(std::move(container_id), temporary));
}

}  // namespace

class TrafficControlBrowserTest : public InProcessBrowserTest {
 public:
  TrafficControlBrowserTest() {
    feature_list_.InitWithFeatures(
        {features::kTrafficControl, containers::features::kContainers}, {});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());

    PrefService* prefs = browser()->GetProfile()->GetPrefs();
    containers::SetContainersEnabled(true, prefs);
    prefs->SetBoolean(prefs::kTrafficControlEnabled, true);

    auto* containers_service =
        ContainersServiceFactory::GetForProfile(browser()->GetProfile());
    ASSERT_TRUE(containers_service);
    auto containers = containers_service->GetContainers();
    ASSERT_FALSE(containers.empty());
    container_id_ = containers[0]->id;
  }

 protected:
  GURL TestUrl(const std::string& host = "example.com") {
    return embedded_test_server()->GetURL(host, "/simple.html");
  }

  void SetRules(std::vector<mojom::TrafficRulePtr> rules) {
    SetRulesToPrefs(rules, *browser()->GetProfile()->GetPrefs());
  }

  // Starts a navigation that Traffic Control cancels and re-opens in a new tab.
  // Returns the new active WebContents after load.
  content::WebContents* NavigateExpectingReroute(const GURL& url) {
    const int initial_count = browser()->tab_strip_model()->count();
    content::OpenURLParams params(url, content::Referrer(),
                                  WindowOpenDisposition::kCurrentTab,
                                  ui::PAGE_TRANSITION_TYPED, false);
    browser()->OpenURL(params, /*navigation_handle_callback=*/{});

    EXPECT_TRUE(base::test::RunUntil([&]() {
      // Either a new tab was added, or the source empty tab was replaced
      // (close + open keeps count stable / decreases).
      content::WebContents* active =
          browser()->tab_strip_model()->GetActiveWebContents();
      return active && active->GetLastCommittedURL().host() == url.host() &&
             !active->IsLoading();
    }));

    content::WebContents* active =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_TRUE(active);
    EXPECT_NE(active->GetLastCommittedURL().host(), "");
    // Silence unused in non-assert builds.
    EXPECT_GE(browser()->tab_strip_model()->count() + 1, initial_count);
    return active;
  }

  void OpenUrlInContainerForTest(
      const GURL& url,
      const containers::mojom::ContainerPtr& container) {
    // Match production OpenUrlInContainer: do not let Traffic Control override.
    NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
    params.disposition = WindowOpenDisposition::kNewForegroundTab;
    params.storage_partition_config = content::StoragePartitionConfig::Create(
        browser()->GetProfile(), containers::kContainersStoragePartitionDomain,
        container->id, browser()->GetProfile()->IsOffTheRecord());
    Navigate(&params);
  }

  void NavigateInCurrentTab(const GURL& url, ui::PageTransition transition) {
    content::OpenURLParams params(url, content::Referrer(),
                                  WindowOpenDisposition::kCurrentTab,
                                  transition, false);
    browser()->OpenURL(params, /*navigation_handle_callback=*/{});
  }

  std::string container_id_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       ReroutesFromNtpAndClosesEmptyTab) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                           GURL(chrome::kChromeUINewTabURL)));
  const int initial_tab_count = browser()->tab_strip_model()->count();

  content::WebContents* new_tab = NavigateExpectingReroute(TestUrl());
  ASSERT_TRUE(new_tab);

  EXPECT_EQ(containers::GetContainerIdForWebContents(new_tab), container_id_);
  EXPECT_EQ(new_tab->GetLastCommittedURL().host(), "example.com");
  EXPECT_LE(browser()->tab_strip_model()->count(), initial_tab_count + 1);
  EXPECT_TRUE(base::test::RunUntil([&]() {
    return containers::GetContainerIdForWebContents(
               browser()->tab_strip_model()->GetActiveWebContents()) ==
           container_id_;
  }));
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest, KeepsNonEmptySourceTab) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("a.test", "/simple.html")));
  content::WebContents* source =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(source);
  const int initial_tab_count = browser()->tab_strip_model()->count();

  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  content::WebContents* new_tab = NavigateExpectingReroute(TestUrl());
  ASSERT_TRUE(new_tab);

  EXPECT_EQ(browser()->tab_strip_model()->count(), initial_tab_count + 1);
  EXPECT_NE(browser()->tab_strip_model()->GetIndexOfWebContents(source),
            TabStripModel::kNoTab);
  EXPECT_EQ(containers::GetContainerIdForWebContents(new_tab), container_id_);
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest, AlreadyInContainerProceeds) {
  auto* containers_service =
      ContainersServiceFactory::GetForProfile(browser()->GetProfile());
  auto container = containers_service->GetRuntimeContainerById(container_id_);
  ASSERT_TRUE(container);

  OpenUrlInContainerForTest(TestUrl(), container);
  content::WebContents* contained =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contained));
  EXPECT_EQ(containers::GetContainerIdForWebContents(contained), container_id_);
  const int tab_count = browser()->tab_strip_model()->count();

  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("example.com", "/title1.html")));
  EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count);
  EXPECT_EQ(containers::GetContainerIdForWebContents(
                browser()->tab_strip_model()->GetActiveWebContents()),
            container_id_);
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest, DisabledRuleDoesNotReroute) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", false, "example.com", container_id_));
  SetRules(std::move(rules));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), TestUrl()));
  content::WebContents* active =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(containers::GetContainerIdForWebContents(active).empty());
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest, PrefOffDoesNotReroute) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));
  browser()->GetProfile()->GetPrefs()->SetBoolean(prefs::kTrafficControlEnabled,
                                                  false);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), TestUrl()));
  content::WebContents* active =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(containers::GetContainerIdForWebContents(active).empty());
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       UnknownContainerLeavesTabInCurrentContainer) {
  auto* containers_service =
      ContainersServiceFactory::GetForProfile(browser()->GetProfile());
  auto container = containers_service->GetRuntimeContainerById(container_id_);
  ASSERT_TRUE(container);
  OpenUrlInContainerForTest(TestUrl("a.test"), container);
  content::WebContents* contained =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contained));

  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", "missing-container-id"));
  SetRules(std::move(rules));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), TestUrl()));
  EXPECT_EQ(browser()->tab_strip_model()->GetActiveWebContents(), contained);
  EXPECT_EQ(containers::GetContainerIdForWebContents(contained), container_id_);
  EXPECT_EQ(contained->GetLastCommittedURL().host(), "example.com");
}

// A confined container's tab is not moved, whether the rule targets another
// container or no container.
IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       ConfinedContainerIsNotRerouted) {
  auto* containers_service =
      ContainersServiceFactory::GetForProfile(browser()->GetProfile());
  auto container = containers_service->CreateAndPersistTemporaryContainer();
  containers_service->SetContainerConfined(container->id);
  OpenUrlInContainerForTest(TestUrl("a.test"), container);
  content::WebContents* confined =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(confined));
  const int tab_count = browser()->tab_strip_model()->count();

  const GURL to_container_url = TestUrl();
  const GURL to_no_container_url = TestUrl("b.test");
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, std::string(to_container_url.host()),
                           container_id_));
  rules.push_back(MakeRule("r2", true, std::string(to_no_container_url.host()),
                           std::string("")));
  SetRules(std::move(rules));

  for (const GURL& url : {to_container_url, to_no_container_url}) {
    SCOPED_TRACE(url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count);
    EXPECT_EQ(browser()->tab_strip_model()->GetActiveWebContents(), confined);
    EXPECT_EQ(confined->GetLastCommittedURL(), url);
    EXPECT_EQ(containers::GetContainerIdForWebContents(confined),
              container->id);
  }
}

// Confinement, not being a temporary container, is what keeps a tab in place.
IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       UnconfinedTemporaryContainerIsRerouted) {
  auto* containers_service =
      ContainersServiceFactory::GetForProfile(browser()->GetProfile());
  auto container = containers_service->CreateAndPersistTemporaryContainer();
  OpenUrlInContainerForTest(TestUrl("a.test"), container);
  ASSERT_TRUE(content::WaitForLoadStop(
      browser()->tab_strip_model()->GetActiveWebContents()));

  const GURL url = TestUrl();
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, std::string(url.host()), container_id_));
  SetRules(std::move(rules));

  content::WebContents* new_tab = NavigateExpectingReroute(url);
  ASSERT_TRUE(new_tab);
  EXPECT_EQ(containers::GetContainerIdForWebContents(new_tab), container_id_);
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest, OpenWithoutContainer) {
  auto* containers_service =
      ContainersServiceFactory::GetForProfile(browser()->GetProfile());
  auto container = containers_service->GetRuntimeContainerById(container_id_);
  ASSERT_TRUE(container);
  OpenUrlInContainerForTest(TestUrl("a.test"), container);
  ASSERT_TRUE(content::WaitForLoadStop(
      browser()->tab_strip_model()->GetActiveWebContents()));
  EXPECT_FALSE(containers::GetContainerIdForWebContents(
                   browser()->tab_strip_model()->GetActiveWebContents())
                   .empty());

  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", std::string("")));
  SetRules(std::move(rules));

  content::WebContents* new_tab = NavigateExpectingReroute(TestUrl());
  ASSERT_TRUE(new_tab);
  EXPECT_TRUE(containers::GetContainerIdForWebContents(new_tab).empty());
  EXPECT_EQ(new_tab->GetLastCommittedURL().host(), "example.com");
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest, TemporaryContainer) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", std::nullopt, true));
  SetRules(std::move(rules));

  content::OpenURLParams params(TestUrl(), content::Referrer(),
                                WindowOpenDisposition::kCurrentTab,
                                ui::PAGE_TRANSITION_TYPED, false);
  browser()->OpenURL(params, /*navigation_handle_callback=*/{});

  EXPECT_TRUE(base::test::RunUntil([&]() {
    content::WebContents* active =
        browser()->tab_strip_model()->GetActiveWebContents();
    return active && containers::IsTemporaryContainerId(
                         containers::GetContainerIdForWebContents(active));
  }));

  content::WebContents* active =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(active);
  EXPECT_TRUE(content::WaitForLoadStop(active));
  EXPECT_EQ(active->GetLastCommittedURL().host(), "example.com");
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       SameSiteNavigationDoesNotReroute) {
  // Land on the matching site before the rule exists.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), TestUrl()));
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(tab);
  EXPECT_TRUE(containers::GetContainerIdForWebContents(tab).empty());
  const int tab_count = browser()->tab_strip_model()->count();

  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  // Same schemeful site, link transition — should stay put.
  const GURL same_site =
      embedded_test_server()->GetURL("example.com", "/title1.html");
  NavigateInCurrentTab(same_site, ui::PAGE_TRANSITION_LINK);
  ASSERT_TRUE(content::WaitForLoadStop(tab));

  EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count);
  EXPECT_TRUE(containers::GetContainerIdForWebContents(tab).empty());
  EXPECT_EQ(tab->GetLastCommittedURL().GetPath(), "/title1.html");
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest, OmniboxSameSiteDoesReroute) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), TestUrl()));
  EXPECT_TRUE(containers::GetContainerIdForWebContents(
                  browser()->tab_strip_model()->GetActiveWebContents())
                  .empty());

  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  const GURL same_site =
      embedded_test_server()->GetURL("example.com", "/title1.html");
  content::WebContents* new_tab = NavigateExpectingReroute(same_site);
  ASSERT_TRUE(new_tab);
  EXPECT_EQ(containers::GetContainerIdForWebContents(new_tab), container_id_);
}

// Models a cross-site single sign-on hand-off such as accounts.google.com ->
// youtube.com: the identity provider reads its own cookie and redirects to
// the relying party with a token that the relying party turns into a session
// cookie.
class TrafficControlSsoBrowserTest : public TrafficControlBrowserTest {
 public:
  void SetUpOnMainThread() override {
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&TrafficControlSsoBrowserTest::HandleRequest));
    TrafficControlBrowserTest::SetUpOnMainThread();
  }

 protected:
  static std::string FindCookie(const net::test_server::HttpRequest& request,
                                std::string_view name) {
    auto it = request.headers.find("Cookie");
    if (it == request.headers.end()) {
      return "none";
    }
    const std::string prefix = std::string(name) + "=";
    for (std::string_view cookie :
         base::SplitStringPiece(it->second, ";", base::TRIM_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY)) {
      if (cookie.starts_with(prefix)) {
        return std::string(cookie.substr(prefix.size()));
      }
    }
    return "none";
  }

  static std::unique_ptr<net::test_server::HttpResponse> SetCookieResponse(
      const std::string& cookie) {
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_content_type("text/html");
    response->AddCustomHeader("Set-Cookie", cookie + "; Path=/");
    response->set_content(cookie);
    return response;
  }

  static std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    const GURL url = request.GetURL();
    // Identity provider sign-in: /idp/login?<account>
    if (url.path() == "/idp/login") {
      return SetCookieResponse("idp_account=" + std::string(url.query()));
    }
    // Identity provider hand-off: /idp/sso?<escaped continue URL>
    if (url.path() == "/idp/sso") {
      const std::string continue_url =
          base::UnescapeBinaryURLComponent(url.query());
      auto response = std::make_unique<net::test_server::BasicHttpResponse>();
      response->set_code(net::HTTP_FOUND);
      response->AddCustomHeader(
          "Location",
          continue_url + "?token=" + FindCookie(request, "idp_account"));
      return response;
    }
    // Relying party consumes the token: /rp/signin?token=<account>
    if (url.path() == "/rp/signin") {
      const std::string query(url.query());
      if (!query.starts_with("token=")) {
        return nullptr;
      }
      return SetCookieResponse("rp_account=" + query.substr(6));
    }
    return nullptr;
  }

  void RunHandOffFromDefaultPartition(const std::string& navigate_script);
};

// The default partition is signed in to the identity provider as account A,
// the container as account B. A sign-on hand-off that starts in the default
// partition must not sign the container's relying party in as account A.
// |navigate_script| is a JsReplace template that starts the hand-off from the
// default tab.
void TrafficControlSsoBrowserTest::RunHandOffFromDefaultPartition(
    const std::string& navigate_script) {
  const GURL idp_login_a =
      embedded_test_server()->GetURL("accounts.test", "/idp/login?A");
  const GURL idp_login_b =
      embedded_test_server()->GetURL("accounts.test", "/idp/login?B");
  const GURL rp_signin =
      embedded_test_server()->GetURL("video.test", "/rp/signin");
  const GURL idp_sso = embedded_test_server()->GetURL(
      "accounts.test",
      "/idp/sso?" + base::EscapeQueryParamValue(rp_signin.spec(), false));

  // Account A in the default partition.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), idp_login_a));
  content::WebContents* default_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(containers::GetContainerIdForWebContents(default_tab).empty());

  // Account B in the container.
  auto* containers_service =
      ContainersServiceFactory::GetForProfile(browser()->GetProfile());
  auto container = containers_service->GetRuntimeContainerById(container_id_);
  ASSERT_TRUE(container);
  OpenUrlInContainerForTest(idp_login_b, container);
  content::WebContents* container_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(container_tab));
  ASSERT_EQ(containers::GetContainerIdForWebContents(container_tab),
            container_id_);
  ASSERT_EQ("idp_account=B", content::EvalJs(container_tab, "document.cookie"));

  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "video.test", container_id_));
  SetRules(std::move(rules));

  // Follow the hand-off from the default partition, the way a link on an
  // account dashboard does.
  browser()->tab_strip_model()->ActivateTabAt(
      browser()->tab_strip_model()->GetIndexOfWebContents(default_tab));
  const int tab_count = browser()->tab_strip_model()->count();
  ASSERT_TRUE(content::ExecJs(default_tab,
                              content::JsReplace(navigate_script, idp_sso)));

  // Traffic Control cancels the redirect to video.test and replays the
  // hand-off in the container.
  content::WebContents* rerouted = nullptr;
  ASSERT_TRUE(base::test::RunUntil([&]() {
    if (browser()->tab_strip_model()->count() != tab_count + 1) {
      return false;
    }
    rerouted = browser()->tab_strip_model()->GetActiveWebContents();
    return rerouted && rerouted->GetLastCommittedURL().host() == "video.test" &&
           !rerouted->IsLoading();
  }));
  ASSERT_EQ(containers::GetContainerIdForWebContents(rerouted), container_id_);
  EXPECT_EQ(rerouted->GetLastCommittedURL().query(), "token=B");
  EXPECT_EQ("rp_account=B", content::EvalJs(rerouted, "document.cookie"));
  EXPECT_EQ("idp_account=B", content::EvalJs(container_tab, "document.cookie"));
}

IN_PROC_BROWSER_TEST_F(TrafficControlSsoBrowserTest,
                       RedirectRerouteDoesNotCarryCrossContainerSession) {
  RunHandOffFromDefaultPartition("location.href = $1;");
}

// location.replace() is a client-side redirect, which puts the default tab's
// page ahead of the hand-off in the redirect chain. Reopening that page in
// the container would sign its identity provider in as account A.
IN_PROC_BROWSER_TEST_F(TrafficControlSsoBrowserTest,
                       ClientRedirectRerouteDoesNotCarryCrossContainerSession) {
  RunHandOffFromDefaultPartition("location.replace($1);");
}

}  // namespace traffic_control
