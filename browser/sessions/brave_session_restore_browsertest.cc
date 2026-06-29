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
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/sessions/brave_session_keys.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/tab_group_sync/tab_group_sync_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/split_tab_metrics.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/saved_tab_groups/public/tab_group_sync_service.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/sessions/core/session_types.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/sessions/core/tab_restore_types.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "components/tabs/public/split_tab_collection.h"
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
  // if the tab is not inside a tree node.
  std::string GetTreeNodeIdForTab(int index) {
    const tabs::TreeTabNodeTabCollection* tree_collection =
        GetTreeCollectionForTab(index);
    return tree_collection ? tree_collection->node().id().ToString()
                           : std::string();
  }

  // Returns the TreeTabNodeTabCollection for the tab at |index|, or nullptr.
  // Convenience overload for the default browser().
  const tabs::TreeTabNodeTabCollection* GetTreeCollectionForTab(int index) {
    return tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(
        brave_tab_strip_model()->GetTabAtIndex(index));
  }

  // Returns the nearest ancestor TreeTabNodeTabCollection for the tab at
  // |index|, traversing up through group/split parents if needed. Unlike
  // GetTreeCollectionForTab, this works for tabs inside groups or splits.
  const tabs::TreeTabNodeTabCollection* GetNearestTreeCollectionForTab(
      int index) {
    return tabs::TreeTabNodeTabCollection::GetNearestTreeTabNodeCollection(
        brave_tab_strip_model()->GetTabAtIndex(index));
  }

  // Creates a new WebContents for the test browser's profile.
  std::unique_ptr<content::WebContents> CreateWebContents() {
    return content::WebContents::Create(
        content::WebContents::CreateParams(browser()->profile()));
  }

  // Adds a new tab at the end, optionally sets |opener|, and navigates it.
  void AddTabWithOptionalOpenerAndNavigate(
      const GURL& url,
      tabs::TabInterface* opener = nullptr) {
    auto tab_model = std::make_unique<tabs::TabModel>(
        CreateWebContents(), browser()->tab_strip_model());
    if (opener) {
      tab_model->set_opener(opener);
    }
    brave_tab_strip_model()->AddTab(
        std::move(tab_model), -1, ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetWebContentsAt(
            browser()->tab_strip_model()->count() - 1);
    ASSERT_TRUE(web_contents);
    ASSERT_TRUE(content::NavigateToURL(web_contents, url));
  }

  // Initializes the TabGroupSyncService as ready (required before calling
  // AddToNewGroup or any other tab-group API that checks IsInitialized()).
  void EnsureTabGroupSyncServiceInitialized(
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    auto* service = tab_groups::TabGroupSyncServiceFactory::GetForProfile(
        browser()->profile());
    ASSERT_TRUE(service);
    service->SetIsInitializedForTesting(true);
  }

  // Creates a split from the tabs at |index_a| and |index_b| in the model.
  // Activates |index_a| first, then calls AddToNewSplit with |index_b|.
  void CreateSplitWithTabs(int index_a, int index_b) {
    ASSERT_NE(index_a, index_b);
    brave_tab_strip_model()->ActivateTabAt(index_a);
    brave_tab_strip_model()->AddToNewSplit(
        {index_b}, split_tabs::SplitTabVisualData(),
        split_tabs::SplitTabCreatedSource::kTabContextMenu);
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
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_a);

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
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_a);

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
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_a);

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
  EXPECT_TRUE(*collapsed == "0" || *collapsed == "1")
      << "tab extra data should contain kBraveTreeNodeCollapsedKey with value "
         "0 "
         "or 1, but was: "
      << *collapsed;
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

// Verifies that closing a child tree tab and restoring it via Ctrl+Shift+T
// (ReplaceRestoredTab path) puts the tab back under its original parent node.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       RestoreClosedTreeChildTabRestoresHierarchy) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  // Build A (root at 0) -> B (child of A at 1).
  auto* tab_a_iface = browser()->tab_strip_model()->GetTabAtIndex(0);
  // Navigate B so TabRestoreService records it (tabs with no committed entry
  // are skipped during capture).
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_a_iface);

  // Verify the initial hierarchy before closing.
  const auto* coll_a = GetTreeCollectionForTab(0);
  const auto* coll_b = GetTreeCollectionForTab(1);
  ASSERT_TRUE(coll_a && coll_b);
  ASSERT_EQ(coll_b->GetParentCollection(), coll_a)
      << "B should be nested under A before close";

  // Record A's node ID — it must survive the close+restore cycle unchanged.
  const std::string node_a_id = coll_a->node().id().ToString();

  // Close B (CLOSE_CREATE_HISTORICAL_TAB populates TabRestoreService).
  browser()->tab_strip_model()->CloseWebContentsAt(
      1, TabCloseTypes::CLOSE_CREATE_HISTORICAL_TAB);
  ASSERT_EQ(1, browser()->tab_strip_model()->count());

  // Restore B via chrome::RestoreTab (Ctrl+Shift+T equivalent).
  ui_test_utils::TabAddedWaiter tab_added_waiter(browser());
  chrome::RestoreTab(browser());
  tab_added_waiter.Wait();
  ASSERT_EQ(2, browser()->tab_strip_model()->count());

  // After restoration, B's tree node must still be nested under A's tree node.
  const auto* restored_coll_b = GetTreeCollectionForTab(1);
  ASSERT_TRUE(restored_coll_b) << "Restored tab B must be inside a tree node";
  EXPECT_EQ(restored_coll_b->GetParentCollection(), coll_a)
      << "Restored B must be nested under A's tree node";
}

// Fixture for tree-tab tests that exercise real session restore across a
// browser relaunch via PRE_ tests. Setting the startup preference to LAST makes
// the next launch restore the previous session's tabs.
class BraveTreeTabSessionRestoreOnRelaunchBrowserTest
    : public BraveTreeTabSessionRestoreBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    BraveTreeTabSessionRestoreBrowserTest::SetUpOnMainThread();
    SessionStartupPref::SetStartupPref(
        browser()->profile(), SessionStartupPref(SessionStartupPref::LAST));
  }
};

// Builds a parent-child tree tab hierarchy that the following (non-PRE_) test
// verifies after a real browser relaunch. Forcing a full session rebuild
// captures the tree extra-data on disk so the next launch can reconstruct the
// hierarchy via the AddRestoredTab path (the same path as session restore on
// browser restart).
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreOnRelaunchBrowserTest,
                       PRE_RestoreClosedWindowPreservesTreeTabHierarchy) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  // Navigate tab A so the session records a committed entry for it.
  content::WebContents* wc_a =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  ASSERT_TRUE(content::NavigateToURL(wc_a, GURL("about:blank")));

  // Build A (root at 0) -> B (child of A at 1).
  auto* tab_a_iface = browser()->tab_strip_model()->GetTabAtIndex(0);
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_a_iface);

  // Verify initial hierarchy before relaunch.
  const auto* coll_a = GetTreeCollectionForTab(0);
  const auto* coll_b = GetTreeCollectionForTab(1);
  ASSERT_TRUE(coll_a && coll_b);
  ASSERT_EQ(coll_b->GetParentCollection(), coll_a);

  // Force a full session rebuild so the tree extra-data is captured on disk
  // (BuildCommandsForBrowser scans the live tree) regardless of any pending
  // deferred kNodeCreated notifications. The session is restored on the next
  // launch via the SessionStartupPref::LAST set in SetUpOnMainThread().
  session_service()->ResetFromCurrentBrowsers();
}

// On relaunch the previous session is restored. AddRestoredTab is called for
// each tab, exercising the same MaybeRestoreTabTreeHierarchy path as session
// restore on browser restart; the tree hierarchy (B nested under A) must be
// reconstructed.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreOnRelaunchBrowserTest,
                       RestoreClosedWindowPreservesTreeTabHierarchy) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  auto* restored_model = brave_tab_strip_model();

  EXPECT_EQ(restored_model->count(), 3)
      << "Restored window should have two more tabs";

  const auto* restored_coll_a = GetTreeCollectionForTab(0);
  const auto* restored_coll_b = GetTreeCollectionForTab(1);
  ASSERT_TRUE(restored_coll_a)
      << "Tab A must be inside a tree node in the restored window";
  ASSERT_TRUE(restored_coll_b)
      << "Tab B must be inside a tree node in the restored window";
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return restored_coll_b->GetParentCollection() == restored_coll_a;
  })) << "Restored B must be nested under A's tree node";
}

// ── Group / Split hierarchy restoration tests ───────────────────────────────
//
// These three PRE_/main pairs verify that MaybeRestoreGroupTreeHierarchy and
// MaybeRestoreSplitTreeHierarchy correctly reconnect nested tree nodes after a
// full browser relaunch (the AddRestoredTab path).  Each PRE_ test builds a
// hierarchy, forces a full session rebuild, and the main test confirms the
// hierarchy is intact after relaunch.

// Scenario: tab A (root) → tree_node_G [group(tab_b, tab_c)]
// Tests MaybeRestoreGroupTreeHierarchy: the group's tree node must be re-
// nested under A's tree node after a browser restart.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreOnRelaunchBrowserTest,
                       PRE_RestoreClosedWindowPreservesGroupInTreeHierarchy) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());
  EnsureTabGroupSyncServiceInitialized();

  // Navigate tab A so the session records a committed entry for it.
  content::WebContents* wc_a =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  ASSERT_TRUE(content::NavigateToURL(wc_a, GURL("about:blank")));

  // Build: A (root) → B (child of A) and C (child of A).
  auto* tab_a_iface = browser()->tab_strip_model()->GetTabAtIndex(0);
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_a_iface);

  auto* tab_b_iface = browser()->tab_strip_model()->GetTabAtIndex(1);
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_b_iface);

  // Group B and C → tree_node_G wraps group_G, becomes a child of tree_node_A.
  tab_groups::TabGroupId group_id =
      brave_tab_strip_model()->AddToNewGroup({1, 2});
  ASSERT_TRUE(
      brave_tab_strip_model()->group_model()->ContainsTabGroup(group_id));

  // Verify the initial hierarchy: group's tree_node is a child of tree_node_A.
  const auto* coll_a = GetTreeCollectionForTab(0);
  ASSERT_TRUE(coll_a) << "tab_a must be in a tree node";
  for (int i = 1; i <= 2; ++i) {
    const tabs::TabCollection* group_coll =
        brave_tab_strip_model()->GetTabAtIndex(i)->GetParentCollection();
    ASSERT_EQ(group_coll->type(), tabs::TabCollection::Type::GROUP);
    const tabs::TabCollection* group_tree_node =
        group_coll->GetParentCollection();
    ASSERT_EQ(group_tree_node->type(), tabs::TabCollection::Type::TREE_NODE);
    ASSERT_EQ(group_tree_node->GetParentCollection(), coll_a)
        << "group's tree_node must be a child of tree_node_A before relaunch";
  }

  session_service()->ResetFromCurrentBrowsers();
}

IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreOnRelaunchBrowserTest,
                       RestoreClosedWindowPreservesGroupInTreeHierarchy) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  // Restored window: tab_a (0), tab_b in group (1), tab_c in group (2),
  // plus the new-tab that is opened on start.
  ASSERT_GE(brave_tab_strip_model()->count(), 3)
      << "Restored window should have at least three tabs";

  const auto* restored_coll_a = GetTreeCollectionForTab(0);
  ASSERT_TRUE(restored_coll_a) << "Tab A must be in a tree node after restore";

  ASSERT_EQ(brave_tab_strip_model()->GetTabAtIndex(1)->GetParentCollection(),
            brave_tab_strip_model()->GetTabAtIndex(2)->GetParentCollection());

  // Tab B must still be inside a group.
  const tabs::TabCollection* restored_group =
      brave_tab_strip_model()->GetTabAtIndex(1)->GetParentCollection();
  ASSERT_EQ(restored_group->type(), tabs::TabCollection::Type::GROUP)
      << "Tab B must be inside a group after restore";

  // The group's tree_node must be a child of tree_node_A.
  const tabs::TabCollection* restored_group_tree_node =
      restored_group->GetParentCollection();
  ASSERT_EQ(restored_group_tree_node->type(),
            tabs::TabCollection::Type::TREE_NODE)
      << "Group must be wrapped in a tree node after restore";

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return restored_group_tree_node->GetParentCollection() == restored_coll_a;
  })) << "Restored group's tree_node must be nested under A's tree_node";
}

// Scenario: tab A (root) → tree_node_S [split(tab_b, tab_c)]
// Tests MaybeRestoreSplitTreeHierarchy: the split's tree node must be re-
// nested under A's tree node after a browser restart.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreOnRelaunchBrowserTest,
                       PRE_RestoreClosedWindowPreservesSplitInTreeHierarchy) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  // Navigate tab A.
  content::WebContents* wc_a =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  ASSERT_TRUE(content::NavigateToURL(wc_a, GURL("about:blank")));

  // Build: A (root) → B (child of A) and C (child of A).
  auto* tab_a_iface = browser()->tab_strip_model()->GetTabAtIndex(0);
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_a_iface);

  auto* tab_b_iface = browser()->tab_strip_model()->GetTabAtIndex(1);
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_b_iface);

  // Split B and C → tree_node_S wraps split_S, becomes a child of tree_node_A.
  CreateSplitWithTabs(1, 2);

  // Verify the initial hierarchy: split's tree_node is a child of tree_node_A.
  const auto* coll_a = GetTreeCollectionForTab(0);
  ASSERT_TRUE(coll_a) << "tab_a must be in a tree node";
  for (int i = 1; i <= 2; ++i) {
    const tabs::TabCollection* split_coll =
        brave_tab_strip_model()->GetTabAtIndex(i)->GetParentCollection();
    ASSERT_EQ(split_coll->type(), tabs::TabCollection::Type::SPLIT);
    const tabs::TabCollection* split_tree_node =
        split_coll->GetParentCollection();
    ASSERT_EQ(split_tree_node->type(), tabs::TabCollection::Type::TREE_NODE);
    ASSERT_EQ(split_tree_node->GetParentCollection(), coll_a)
        << "split's tree_node must be a child of tree_node_A before relaunch";
  }

  session_service()->ResetFromCurrentBrowsers();
}

IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreOnRelaunchBrowserTest,
                       RestoreClosedWindowPreservesSplitInTreeHierarchy) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  ASSERT_GE(brave_tab_strip_model()->count(), 3)
      << "Restored window should have at least three tabs";

  const auto* restored_coll_a = GetTreeCollectionForTab(0);
  ASSERT_TRUE(restored_coll_a) << "Tab A must be in a tree node after restore";
  ASSERT_EQ(brave_tab_strip_model()->GetTabAtIndex(1)->GetParentCollection(),
            brave_tab_strip_model()->GetTabAtIndex(2)->GetParentCollection());

  // Tab B must still be inside a split.
  const tabs::TabCollection* restored_split =
      brave_tab_strip_model()->GetTabAtIndex(1)->GetParentCollection();
  ASSERT_EQ(restored_split->type(), tabs::TabCollection::Type::SPLIT)
      << "Tab B must be inside a split after restore";

  // The split's tree_node must be a child of tree_node_A.
  const tabs::TabCollection* restored_split_tree_node =
      restored_split->GetParentCollection();
  ASSERT_EQ(restored_split_tree_node->type(),
            tabs::TabCollection::Type::TREE_NODE)
      << "Split must be wrapped in a tree node after restore";

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return restored_split_tree_node->GetParentCollection() == restored_coll_a;
  })) << "Restored split's tree_node must be nested under A's tree_node";
}

// Scenario: tab A (root) → tree_node_G [group(split(tab_b, tab_c))]
// Tests that when a group contains a split, the group's tree node is correctly
// re-nested under A's tree node after a browser restart.
IN_PROC_BROWSER_TEST_F(
    BraveTreeTabSessionRestoreOnRelaunchBrowserTest,
    PRE_RestoreClosedWindowPreservesGroupWithSplitInTreeHierarchy) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());
  EnsureTabGroupSyncServiceInitialized();

  // Navigate tab A.
  content::WebContents* wc_a =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  ASSERT_TRUE(content::NavigateToURL(wc_a, GURL("about:blank")));

  // Build: A (root) → B (child of A) and C (child of B).
  auto* tab_a_iface = browser()->tab_strip_model()->GetTabAtIndex(0);
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_a_iface);

  auto* tab_b_iface = browser()->tab_strip_model()->GetTabAtIndex(1);
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_b_iface);

  // Group B and C → tree_node_G (child of tree_node_A) wraps group_G.
  tab_groups::TabGroupId group_id =
      brave_tab_strip_model()->AddToNewGroup({1, 2});
  ASSERT_TRUE(
      brave_tab_strip_model()->group_model()->ContainsTabGroup(group_id));

  // Verify the group's tree_node is a child of tree_node_A.
  const auto* coll_a = GetTreeCollectionForTab(0);
  ASSERT_TRUE(coll_a);

  const tabs::TabCollection* group_coll =
      brave_tab_strip_model()->GetTabAtIndex(1)->GetParentCollection();
  ASSERT_EQ(group_coll->type(), tabs::TabCollection::Type::GROUP);
  const tabs::TabCollection* group_tree_node =
      group_coll->GetParentCollection();
  ASSERT_EQ(group_tree_node->type(), tabs::TabCollection::Type::TREE_NODE);
  ASSERT_EQ(group_tree_node->GetParentCollection(), coll_a);

  // Split B and C inside the group → split_S becomes a direct child of group_G.
  CreateSplitWithTabs(1, 2);

  // Verify the split is now inside the group.
  const tabs::TabCollection* split_coll =
      brave_tab_strip_model()->GetTabAtIndex(1)->GetParentCollection();
  ASSERT_EQ(split_coll->type(), tabs::TabCollection::Type::SPLIT);
  ASSERT_EQ(split_coll->GetParentCollection(), group_coll)
      << "split must be a direct child of the group";

  // The nearest tree_node for tabs inside the split is still tree_node_G.
  ASSERT_EQ(GetNearestTreeCollectionForTab(1), group_tree_node);
  ASSERT_EQ(GetNearestTreeCollectionForTab(2), group_tree_node);

  session_service()->ResetFromCurrentBrowsers();
}

IN_PROC_BROWSER_TEST_F(
    BraveTreeTabSessionRestoreOnRelaunchBrowserTest,
    RestoreClosedWindowPreservesGroupWithSplitInTreeHierarchy) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  ASSERT_GE(brave_tab_strip_model()->count(), 3)
      << "Restored window should have at least three tabs";

  const auto* restored_coll_a = GetTreeCollectionForTab(0);
  ASSERT_TRUE(restored_coll_a) << "Tab A must be in a tree node after restore";

  // Tab B must be inside a group (tabs inside a split inside a group are
  // restored to the group's level; the split may be re-created separately).
  const tabs::TreeTabNodeTabCollection* nearest_b =
      GetNearestTreeCollectionForTab(1);
  ASSERT_TRUE(nearest_b)
      << "Tab B must be inside a tree node (via group) after restore";

  // The group's tree_node (nearest ancestor of B) must be under tree_node_A.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return nearest_b->GetParentCollection() == restored_coll_a;
  })) << "Restored group's tree_node must be nested under A's tree_node";
}

// ── Restore-closed-tab hierarchy tests ──────────────────────────────────────
//
// These tests verify the Ctrl+Shift+T (TabRestoreService) path for group and
// split tabs.  The hierarchy must be preserved when a tab inside a grouped or
// split tree node is closed and then re-opened.

// Closing one tab from a group that is nested under tree_node_A and then
// restoring it via chrome::RestoreTab must keep the group's tree_node under A.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       RestoreClosedGroupTabRestoresGroupHierarchy) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());
  EnsureTabGroupSyncServiceInitialized();

  // Build: A (root) → B (child of A) and C (child of B).
  auto* tab_a_iface = browser()->tab_strip_model()->GetTabAtIndex(0);
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_a_iface);

  auto* tab_b_iface = browser()->tab_strip_model()->GetTabAtIndex(1);
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_b_iface);

  // Group B and C → tree_node_G child of tree_node_A.
  tab_groups::TabGroupId group_id =
      brave_tab_strip_model()->AddToNewGroup({1, 2});
  ASSERT_TRUE(
      brave_tab_strip_model()->group_model()->ContainsTabGroup(group_id));

  // Capture the collection pointers before closing.
  const auto* coll_a = GetTreeCollectionForTab(0);
  const tabs::TabCollection* group_coll =
      brave_tab_strip_model()->GetTabAtIndex(1)->GetParentCollection();
  ASSERT_EQ(group_coll->type(), tabs::TabCollection::Type::GROUP);
  const tabs::TabCollection* group_tree_node =
      group_coll->GetParentCollection();
  ASSERT_EQ(group_tree_node->type(), tabs::TabCollection::Type::TREE_NODE);
  ASSERT_EQ(group_tree_node->GetParentCollection(), coll_a)
      << "group's tree_node must be a child of tree_node_A before close";

  // Close tab B (C remains, so the group and its tree_node survive).
  browser()->tab_strip_model()->CloseWebContentsAt(
      1, TabCloseTypes::CLOSE_CREATE_HISTORICAL_TAB);
  ASSERT_EQ(2, browser()->tab_strip_model()->count());

  // Restore tab B via chrome::RestoreTab (Ctrl+Shift+T equivalent).
  ui_test_utils::TabAddedWaiter tab_added_waiter(browser());
  chrome::RestoreTab(browser());
  tab_added_waiter.Wait();
  ASSERT_EQ(3, browser()->tab_strip_model()->count());

  // After restoration, the group's tree_node must still be under tree_node_A.
  const tabs::TabCollection* restored_group =
      brave_tab_strip_model()->GetTabAtIndex(1)->GetParentCollection();
  ASSERT_EQ(restored_group->type(), tabs::TabCollection::Type::GROUP)
      << "Restored tab B must be inside a group";
  const tabs::TabCollection* restored_group_tree_node =
      restored_group->GetParentCollection();
  ASSERT_EQ(restored_group_tree_node->type(),
            tabs::TabCollection::Type::TREE_NODE)
      << "Group must be wrapped in a tree node";
  EXPECT_EQ(restored_group_tree_node->GetParentCollection(), coll_a)
      << "Restored group's tree_node must remain nested under A's tree_node";
}

// Closing one tab from a split that is nested under tree_node_A dissolves the
// split.  After restoring the tab, its new tree node (or the re-created split's
// tree node) must still be nested under tree_node_A.
IN_PROC_BROWSER_TEST_F(BraveTreeTabSessionRestoreBrowserTest,
                       RestoreClosedSplitTabRestoresHierarchy) {
  ASSERT_TRUE(brave_tab_strip_model()->tree_model());

  // Build: A (root) → B (child of A) and C (child of B).
  auto* tab_a_iface = browser()->tab_strip_model()->GetTabAtIndex(0);
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_a_iface);

  auto* tab_b_iface = browser()->tab_strip_model()->GetTabAtIndex(1);
  AddTabWithOptionalOpenerAndNavigate(GURL("about:blank"), tab_b_iface);

  // Split B and C → tree_node_S child of tree_node_A.
  CreateSplitWithTabs(1, 2);

  const auto* coll_a = GetTreeCollectionForTab(0);
  ASSERT_TRUE(coll_a);
  const tabs::TabCollection* split_coll =
      brave_tab_strip_model()->GetTabAtIndex(1)->GetParentCollection();
  ASSERT_EQ(split_coll->type(), tabs::TabCollection::Type::SPLIT);
  const tabs::TabCollection* split_tree_node =
      split_coll->GetParentCollection();
  ASSERT_EQ(split_tree_node->type(), tabs::TabCollection::Type::TREE_NODE);
  ASSERT_EQ(split_tree_node->GetParentCollection(), coll_a)
      << "split's tree_node must be a child of tree_node_A before close";

  // Close tab B. Closing one tab of a split dissolves the split; C remains as
  // a standalone tab wrapped in its own tree node.
  browser()->tab_strip_model()->CloseWebContentsAt(
      1, TabCloseTypes::CLOSE_CREATE_HISTORICAL_TAB);
  ASSERT_EQ(2, browser()->tab_strip_model()->count());

  // Restore tab B.
  ui_test_utils::TabAddedWaiter tab_added_waiter(browser());
  chrome::RestoreTab(browser());
  tab_added_waiter.Wait();
  ASSERT_EQ(3, browser()->tab_strip_model()->count());

  // The restored tab B's nearest tree node must be under tree_node_A.
  // B is restored as a standalone tree node (the split was dissolved when B
  // was closed), so MaybeRestoreTabTreeHierarchy puts it back under A.
  const tabs::TreeTabNodeTabCollection* restored_nearest_b =
      GetNearestTreeCollectionForTab(1);
  ASSERT_TRUE(restored_nearest_b)
      << "Restored tab B must be inside a tree node";
  EXPECT_EQ(restored_nearest_b->GetParentCollection(), coll_a)
      << "Restored tab B's tree node must be nested under A's tree_node";
}
