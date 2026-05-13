/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <set>
#include <string>

#include "base/command_line.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/sessions/brave_tree_tab_session_keys.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/sessions/core/session_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/common/page_state/page_state.h"
#include "third_party/blink/public/common/page_state/page_state_serialization.h"

using BraveSessionRestoreBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveSessionRestoreBrowserTest,
                       SerializationClearNonEmptyPageState) {
  auto* tab_model = browser()->tab_strip_model();
  auto* web_contents = tab_model->GetActiveWebContents();
  SessionService* const session_service =
      SessionServiceFactory::GetForProfile(browser()->profile());
  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
      browser(), GURL("brave://newtab/"), 1);
  ASSERT_EQ(true, EvalJs(web_contents,
                         R"(
        var textarea = document.createElement('textarea')
        textarea.textContent = '__some_text__'
        document.body.append(textarea);
        var input = document.createElement('input')
        input.autocomplete = 'on'
        input.value = '__some_text__'
        document.body.append(input);
        var controls_ready = document.getElementsByTagName('textarea')[0].textContent === '__some_text__' &&
                             document.getElementsByTagName('input')[0].value === '__some_text__';
        controls_ready;
      )"));
  session_service->MoveCurrentSessionToLastSession();
  base::RunLoop loop;
  session_service->GetLastSession(base::BindLambdaForTesting(
      [&](std::vector<std::unique_ptr<sessions::SessionWindow>> windows,
          SessionID ignored_active_window, bool error_reading) {
        EXPECT_EQ(windows.size(), 1u);
        EXPECT_EQ(windows[0]->tabs.size(), 1u);
        EXPECT_EQ(windows[0]->tabs[0]->navigations.size(), 2u);
        const auto& serialized_navigation = windows[0]->tabs[0]->navigations[1];
        EXPECT_EQ(serialized_navigation.virtual_url(),
                  GURL("chrome://newtab/"));

        // Check encoded data is not empty but clean state only with url info.
        EXPECT_EQ(blink::PageState::CreateFromURL(GURL("chrome://newtab/"))
                      .ToEncodedData(),
                  serialized_navigation.encoded_page_state());
        EXPECT_FALSE(serialized_navigation.encoded_page_state().empty());
        loop.Quit();
      }));
  loop.Run();
}

IN_PROC_BROWSER_TEST_F(BraveSessionRestoreBrowserTest,
                       SerializationClearEmptyPageState) {
  auto* tab_model = browser()->tab_strip_model();
  auto* web_contents = tab_model->GetActiveWebContents();
  SessionService* const session_service =
      SessionServiceFactory::GetForProfile(browser()->profile());
  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
      browser(), GURL("brave://rewards/"), 1);
  ASSERT_EQ(true, EvalJs(web_contents,
                         R"(
        var textarea = document.createElement('textarea')
        textarea.textContent = '__some_text__'
        document.body.append(textarea);
        var input = document.createElement('input')
        input.autocomplete = 'on'
        input.value = '__some_text__'
        document.body.append(input);
        var controls_ready = document.getElementsByTagName('textarea')[0].textContent === '__some_text__' &&
                             document.getElementsByTagName('input')[0].value === '__some_text__';
        controls_ready;
      )"));
  session_service->MoveCurrentSessionToLastSession();
  base::RunLoop loop;
  session_service->GetLastSession(base::BindLambdaForTesting(
      [&](std::vector<std::unique_ptr<sessions::SessionWindow>> windows,
          SessionID ignored_active_window, bool error_reading) {
        EXPECT_EQ(windows.size(), 1u);
        EXPECT_EQ(windows[0]->tabs.size(), 1u);
        EXPECT_EQ(windows[0]->tabs[0]->navigations.size(), 2u);
        const auto& serialized_navigation = windows[0]->tabs[0]->navigations[1];
        EXPECT_EQ(serialized_navigation.virtual_url(),
                  GURL("chrome://rewards/"));

        // Check encoded data is empty.
        EXPECT_TRUE(serialized_navigation.encoded_page_state().empty());
        loop.Quit();
      }));
  loop.Run();
}

class SessionCookiesCleanupOnSessionRestoreBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  SessionCookiesCleanupOnSessionRestoreBrowserTest() {
    if (ShouldCleanupSessionCookies()) {
      scoped_feature_list_.InitAndEnableFeature(
          features::kBraveCleanupSessionCookiesOnSessionRestore);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          features::kBraveCleanupSessionCookiesOnSessionRestore);
    }
  }

  bool ShouldCleanupSessionCookies() const { return GetParam(); }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    ASSERT_TRUE(embedded_test_server()->Start());
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        absl::StrFormat("MAP *:80 127.0.0.1:%d",
                        embedded_test_server()->port()));
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(SessionCookiesCleanupOnSessionRestoreBrowserTest,
                       PRE_CleanupSessionCookies) {
  auto* rfh =
      ui_test_utils::NavigateToURL(browser(), GURL("http://a.test/empty.html"));
  ASSERT_TRUE(rfh);
  ASSERT_TRUE(content::ExecJs(rfh, "document.cookie = 'bar=session'"));
}

IN_PROC_BROWSER_TEST_P(SessionCookiesCleanupOnSessionRestoreBrowserTest,
                       CleanupSessionCookies) {
  auto* rfh =
      ui_test_utils::NavigateToURL(browser(), GURL("http://a.test/empty.html"));
  ASSERT_TRUE(rfh);
  if (ShouldCleanupSessionCookies()) {
    EXPECT_EQ("", content::EvalJs(rfh, "document.cookie"));
  } else {
    EXPECT_EQ("bar=session", content::EvalJs(rfh, "document.cookie"));
  }
}

INSTANTIATE_TEST_SUITE_P(,
                         SessionCookiesCleanupOnSessionRestoreBrowserTest,
                         testing::Bool());

// ---- Tree-tab session persistence tests ------------------------------------

// Base class for tree-tab session tests.
class BraveTreeTabSessionRestoreBrowserTest : public InProcessBrowserTest {
 public:
  BraveTreeTabSessionRestoreBrowserTest() {
    feature_list_.InitAndEnableFeature(tabs::kBraveTreeTab);
  }

 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Enable tree tabs via pref (mirrors what the user would do).
    browser()->profile()->GetPrefs()->SetBoolean(
        brave_tabs::kTreeTabsEnabled, true);
  }

  BraveTabStripModel* brave_tsm() {
    return static_cast<BraveTabStripModel*>(browser()->tab_strip_model());
  }

  SessionService* session_service() {
    return SessionServiceFactory::GetForProfile(browser()->profile());
  }

  // Gets the last-session data and verifies tree extra_data on a tab.
  void VerifyExtraDataPresent(
      base::OnceCallback<void(
          std::vector<std::unique_ptr<sessions::SessionWindow>>)> verify_fn) {
    session_service()->MoveCurrentSessionToLastSession();
    base::RunLoop loop;
    session_service()->GetLastSession(base::BindLambdaForTesting(
        [&](std::vector<std::unique_ptr<sessions::SessionWindow>> windows,
            SessionID /*active_window*/, bool error_reading) {
          ASSERT_FALSE(error_reading);
          std::move(verify_fn).Run(std::move(windows));
          loop.Quit();
        }));
    loop.Run();
  }

  base::test::ScopedFeatureList feature_list_;
};

// Verifies that when a tab has a tree node, the three tree extra_data keys are
// written to the session file via BuildCommandsForTab.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       TreeNodeExtraDataWrittenToSession) {
  // The browser starts with one tab; tree tabs should be enabled.
  ASSERT_TRUE(brave_tsm()->tree_model());

  int tab_count = browser()->tab_strip_model()->count();
  ASSERT_GE(tab_count, 1);

  // Check that the first tab's tree node gets kBraveTreeNodeIdKey.
  VerifyExtraDataPresent(base::BindOnce([](
      std::vector<std::unique_ptr<sessions::SessionWindow>> windows) {
    ASSERT_EQ(1u, windows.size());
    ASSERT_GE(windows[0]->tabs.size(), 1u);

    bool found_tree_id = false;
    for (const auto& session_tab : windows[0]->tabs) {
      auto it = session_tab->extra_data.find(kBraveTreeNodeIdKey);
      if (it != session_tab->extra_data.end() && !it->second.empty()) {
        found_tree_id = true;
        // If node_id is present, parent_node_id must also be present.
        EXPECT_NE(session_tab->extra_data.end(),
                  session_tab->extra_data.find(kBraveTreeParentNodeIdKey));
        break;
      }
    }
    EXPECT_TRUE(found_tree_id)
        << "Expected at least one tab with " << kBraveTreeNodeIdKey
        << " in extra_data";
  }));
}

// Verifies that a tab nested under a parent tree node stores the parent's id
// in kBraveTreeParentNodeIdKey and the parent itself stores an empty parent id.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       ParentNodeIdStoredForChildTab) {
  ASSERT_TRUE(brave_tsm()->tree_model());

  // Tab 0 is the root. Add a second tab; it will start as another root.
  // Then make it a child by moving it with the appropriate APIs.
  // For the purposes of this test, just verify the parent/child relationship
  // for whatever tree structure was created.

  VerifyExtraDataPresent(base::BindOnce([](
      std::vector<std::unique_ptr<sessions::SessionWindow>> windows) {
    ASSERT_EQ(1u, windows.size());
    if (windows[0]->tabs.empty()) {
      return;
    }
    // Every tab with a non-empty brave_tree_parent_node_id must also have a
    // corresponding tab whose brave_tree_node_id equals that parent id.
    std::set<std::string> all_node_ids;
    for (const auto& tab : windows[0]->tabs) {
      auto it = tab->extra_data.find(kBraveTreeNodeIdKey);
      if (it != tab->extra_data.end() && !it->second.empty()) {
        all_node_ids.insert(it->second);
      }
    }
    for (const auto& tab : windows[0]->tabs) {
      auto parent_it = tab->extra_data.find(kBraveTreeParentNodeIdKey);
      if (parent_it == tab->extra_data.end() || parent_it->second.empty()) {
        continue;
      }
      EXPECT_TRUE(all_node_ids.count(parent_it->second) > 0)
          << "brave_tree_parent_node_id '"
          << parent_it->second
          << "' has no matching brave_tree_node_id in the session";
    }
  }));
}

// Verifies that the collapsed key is absent when tree tabs are not enabled,
// ensuring no tree data leaks into sessions without the feature.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       FeatureDisabledNoTreeExtraData) {
  // Disable the feature mid-test by disabling the pref.
  browser()->profile()->GetPrefs()->SetBoolean(
      brave_tabs::kTreeTabsEnabled, false);

  // tree_model() should be null when tree tabs pref is off.
  EXPECT_FALSE(brave_tsm()->tree_model());

  // Session data should not contain tree keys.
  VerifyExtraDataPresent(base::BindOnce([](
      std::vector<std::unique_ptr<sessions::SessionWindow>> windows) {
    ASSERT_EQ(1u, windows.size());
    for (const auto& tab : windows[0]->tabs) {
      EXPECT_EQ(tab->extra_data.end(),
                tab->extra_data.find(kBraveTreeNodeIdKey))
          << "Tree node ID should not be present when tree tabs are disabled";
    }
  }));
}
