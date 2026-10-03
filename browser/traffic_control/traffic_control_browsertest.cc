// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/memory/weak_ptr.h"
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
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/resource_request_body.h"
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
  content::WebContents* NavigateExpectingReroute(
      const GURL& url,
      ui::PageTransition transition = ui::PAGE_TRANSITION_TYPED,
      bool is_renderer_initiated = false) {
    const int initial_count = browser()->tab_strip_model()->count();
    content::OpenURLParams params(url, content::Referrer(),
                                  WindowOpenDisposition::CURRENT_TAB,
                                  transition, is_renderer_initiated);
    if (is_renderer_initiated) {
      params.initiator_origin =
          url::Origin::Create(browser()
                                  ->tab_strip_model()
                                  ->GetActiveWebContents()
                                  ->GetLastCommittedURL());
    }
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
    params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
    params.storage_partition_config = content::StoragePartitionConfig::Create(
        browser()->GetProfile(), containers::kContainersStoragePartitionDomain,
        container->id, browser()->GetProfile()->IsOffTheRecord());
    Navigate(&params);
  }

  void NavigateInCurrentTab(const GURL& url, ui::PageTransition transition) {
    content::OpenURLParams params(url, content::Referrer(),
                                  WindowOpenDisposition::CURRENT_TAB,
                                  transition, false);
    browser()->OpenURL(params, /*navigation_handle_callback=*/{});
  }

  void NavigatePostInCurrentTab(const GURL& url) {
    content::OpenURLParams params(url, content::Referrer(),
                                  WindowOpenDisposition::CURRENT_TAB,
                                  ui::PAGE_TRANSITION_FORM_SUBMIT, false);
    params.post_data = network::ResourceRequestBody::CreateFromCopyOfBytes(
        base::as_byte_span("traffic-control-test"));
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

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       PrivateWindowDoesNotInheritRulesOrStorage) {
  const GURL url = TestUrl();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* regular_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::ExecJs(regular_tab, R"(
    localStorage.setItem('traffic-control', 'regular');
    document.cookie = 'traffic_control=regular; path=/';
  )"));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), TestUrl("a.test")));

  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  content::WebContents* contained_tab = NavigateExpectingReroute(url);
  ASSERT_TRUE(contained_tab);
  ASSERT_EQ(containers::GetContainerIdForWebContents(contained_tab),
            container_id_);
  ASSERT_TRUE(content::ExecJs(contained_tab, R"(
    localStorage.setItem('traffic-control', 'container');
    document.cookie = 'traffic_control=container; path=/';
  )"));
  const int regular_tab_count = browser()->tab_strip_model()->count();

  auto* private_browser = CreateIncognitoBrowser(browser()->GetProfile());
  ASSERT_TRUE(private_browser->GetProfile()->IsOffTheRecord());
  EXPECT_EQ(TrafficControlServiceFactory::GetForProfile(
                private_browser->GetProfile()),
            nullptr);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(private_browser, url));
  content::WebContents* private_tab =
      private_browser->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(private_tab);

  EXPECT_EQ(private_browser->tab_strip_model()->count(), 1);
  EXPECT_EQ(browser()->tab_strip_model()->count(), regular_tab_count);
  EXPECT_EQ(private_tab->GetBrowserContext(), private_browser->GetProfile());
  EXPECT_EQ(private_tab->GetLastCommittedURL(), url);
  EXPECT_TRUE(containers::GetContainerIdForWebContents(private_tab).empty());
  EXPECT_EQ(content::EvalJs(private_tab,
                            "localStorage.getItem('traffic-control') === null"),
            true);
  EXPECT_EQ(content::EvalJs(private_tab, "document.cookie"), "");
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       RerouteFromNewEmptyTabClosesSourceTab) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  const int initial_tab_count = browser()->tab_strip_model()->count();
  NavigateParams params(browser(), GURL(chrome::kChromeUINewTabURL),
                        ui::PAGE_TRANSITION_TYPED);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  Navigate(&params);
  const int tab_count_with_empty_tab = browser()->tab_strip_model()->count();
  ASSERT_EQ(tab_count_with_empty_tab, initial_tab_count + 1);

  content::WebContents* new_tab = NavigateExpectingReroute(TestUrl());
  ASSERT_TRUE(new_tab);

  EXPECT_EQ(browser()->tab_strip_model()->count(), initial_tab_count + 1);
  EXPECT_EQ(containers::GetContainerIdForWebContents(new_tab), container_id_);
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

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       KeepsSourceTabWithPendingNavigation) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  content::WebContents* source =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(source);
  base::WeakPtr<content::WebContents> source_weak = source->GetWeakPtr();
  const int initial_tab_count = browser()->tab_strip_model()->count();

  content::OpenURLParams params(TestUrl(), content::Referrer(),
                                WindowOpenDisposition::CURRENT_TAB,
                                ui::PAGE_TRANSITION_TYPED, false);
  browser()->OpenURL(params, /*navigation_handle_callback=*/{});
  NavigateInCurrentTab(TestUrl("a.test"), ui::PAGE_TRANSITION_LINK);

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return source_weak &&
           source_weak->GetLastCommittedURL().host() == "a.test" &&
           !source_weak->IsLoading();
  }));
  EXPECT_NE(browser()->tab_strip_model()->GetIndexOfWebContents(source),
            TabStripModel::kNoTab);
  EXPECT_EQ(browser()->tab_strip_model()->count(), initial_tab_count + 1);
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
                                WindowOpenDisposition::CURRENT_TAB,
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

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       AutomaticTransitionIsPreservedWhenRerouted) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  content::WebContents* new_tab =
      NavigateExpectingReroute(TestUrl(), ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                               /*is_renderer_initiated=*/true);
  ASSERT_TRUE(new_tab);

  content::NavigationEntry* entry =
      new_tab->GetController().GetLastCommittedEntry();
  ASSERT_TRUE(entry);
  EXPECT_TRUE(ui::PageTransitionCoreTypeIs(entry->GetTransitionType(),
                                           ui::PAGE_TRANSITION_AUTO_TOPLEVEL));
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       SandboxedTopNavigationDoesNotReroute) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), TestUrl("a.test")));
  content::WebContents* source =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(source);
  const int tab_count = browser()->tab_strip_model()->count();
  const GURL destination_url = TestUrl();
  // Allow top navigation to reach Traffic Control while omitting allow-popups.
  const GURL frame_url = embedded_test_server()->GetURL(
      "a.test",
      "/set-header?Content-Security-Policy: sandbox allow-scripts "
      "allow-same-origin allow-top-navigation");

  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  ASSERT_TRUE(content::ExecJs(source, content::JsReplace(R"JS(
        const frame = document.createElement('iframe');
        frame.src = $1;
        document.body.appendChild(frame);
      )JS",
                                                         frame_url)));

  ASSERT_TRUE(base::test::RunUntil([&]() {
    content::RenderFrameHost* frame = content::ChildFrameAt(source, 0);
    return frame && frame->GetLastCommittedURL() == frame_url;
  }));

  content::RenderFrameHost* frame = content::ChildFrameAt(source, 0);
  ASSERT_TRUE(frame);

  // Exercise the sandbox restriction without supplying a user activation.
  content::ExecuteScriptAsyncWithoutUserGesture(
      frame, content::JsReplace("top.location = $1", destination_url));
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return source->GetLastCommittedURL() == destination_url; }));

  EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count);
  EXPECT_EQ(browser()->tab_strip_model()->GetActiveWebContents(), source);
  EXPECT_EQ(source->GetLastCommittedURL(), destination_url);
  EXPECT_TRUE(containers::GetContainerIdForWebContents(source).empty());
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       PostNavigationDoesNotReroute) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  content::WebContents* source =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(source);
  const int tab_count = browser()->tab_strip_model()->count();
  NavigatePostInCurrentTab(TestUrl());
  ASSERT_TRUE(content::WaitForLoadStop(source));

  EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count);
  EXPECT_EQ(browser()->tab_strip_model()->GetActiveWebContents(), source);
  EXPECT_TRUE(containers::GetContainerIdForWebContents(source).empty());
  EXPECT_EQ(source->GetLastCommittedURL().host(), "example.com");
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest, ReloadDoesNotReroute) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), TestUrl()));
  content::WebContents* source =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(source);
  const int tab_count = browser()->tab_strip_model()->count();

  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  source->GetController().Reload(content::ReloadType::NORMAL,
                                 /*check_for_repost=*/true);
  ASSERT_TRUE(content::WaitForLoadStop(source));

  EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count);
  EXPECT_EQ(browser()->tab_strip_model()->GetActiveWebContents(), source);
  EXPECT_TRUE(containers::GetContainerIdForWebContents(source).empty());
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       HistoryNavigationDoesNotReroute) {
  const GURL matching_url = TestUrl();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), matching_url));
  content::WebContents* source =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(source);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), TestUrl("a.test")));
  const int tab_count = browser()->tab_strip_model()->count();

  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com", container_id_));
  SetRules(std::move(rules));

  source->GetController().GoBack();
  ASSERT_TRUE(content::WaitForLoadStop(source));

  EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count);
  EXPECT_EQ(browser()->tab_strip_model()->GetActiveWebContents(), source);
  EXPECT_TRUE(containers::GetContainerIdForWebContents(source).empty());
  EXPECT_EQ(source->GetLastCommittedURL(), matching_url);
}

IN_PROC_BROWSER_TEST_F(TrafficControlBrowserTest,
                       NonHttpNavigationDoesNotReroute) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "chrome://version", container_id_));
  SetRules(std::move(rules));

  auto* service =
      TrafficControlServiceFactory::GetForProfile(browser()->GetProfile());
  ASSERT_TRUE(service);
  const GURL version_url(chrome::kChromeUIVersionURL);
  ASSERT_TRUE(service->FindMatchingRule(version_url));

  const int tab_count = browser()->tab_strip_model()->count();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), version_url));
  content::WebContents* active =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count);
  EXPECT_TRUE(containers::GetContainerIdForWebContents(active).empty());
  EXPECT_EQ(active->GetLastCommittedURL(), version_url);
}

}  // namespace traffic_control
