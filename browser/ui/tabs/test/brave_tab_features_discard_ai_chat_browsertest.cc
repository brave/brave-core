// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/ai_chat/tab_tracker_service_factory.h"
#include "brave/browser/ui/tabs/test/brave_tab_features_discard_browsertest.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/components/web_mcp/core/browser/web_mcp_rule_registry.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "third_party/blink/public/common/features.h"
#include "url/gurl.h"

namespace {

// Keeps the latest tab data reported by the TabTrackerService.
class TestTabDataObserver : public ai_chat::mojom::TabDataObserver {
 public:
  mojo::PendingRemote<ai_chat::mojom::TabDataObserver> BindAndPassRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  bool HasTab(int32_t id, const GURL& url, const std::string& title) const {
    return std::ranges::any_of(tabs_, [&](const auto& tab) {
      return tab->id == id && tab->url == url && tab->title == title;
    });
  }

  // ai_chat::mojom::TabDataObserver:
  void TabDataChanged(std::vector<ai_chat::mojom::TabDataPtr> tabs) override {
    tabs_ = std::move(tabs);
  }

 private:
  std::vector<ai_chat::mojom::TabDataPtr> tabs_;
  mojo::Receiver<ai_chat::mojom::TabDataObserver> receiver_{this};
};

}  // namespace

class BraveTabFeaturesDiscardAIChatBrowserTest
    : public BraveTabFeaturesDiscardBrowserTest {
 public:
  BraveTabFeaturesDiscardAIChatBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(blink::features::kWebMCP);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    BraveTabFeaturesDiscardBrowserTest::SetUpCommandLine(command_line);
    // document.modelContext is an experimental runtime feature.
    command_line->AppendSwitchASCII("enable-blink-features", "WebMCP");
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveTabFeaturesDiscardAIChatBrowserTest,
                       TabTrackerFollowsDiscardedTab) {
  TestTabDataObserver observer;
  ai_chat::TabTrackerServiceFactory::GetForBrowserContext(
      browser()->GetProfile())
      ->AddObserver(observer.BindAndPassRemote());

  tabs::TabInterface* tab = OpenBackgroundTab(GetURL("/title2.html"));
  ASSERT_NO_FATAL_FAILURE(DiscardAndReload(tab));

  const GURL url = GetURL("/title3.html");
  ASSERT_TRUE(content::NavigateToURL(tab->GetContents(), url));
  EXPECT_TRUE(base::test::RunUntil([&] {
    return observer.HasTab(tab->GetHandle().raw_value(), url,
                           "Title Of More Awesomeness");
  }));
}

IN_PROC_BROWSER_TEST_F(BraveTabFeaturesDiscardAIChatBrowserTest,
                       WebMcpToolInjectedIntoDiscardedTab) {
  // Covers creating the component directory and deleting it at the end.
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir component_dir;
  ASSERT_TRUE(component_dir.CreateUniqueTempDir());
  const base::FilePath scripts_dir =
      component_dir.GetPath().AppendASCII("scripts");
  ASSERT_TRUE(base::CreateDirectory(scripts_dir));
  // The port is part of the URL spec, hence the wildcard after the host.
  constexpr char kScript[] = R"(// ==WebMCP==
// @name discard_test_tool
// @match https://a.com*
// @description Test tool.
// ==/WebMCP==
return 'ok';
)";
  ASSERT_TRUE(base::WriteFile(scripts_dir.AppendASCII("tool.js"), kScript));
  auto* registry = web_mcp::WebMcpRuleRegistry::GetInstance();
  registry->LoadRules(component_dir.GetPath());
  ASSERT_TRUE(base::test::RunUntil([&] {
    return std::ranges::any_of(registry->rules(), [](const auto& rule) {
      return rule.tool_name == "discard_test_tool";
    });
  }));

  tabs::TabInterface* tab = OpenBackgroundTab(GetURL("/title2.html"));
  ASSERT_NO_FATAL_FAILURE(DiscardAndReload(tab));

  // Waits until the injected script has registered the tool.
  EXPECT_EQ(true, content::EvalJs(tab->GetContents(), R"JS(
    (async () => {
      while (!(await document.modelContext.getTools())
                 .some(tool => tool.name === 'discard_test_tool')) {
        await new Promise(resolve => setTimeout(resolve, 50));
      }
      return true;
    })()
  )JS"));
}
