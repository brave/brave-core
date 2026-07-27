/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_BROWSERTEST_BASE_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_BROWSERTEST_BASE_H_

#include <cstddef>
#include <memory>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "ui/gfx/animation/animation_test_api.h"

class BraveBrowserView;
class SidebarContainerView;
class SidebarControlView;
class SidebarItemsContentsView;
class SidebarItemsScrollView;
class SidePanel;
class SidePanelButton;
class TabStripModel;

namespace base {
class RunLoop;
}  // namespace base

namespace gfx {
class Point;
}  // namespace gfx

namespace ui {
class MouseEvent;
}  // namespace ui

namespace views {
class View;
}  // namespace views

namespace sidebar {

class SidebarModel;
class SidebarService;

// Shared base fixture for the sidebar browser tests. It is defined in a header
// so that it can be reused by both the `browser_tests` and the
// `interactive_ui_tests` source sets. The class name and namespace must stay
// `sidebar::SidebarBrowserTest` because the sidebar/frame view classes befriend
// it to grant the helpers below access to their private members.
class SidebarBrowserTest : public InProcessBrowserTest {
 public:
  SidebarBrowserTest();
  ~SidebarBrowserTest() override;

  // InProcessBrowserTest:
  void SetUpOnMainThread() override;
  void PreRunTestOnMainThread() override;

  SidebarModel* model() const { return controller()->model(); }
  TabStripModel* tab_model() const { return browser()->tab_strip_model(); }

  SidebarController* controller() const {
    return browser()->GetFeatures().sidebar_controller();
  }

  SidePanelButton* GetSidePanelToolbarButton() const;
  views::View* GetVerticalTabsContainer() const;
  raw_ptr<SidebarItemsContentsView> GetSidebarItemsContentsView(
      SidebarController* controller) const;
  SidebarItemsScrollView* GetSidebarItemsScrollView(
      SidebarController* controller) const;

  // If the item at |index| is panel item, this will return after waiting
  // model's active index is changed as active index could not be not updated
  // synchronously. Panel activation is done via SidePanelCoordinator instead of
  // asking activation to SidebarController directly.
  void SimulateSidebarItemClickAt(size_t index);

  void HandleBrowserWindowMouseEvent();

  SidebarControlView* GetSidebarControlView() const;
  SidebarContainerView* GetSidebarContainerView() const;

  SidePanel* GetSidePanel();

  bool IsSidebarUIOnLeft() const;

  void ShowSidebar();
  void HideSidebar();

  void WaitUntil(base::RepeatingCallback<bool()> condition);
  void Run();

  void SetItemAddedBubbleLaunchedCallback(
      SidebarItemsContentsView* items_contents_view);
  void ItemAddedBubbleLaunchedCallback(views::View* anchor);

  void AddItemsTillScrollable(SidebarItemsScrollView* scroll_view,
                              SidebarService* sidebar_service);

  bool NeedScrollForItemAt(size_t index, SidebarItemsScrollView* scroll_view);

  void VerifyTargetDragIndicatorIndexCalc(const gfx::Point& screen_position);

  base::RunLoop* run_loop() const { return run_loop_.get(); }

  size_t GetDefaultItemCount() const;

  int GetFirstPanelItemIndex();
  int GetFirstWebItemIndex();

  BraveBrowserView* browser_view();

 protected:
  static ui::MouseEvent GetDummyEvent();

  raw_ptr<views::View, DanglingUntriaged> item_added_bubble_anchor_ = nullptr;
  std::unique_ptr<base::RunLoop> run_loop_;
  gfx::AnimationTestApi::RenderModeResetter animation_resetter_;
  base::WeakPtrFactory<SidebarBrowserTest> weak_factory_{this};
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_BROWSERTEST_BASE_H_
