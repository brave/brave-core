/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspaces/workspace_service.h"

#include <optional>

#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/uuid.h"
#include "brave/browser/workspaces/features.h"
#include "brave/browser/workspaces/workspace_metadata.h"
#include "brave/browser/workspaces/workspace_service_factory.h"
#include "chrome/browser/ui/browser_init_state.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/base_window.h"
#include "url/gurl.h"

// Covers the behaviour a workspace switch has to get right: tabs are never
// rebuilt or lost, a workspace is released when it leaves the screen, and
// workspace windows stay out of Chromium's session.
class WorkspaceServiceBrowserTest : public InProcessBrowserTest {
 public:
  WorkspaceServiceBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kWorkspaces);
  }
  ~WorkspaceServiceBrowserTest() override = default;

  WorkspaceService* service() {
    return WorkspaceServiceFactory::GetForProfile(browser()->GetProfile());
  }

  // Returns the window hosting |id|, or nullptr when it has none.
  BrowserWindowInterface* WindowFor(const base::Uuid& id) {
    std::optional<SessionID> window_id = service()->GetWindowForWorkspace(id);
    return window_id ? BrowserWindowInterface::FromSessionID(*window_id)
                     : nullptr;
  }

  bool IsVisible(BrowserWindowInterface* window) {
    return window->GetWindow()->IsVisible();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Switching between spaces must not rebuild tabs. The whole point of swapping
// window visibility rather than moving tabs is that a tab keeps its live
// state -- scroll position, media, form contents -- which only holds while the
// same WebContents stays alive.
IN_PROC_BROWSER_TEST_F(WorkspaceServiceBrowserTest,
                       SwitchingKeepsTheSameWebContents) {
  ASSERT_TRUE(service());
  content::WebContents* unmanaged_tab =
      browser()->GetTabStripModel()->GetWebContentsAt(0);
  ASSERT_TRUE(unmanaged_tab);

  WorkspaceMetadata workspace =
      service()->CreateWorkspaceInWindow("Work", browser());
  BrowserWindowInterface* workspace_window = WindowFor(workspace.id);
  ASSERT_TRUE(workspace_window);
  content::WebContents* workspace_tab =
      workspace_window->GetTabStripModel()->GetWebContentsAt(0);
  ASSERT_TRUE(workspace_tab);

  // Back to the unmanaged window, then into the workspace again.
  service()->SwitchWindowToUnmanaged(workspace_window);
  service()->SwitchWindowToWorkspace(workspace.id, browser());

  EXPECT_EQ(browser()->GetTabStripModel()->GetWebContentsAt(0), unmanaged_tab)
      << "The unmanaged tab was rebuilt instead of being kept alive";
  EXPECT_EQ(workspace_window->GetTabStripModel()->GetWebContentsAt(0),
            workspace_tab)
      << "The workspace tab was rebuilt instead of being kept alive";
}

// Creating a space must not disturb the tabs already open. They stay in the
// window they were in, which is hidden rather than emptied.
IN_PROC_BROWSER_TEST_F(WorkspaceServiceBrowserTest,
                       CreatingASpaceKeepsTheTabsAlreadyOpen) {
  ASSERT_TRUE(service());
  chrome::AddTabAt(browser(), GURL("about:blank"), /*index=*/-1,
                   /*foreground=*/false);
  chrome::AddTabAt(browser(), GURL("about:blank"), /*index=*/-1,
                   /*foreground=*/false);
  const int tabs_before = browser()->GetTabStripModel()->count();
  ASSERT_EQ(tabs_before, 3);

  WorkspaceMetadata workspace =
      service()->CreateWorkspaceInWindow("Work", browser());

  EXPECT_EQ(browser()->GetTabStripModel()->count(), tabs_before)
      << "Creating a space took tabs away from the window it was made from";
  EXPECT_FALSE(IsVisible(browser()));

  BrowserWindowInterface* workspace_window = WindowFor(workspace.id);
  ASSERT_TRUE(workspace_window);
  EXPECT_TRUE(IsVisible(workspace_window));
  EXPECT_EQ(workspace_window->GetTabStripModel()->count(), 1)
      << "A new space should start with a single tab";
}

// A workspace is held only while it is on screen. Once the window showing it
// is hidden, another window has to be able to pick it up again.
IN_PROC_BROWSER_TEST_F(WorkspaceServiceBrowserTest,
                       SwitchingAwayReleasesTheWorkspace) {
  ASSERT_TRUE(service());
  const SessionID unmanaged_id = browser()->GetSessionID();

  WorkspaceMetadata workspace =
      service()->CreateWorkspaceInWindow("Work", browser());
  BrowserWindowInterface* workspace_window = WindowFor(workspace.id);
  ASSERT_TRUE(workspace_window);
  ASSERT_TRUE(
      service()->IsWorkspaceOnScreenElsewhere(workspace.id, unmanaged_id));

  service()->SwitchWindowToUnmanaged(workspace_window);

  EXPECT_FALSE(
      service()->IsWorkspaceOnScreenElsewhere(workspace.id, unmanaged_id))
      << "A workspace that is no longer on screen is still being held";
}

// Closing the last tab of a space closes its window. That must not be read as
// a request to quit: the window the space was opened from comes back instead.
IN_PROC_BROWSER_TEST_F(WorkspaceServiceBrowserTest,
                       ClosingTheLastTabOfASpaceReturnsToTheUnmanagedWindow) {
  ASSERT_TRUE(service());
  const SessionID unmanaged_id = browser()->GetSessionID();

  WorkspaceMetadata workspace =
      service()->CreateWorkspaceInWindow("Work", browser());
  BrowserWindowInterface* workspace_window = WindowFor(workspace.id);
  ASSERT_TRUE(workspace_window);
  ASSERT_FALSE(IsVisible(browser()));

  // Closing the only tab takes the window with it.
  ui_test_utils::BrowserDestroyedObserver destroyed(workspace_window);
  workspace_window->GetTabStripModel()->CloseAllTabs();
  destroyed.Wait();

  // Let any retire-everything task posted by OnBrowserClosed run, so a
  // regression here shows up as a closed window rather than a race.
  base::RunLoop().RunUntilIdle();

  // Looked up by id: if this regresses, the unmanaged window is gone and
  // holding a pointer to it would be a use-after-free.
  BrowserWindowInterface* home =
      BrowserWindowInterface::FromSessionID(unmanaged_id);
  ASSERT_TRUE(home) << "Closing a space also closed the window it was opened "
                       "from, which quits the browser";
  EXPECT_TRUE(IsVisible(home))
      << "The window the space was opened from should be showing again";
}

// Workspaces persist themselves. Letting Chromium record their windows too
// means "continue where you left off" restores a workspace as loose tabs.
IN_PROC_BROWSER_TEST_F(WorkspaceServiceBrowserTest,
                       WorkspaceWindowsAreOmittedFromSessionRestore) {
  ASSERT_TRUE(service());
  WorkspaceMetadata workspace =
      service()->CreateWorkspaceInWindow("Work", browser());
  BrowserWindowInterface* workspace_window = WindowFor(workspace.id);
  ASSERT_TRUE(workspace_window);

  EXPECT_TRUE(
      BrowserInitState::From(workspace_window)->omit_from_session_restore())
      << "Workspace windows must not be tracked by session restore";
  EXPECT_FALSE(BrowserInitState::From(browser())->omit_from_session_restore())
      << "The unmanaged window is what session restore should bring back";
}
