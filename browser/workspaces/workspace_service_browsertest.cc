/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspaces/workspace_service.h"

#include <optional>

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/uuid.h"
#include "brave/browser/workspaces/features.h"
#include "brave/browser/workspaces/workspace_metadata.h"
#include "brave/browser/workspaces/workspace_service_factory.h"
#include "chrome/browser/ui/browser_init_state.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
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

  // Looked up by id on each poll rather than held across the close: if this
  // regresses the unmanaged window is destroyed, and a stale pointer would
  // read as a crash instead of a failure. Waiting on the condition also
  // covers the retire-everything task OnBrowserClosed posts.
  EXPECT_TRUE(base::test::RunUntil([&] {
    BrowserWindowInterface* home =
        BrowserWindowInterface::FromSessionID(unmanaged_id);
    return home && home->GetWindow()->IsVisible();
  })) << "Closing a space should hand the screen back to the window it was "
         "opened from, not close it and quit the browser";
}

// The mirror of the test above. Closing a space's window outright is a
// request to close it, not to go back to the window the space came from --
// and once the tabs are gone the two look identical, so the service is told
// which it was while the tabs are still there.
IN_PROC_BROWSER_TEST_F(WorkspaceServiceBrowserTest,
                       ClosingASpaceWindowDoesNotReturnToTheUnmanagedWindow) {
  ASSERT_TRUE(service());

  // A second window keeps the browser reachable, so closing the space's
  // window does not start the retire-everything sweep and this test can look
  // at what happened to the first window.
  ASSERT_TRUE(CreateBrowser(browser()->GetProfile()));

  WorkspaceMetadata workspace =
      service()->CreateWorkspaceInWindow("Work", browser());
  BrowserWindowInterface* workspace_window = WindowFor(workspace.id);
  ASSERT_TRUE(workspace_window);
  ASSERT_FALSE(IsVisible(browser()));

  // What BraveBrowserView::OnWindowCloseRequested() reports when the person
  // closes the window themselves: the tabs are still present at that point.
  ASSERT_GT(workspace_window->GetTabStripModel()->count(), 0);
  service()->NoteWindowCloseRequested(workspace_window);

  ui_test_utils::BrowserDestroyedObserver destroyed(workspace_window);
  workspace_window->GetTabStripModel()->CloseAllTabs();
  destroyed.Wait();

  EXPECT_FALSE(IsVisible(browser()))
      << "Closing a space's window should not bring back the window it was "
         "opened from; that is what losing its last tab does";
}

// A workspace is written when its window goes off screen, not only when the
// window closes. Without this a space switched away from holds nothing on
// disk until it is closed, and these windows are excluded from Chromium's
// session restore, so a crash in between loses every tab.
IN_PROC_BROWSER_TEST_F(WorkspaceServiceBrowserTest,
                       SwitchingAwayFromASpaceSavesIt) {
  ASSERT_TRUE(service());

  WorkspaceMetadata workspace =
      service()->CreateWorkspaceInWindow("Work", browser());
  BrowserWindowInterface* workspace_window = WindowFor(workspace.id);
  ASSERT_TRUE(workspace_window);

  chrome::AddTabAt(workspace_window, GURL("about:blank"), /*index=*/-1,
                   /*foreground=*/false);
  chrome::AddTabAt(workspace_window, GURL("about:blank"), /*index=*/-1,
                   /*foreground=*/false);
  ASSERT_EQ(workspace_window->GetTabStripModel()->count(), 3);

  // Nothing has been written yet: the space was created empty and has not
  // left the screen.
  std::optional<WorkspaceMetadata> before =
      service()->GetWorkspace(workspace.id);
  ASSERT_TRUE(before);
  ASSERT_EQ(before->number_of_tabs, 0);

  service()->SwitchWindowToUnmanaged(workspace_window);

  std::optional<WorkspaceMetadata> after =
      service()->GetWorkspace(workspace.id);
  ASSERT_TRUE(after);
  EXPECT_EQ(after->number_of_tabs, 3)
      << "Switching away from a space should have written its tabs";
}

// Deleting a space always takes its window with it. A workspace window left
// open after being unbound is saved by nobody -- it is excluded from
// Chromium's session restore, and unbinding drops the service as its saver --
// so the person is handed an ordinary window instead.
IN_PROC_BROWSER_TEST_F(WorkspaceServiceBrowserTest,
                       DeletingTheSpaceYouAreViewingLeavesAnOrdinaryWindow) {
  ASSERT_TRUE(service());

  WorkspaceMetadata workspace =
      service()->CreateWorkspaceInWindow("Work", browser());
  BrowserWindowInterface* workspace_window = WindowFor(workspace.id);
  ASSERT_TRUE(workspace_window);
  const SessionID workspace_window_id = workspace_window->GetSessionID();
  ASSERT_TRUE(IsVisible(workspace_window));

  service()->DeleteWorkspace(workspace.id);

  EXPECT_TRUE(base::test::RunUntil([&] {
    return BrowserWindowInterface::FromSessionID(workspace_window_id) ==
           nullptr;
  })) << "The deleted space's window should not be left open";

  // Whatever the person is left looking at has to be a window something will
  // persist, which a workspace window is not.
  bool has_tracked_window = false;
  ProfileBrowserCollection::GetForProfile(browser()->GetProfile())
      ->ForEach([&](BrowserWindowInterface* browser_window) {
        if (browser_window->GetType() ==
                BrowserWindowInterface::Type::TYPE_NORMAL &&
            browser_window->GetWindow()->IsVisible() &&
            !BrowserInitState::From(browser_window)
                 ->omit_from_session_restore()) {
          has_tracked_window = true;
          return false;
        }
        return true;
      });
  EXPECT_TRUE(has_tracked_window)
      << "Deleting a space should leave a window that session restore saves";
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
