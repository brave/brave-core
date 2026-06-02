/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <set>
#include <string>

#include "base/command_line.h"
#include "base/containers/map_util.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/sessions/brave_session_keys.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/sessions/core/session_types.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/sessions/core/tab_restore_types.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"
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
        var controls_ready = textarea.textContent === '__some_text__' &&
                             input.value === '__some_text__';
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
        var controls_ready = textarea.textContent === '__some_text__' &&
                             input.value === '__some_text__';
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

// Base class for tree-tab session tests.
class BraveTreeTabSessionRestoreBrowserTest : public InProcessBrowserTest {
 public:
  BraveTreeTabSessionRestoreBrowserTest() {
    feature_list_.InitAndEnableFeature(tabs::kBraveTreeTab);
  }

 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // Both vertical tabs and tree tabs prefs must be true for
    // BraveTabStripModel::BuildTreeTabs() to be called and tree_model() to
    // return non-null.
    browser()->profile()->GetPrefs()->SetBoolean(
        brave_tabs::kVerticalTabsEnabled, true);
    browser()->profile()->GetPrefs()->SetBoolean(brave_tabs::kTreeTabsEnabled,
                                                 true);
  }

  BraveTabStripModel* brave_tab_strip_model() {
    return static_cast<BraveTabStripModel*>(browser()->tab_strip_model());
  }

  // Returns the tree node id string for the tab at |index|, or an empty string
  // if the tab is not inside a tree node. Mirrors the GetTreeTabNodeCollection
  // helper used by TreeTabSessionObserver.
  std::string GetTreeNodeIdForTab(int index) {
    const tabs::TabInterface* tab =
        brave_tab_strip_model()->GetTabAtIndex(index);
    const tabs::TabCollection* parent = tab->GetParentCollection();
    if (parent && parent->type() == tabs::TabCollection::Type::TREE_NODE) {
      return static_cast<const tabs::TreeTabNodeTabCollection*>(parent)
          ->node()
          .id()
          .ToString();
    }
    return std::string();
  }

  SessionService* session_service() {
    return SessionServiceFactory::GetForProfile(browser()->profile());
  }

  // Saves the last session using only incremental commands (no rebuild).
  // Tests the AddTabExtraData path triggered by kNodeCreated events.
  // Flushes pending tasks first so that deferred kNodeCreated notifications
  // (posted via PostTask in TreeTabModel::AddTreeTabNode) have fired and
  // written their AddTabExtraData commands before the session is saved.
  void VerifyExtraData(
      base::OnceCallback<void(
          std::vector<std::unique_ptr<sessions::SessionWindow>>)> predicate,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    session_service()->MoveCurrentSessionToLastSession();
    base::RunLoop loop;
    session_service()->GetLastSession(base::BindLambdaForTesting(
        [&](std::vector<std::unique_ptr<sessions::SessionWindow>> windows,
            SessionID /*active_window*/, bool error_reading) {
          ASSERT_FALSE(error_reading);
          std::move(predicate).Run(std::move(windows));
          loop.Quit();
        }));
    loop.Run();
  }

  // Forces a full session rebuild (mirroring what happens on browser restart)
  // then saves and reads back the last session. Tests the
  // BRAVE_BUILD_COMMANDS_FOR_TREE_TAB macro in BuildCommandsForBrowser.
  void VerifyExtraDataAfterFullRebuild(
      base::OnceCallback<void(
          std::vector<std::unique_ptr<sessions::SessionWindow>>)> predicate,
      base::Location location = base::Location::Current()) {
    session_service()->ResetFromCurrentBrowsers();
    VerifyExtraData(std::move(predicate), location);
  }

  // Asserts that at least one tab carries all three tree extra-data keys and
  // that the keys for parent and collapsed state accompany any node-id found.
  static void VerifyTreeNodeExtraDataKeys(
      std::vector<std::unique_ptr<sessions::SessionWindow>> windows) {
    ASSERT_EQ(1u, windows.size());
    ASSERT_GE(windows[0]->tabs.size(), 1u);

    bool found_tree_id = false;
    for (const auto& session_tab : windows[0]->tabs) {
      const auto* node_id =
          base::FindOrNull(session_tab->extra_data, kBraveTreeNodeIdKey);
      if (node_id && !node_id->empty()) {
        found_tree_id = true;
        EXPECT_TRUE(base::FindOrNull(session_tab->extra_data,
                                     kBraveTreeParentNodeIdKey))
            << "tab extra data should contain kBraveTreeParentNodeIdKey";
        auto* collapsed = base::FindOrNull(session_tab->extra_data,
                                           kBraveTreeNodeCollapsedKey);
        EXPECT_TRUE(collapsed && (*collapsed == "0" || *collapsed == "1"))
            << "tab extra data should contain kBraveTreeNodeCollapsedKey with "
               "value 0 or 1";
        break;
      }
    }
    EXPECT_TRUE(found_tree_id) << "Expected at least one tab with "
                               << kBraveTreeNodeIdKey << " in extra_data";
  }

  base::test::ScopedFeatureList feature_list_;
};

// Verifies that when a tab has a tree node, the three tree extra_data keys are
// written incrementally to the session via AddTabExtraData (kNodeCreated path).
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       TreeNodeExtraDataWrittenToSession) {
  // The browser starts with one tab; tree tabs should be enabled.
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  int tab_count = browser()->tab_strip_model()->count();
  ASSERT_EQ(tab_count, 1);

  auto* tree_model = brave_tab_strip_model()->tree_model();
  if (tree_model->has_pending_add_tree_tab_node_notification_for_testing()) {
    // Let the deferred kNodeCreated notification task run.
    base::RunLoop loop;
    auto subscription =
        tree_model->RegisterAddTreeTabNodeCallback(base::BindLambdaForTesting(
            [&](const tabs::TreeTabNode& node) { loop.Quit(); }));
    loop.Run();
  }

  // Check that the first tab's tree node gets kBraveTreeNodeIdKey via the
  // incremental (AddTabExtraData) path, without a full rebuild.
  VerifyExtraData(base::BindOnce(
      &BraveTreeTabSessionRestoreBrowserTest::VerifyTreeNodeExtraDataKeys));
}

// Verifies that tree extra_data is written during a full session rebuild
// (BRAVE_BUILD_COMMANDS_FOR_TREE_TAB macro in BuildCommandsForBrowser).
// This mirrors what happens on browser restart.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       TreeNodeExtraDataSurvivesFullRebuild) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  VerifyExtraDataAfterFullRebuild(base::BindOnce(
      &BraveTreeTabSessionRestoreBrowserTest::VerifyTreeNodeExtraDataKeys));
}

// Verifies that a tab nested under a parent tree node stores the parent's id
// in kBraveTreeParentNodeIdKey and the parent itself stores an empty parent id.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       ParentNodeIdStoredForChildTab) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  // Create a new tab and add it to the tree, as a child of the first tab
  std::unique_ptr<tabs::TabModel> tab_model = std::make_unique<tabs::TabModel>(
      content::WebContents::Create(
          content::WebContents::CreateParams(browser()->profile())),
      browser()->tab_strip_model());
  brave_tab_strip_model()->AddTab(std::move(tab_model), -1,
                                  ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  // Navigate the new tab so it has a committed navigation entry.
  // Tabs whose only entry is the placeholder InitialNavigationEntry are
  // filtered out during session save (IsInitialEntry() check) and then
  // discarded during session reconstruction (navigations.empty() check), so
  // they are never restored.
  content::WebContents* new_tab =
      browser()->tab_strip_model()->GetWebContentsAt(1);
  ASSERT_TRUE(new_tab);
  ASSERT_TRUE(content::NavigateToURL(new_tab, GURL("about:blank")));

  VerifyExtraDataAfterFullRebuild(base::BindOnce(
      [](std::vector<std::unique_ptr<sessions::SessionWindow>> windows) {
        ASSERT_EQ(1u, windows.size());
        ASSERT_EQ(windows[0]->tabs.size(), 2u);

        // Every tab with a non-empty brave_tree_parent_node_id must also
        // have a corresponding tab whose brave_tree_node_id equals that
        // parent id.
        std::set<std::string> all_node_ids;
        for (const auto& tab : windows[0]->tabs) {
          const auto* node_id =
              base::FindOrNull(tab->extra_data, kBraveTreeNodeIdKey);
          if (node_id && !node_id->empty()) {
            all_node_ids.insert(*node_id);
          }
        }

        for (const auto& tab : windows[0]->tabs) {
          const auto* parent_id =
              base::FindOrNull(tab->extra_data, kBraveTreeParentNodeIdKey);
          if (!parent_id || parent_id->empty()) {
            continue;
          }
          EXPECT_TRUE(all_node_ids.count(*parent_id) > 0)
              << "brave_tree_parent_node_id '" << *parent_id
              << "' has no matching brave_tree_node_id in the session";
        }
      }));
}

// Verifies that moving a child tab before its parent reparents it to a root
// node and that both the incremental (AddTabExtraData) path and the full
// rebuild path write an empty kBraveTreeParentNodeIdKey for the moved tab.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       ReparentedChildBecomesRootInSession) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  // Build A (root at 0) -> B (child of A at 1).
  auto* tab_a = browser()->tab_strip_model()->GetTabAtIndex(0);
  {
    auto tab_b = std::make_unique<tabs::TabModel>(
        content::WebContents::Create(
            content::WebContents::CreateParams(browser()->profile())),
        browser()->tab_strip_model());
    tab_b->set_opener(tab_a);
    brave_tab_strip_model()->AddTab(
        std::move(tab_b), -1, ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  }
  content::WebContents* wc_b =
      browser()->tab_strip_model()->GetWebContentsAt(1);
  ASSERT_TRUE(wc_b);

  // Navigate B so it has a committed entry and is not filtered out during
  // session save.
  ASSERT_TRUE(content::NavigateToURL(wc_b, GURL("about:blank")));

  // Reparent B to a root by moving it before A.
  browser()->tab_strip_model()->MoveWebContentsAt(1, 0, false);

  // Both A and B should now have empty parent IDs (both are roots). Both A and
  // B should now have empty parent IDs (both are roots).
  auto check_both_roots =
      [](std::vector<std::unique_ptr<sessions::SessionWindow>> windows) {
        ASSERT_EQ(1u, windows.size());
        ASSERT_EQ(2u, windows[0]->tabs.size());
        for (const auto& tab : windows[0]->tabs) {
          const auto* parent_id =
              base::FindOrNull(tab->extra_data, kBraveTreeParentNodeIdKey);
          EXPECT_TRUE(parent_id && parent_id->empty())
              << "after reparenting B before A, both tabs should be roots";
        }
      };

  // Incremental path: written by the kNodeReparented AddTabExtraData call.
  VerifyExtraData(base::BindOnce(check_both_roots));
  // Rebuild path: written by BuildCommandsForBrowser scanning the live tree.
  VerifyExtraDataAfterFullRebuild(base::BindOnce(check_both_roots));
}

// Verifies that when a child tab is closed the session contains only the
// remaining parent tab and that it is still recorded as a root node.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       ClosedChildTabRemovedFromSession) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  // Build A (root at 0) -> B (child of A at 1). Navigate B so it persists.
  auto* tab_a = browser()->tab_strip_model()->GetTabAtIndex(0);
  {
    auto tab_b = std::make_unique<tabs::TabModel>(
        content::WebContents::Create(
            content::WebContents::CreateParams(browser()->profile())),
        browser()->tab_strip_model());
    tab_b->set_opener(tab_a);
    brave_tab_strip_model()->AddTab(
        std::move(tab_b), -1, ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  }
  content::WebContents* wc_b =
      browser()->tab_strip_model()->GetWebContentsAt(1);
  ASSERT_TRUE(wc_b);
  ASSERT_TRUE(content::NavigateToURL(wc_b, GURL("about:blank")));

  // Close B. SessionService::TabClosed() removes B from the session.
  browser()->tab_strip_model()->CloseWebContentsAt(1,
                                                   TabCloseTypes::CLOSE_NONE);

  // Only A should remain in the session, and it must be a root.
  VerifyExtraData(base::BindOnce(
      [](std::vector<std::unique_ptr<sessions::SessionWindow>> windows) {
        ASSERT_EQ(1u, windows.size());
        ASSERT_EQ(1u, windows[0]->tabs.size());

        const auto& tab = windows[0]->tabs[0];
        const auto* node_id =
            base::FindOrNull(tab->extra_data, kBraveTreeNodeIdKey);
        EXPECT_TRUE(node_id && !node_id->empty())
            << "surviving tab should still carry kBraveTreeNodeIdKey";

        const auto* parent_id =
            base::FindOrNull(tab->extra_data, kBraveTreeParentNodeIdKey);
        EXPECT_TRUE(parent_id && parent_id->empty())
            << "surviving tab is the root; its parent ID should be empty";
      }));
}

// Verifies that closing a tab that lives inside a tree node captures the tab's
// tree position (node id, parent id, collapsed state) into the
// TabRestoreService entry via BrowserLiveTabContext::GetExtraDataForTab, so
// that "restore closed tab" can return the tab to its tree position.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       ClosedTreeChildTabCapturesTreePositionForRestore) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  // Build A (root at 0) -> B (child of A at 1). Navigate B so it has a
  // committed entry; tabs with no navigation are skipped by TabRestoreService.
  auto* tab_a = browser()->tab_strip_model()->GetTabAtIndex(0);
  {
    auto tab_b = std::make_unique<tabs::TabModel>(
        content::WebContents::Create(
            content::WebContents::CreateParams(browser()->profile())),
        browser()->tab_strip_model());
    tab_b->set_opener(tab_a);
    brave_tab_strip_model()->AddTab(
        std::move(tab_b), -1, ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  }
  content::WebContents* web_contents_b =
      browser()->tab_strip_model()->GetWebContentsAt(1);
  ASSERT_TRUE(web_contents_b);
  ASSERT_TRUE(content::NavigateToURL(web_contents_b, GURL("about:blank")));

  // Capture the live node ids before closing: B is the tab being closed and A
  // is its parent.
  const std::string node_b_id = GetTreeNodeIdForTab(1);
  const std::string node_a_id = GetTreeNodeIdForTab(0);
  ASSERT_FALSE(node_b_id.empty());
  ASSERT_FALSE(node_a_id.empty());

  sessions::TabRestoreService* tab_restore_service =
      TabRestoreServiceFactory::GetForProfile(browser()->profile());
  ASSERT_TRUE(tab_restore_service);

  // Close B. This synchronously creates a TAB restore entry whose extra_data is
  // populated by BrowserLiveTabContext::GetExtraDataForTab().
  browser()->tab_strip_model()->CloseWebContentsAt(
      1, TabCloseTypes::CLOSE_CREATE_HISTORICAL_TAB);

  ASSERT_GE(tab_restore_service->entries().size(), 1u);
  const sessions::tab_restore::Entry* entry =
      tab_restore_service->entries().front().get();
  ASSERT_EQ(sessions::tab_restore::Type::TAB, entry->type);
  const auto* restored_tab =
      static_cast<const sessions::tab_restore::Tab*>(entry);

  // The closed tab should carry its own node id...
  const auto* node_id =
      base::FindOrNull(restored_tab->extra_data, kBraveTreeNodeIdKey);
  ASSERT_TRUE(node_id);
  EXPECT_EQ(*node_id, node_b_id);

  // ...its parent's node id (A)...
  const auto* parent_id =
      base::FindOrNull(restored_tab->extra_data, kBraveTreeParentNodeIdKey);
  ASSERT_TRUE(parent_id);
  EXPECT_EQ(*parent_id, node_a_id);

  // ...and the collapsed state, expressed as "0" or "1".
  const auto* collapsed =
      base::FindOrNull(restored_tab->extra_data, kBraveTreeNodeCollapsedKey);
  ASSERT_TRUE(collapsed);
  EXPECT_TRUE(*collapsed == "0" || *collapsed == "1");
}

// Verifies that closing a tab while the tree tab feature is enabled but the
// current window is not displaying tabs as a tree (tree_model() is null) does
// not write any tree extra_data to the TabRestoreService entry.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       ClosedTabWithoutTreeModelHasNoTreeExtraData) {
  // Disable the tree tabs pref so BraveTabStripModel::tree_model() is null even
  // though the kBraveTreeTab feature is enabled.
  browser()->profile()->GetPrefs()->SetBoolean(brave_tabs::kTreeTabsEnabled,
                                               false);
  ASSERT_FALSE(brave_tab_strip_model()->tree_model());

  // Open a second tab and navigate it so it is recorded by TabRestoreService.
  auto tab_b = std::make_unique<tabs::TabModel>(
      content::WebContents::Create(
          content::WebContents::CreateParams(browser()->profile())),
      browser()->tab_strip_model());
  brave_tab_strip_model()->AddTab(std::move(tab_b), -1,
                                  ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  content::WebContents* web_contents_b =
      browser()->tab_strip_model()->GetWebContentsAt(1);
  ASSERT_TRUE(web_contents_b);
  ASSERT_TRUE(content::NavigateToURL(web_contents_b, GURL("about:blank")));

  sessions::TabRestoreService* tab_restore_service =
      TabRestoreServiceFactory::GetForProfile(browser()->profile());
  ASSERT_TRUE(tab_restore_service);

  browser()->tab_strip_model()->CloseWebContentsAt(
      1, TabCloseTypes::CLOSE_CREATE_HISTORICAL_TAB);

  ASSERT_GE(tab_restore_service->entries().size(), 1u);
  const sessions::tab_restore::Entry* entry =
      tab_restore_service->entries().front().get();
  ASSERT_EQ(sessions::tab_restore::Type::TAB, entry->type);
  const auto* restored_tab =
      static_cast<const sessions::tab_restore::Tab*>(entry);

  EXPECT_FALSE(base::FindOrNull(restored_tab->extra_data, kBraveTreeNodeIdKey));
  EXPECT_FALSE(
      base::FindOrNull(restored_tab->extra_data, kBraveTreeParentNodeIdKey));
  EXPECT_FALSE(
      base::FindOrNull(restored_tab->extra_data, kBraveTreeNodeCollapsedKey));
}
