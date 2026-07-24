/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"

#include <algorithm>
#include <memory>

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_collection_observer.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/interaction/browser_elements_views.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller_interactive_test_mixin.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_collection.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "ui/base/test/ui_controls.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"

#if BUILDFLAG(IS_LINUX)
#include "ui/base/ozone_buildflags.h"
#include "ui/linux/linux_ui.h"
#include "ui/ozone/public/ozone_platform.h"
#endif  // BUILDFLAG(IS_LINUX)

#if defined(USE_AURA)
#include "ui/aura/env.h"
#endif  // defined(USE_AURA)

namespace {

// The following are copied from
// chrome/browser/ui/views/tabs/dragging/tab_drag_controller_interactive_uitest.cc
// rather than reused via a chromium_src `#include <...cc>` textual include
// because
//  * We're not running upstream's interactive_ui_tests target.
//  * Some of the upstream drag controller tests are flaky with ours.
bool IsDragSessionActive(TabStrip* tab_strip) {
  return tab_strip->GetDragContext()->GetDragController() != nullptr;
}

// If this returns false, we must use ResizeUsingMouseEmulation() instead of
// BrowserWindow::SetBounds().
bool PlatformSupportsScreenCoordinates() {
#if BUILDFLAG(IS_LINUX)
  return ui::OzonePlatform::GetInstance()
      ->GetPlatformProperties()
      .supports_global_screen_coordinates;
#else
  return true;
#endif  // BUILDFLAG(IS_LINUX)
}

TabStrip* GetTabStripForBrowser(BrowserWindowInterface* browser) {
  return BrowserView::GetBrowserViewForBrowser(browser)
      ->horizontal_tab_strip_for_testing();
}

// Resizes the given browser to the specified bounds by using ui_test_utils to
// generate mouse movements, similar to how a regular user would resize the
// window. This is used instead of BrowserWindow::SetBounds() on platforms
// where clients don't have complete control over window bounds (i.e.,
// Wayland).
void ResizeUsingMouseEmulation(Browser* browser,
                               const gfx::Rect& target_bounds) {
#if BUILDFLAG(IS_LINUX)
  auto* window = browser->GetWindow()->GetNativeWindow();
  auto width_difference = window->bounds().width() - target_bounds.width();
  auto height_difference = window->bounds().height() - target_bounds.height();

  // Resize the window, if needed.
  if (width_difference != 0 || height_difference != 0) {
    // We use the container bounds because they don't include window
    // decorations.
    auto bottom_right = browser->tab_strip_model()
                            ->GetActiveWebContents()
                            ->GetContainerBounds()
                            .bottom_right();
    auto resize_target = gfx::Point(bottom_right.x() - width_difference,
                                    bottom_right.y() - height_difference);
    ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(bottom_right, window));
    ASSERT_TRUE(ui_test_utils::SendMouseEventsSync(
        ui_controls::MouseButton::LEFT, ui_controls::MouseButtonState::DOWN,
        window));
    ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(resize_target, window));
    ASSERT_TRUE(ui_test_utils::SendMouseEventsSync(
        ui_controls::MouseButton::LEFT, ui_controls::MouseButtonState::UP,
        window));
  }

  // Move the window.
  auto* grab_handle_space =
      BrowserElementsViews::From(browser)->GetViewAs<views::View>(
          kTabStripFrameGrabHandleElementId);
  auto grab_coordinates =
      ui_test_utils::GetCenterInScreenCoordinates(grab_handle_space);
  gfx::Vector2d grab_offset = {grab_coordinates.x(), grab_coordinates.y()};
  auto move_target = target_bounds.origin() + grab_offset;
  ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(grab_coordinates, window));
  ASSERT_TRUE(ui_test_utils::SendMouseEventsSync(
      ui_controls::MouseButton::LEFT, ui_controls::MouseButtonState::DOWN,
      window));

  // Move the mouse past the drag threshold to allow the real move to proceed.
  int threshold = ui::LinuxUi::kDefaultWindowDragThreshold + 1;
  if (auto* linux_ui = ui::LinuxUi::instance()) {
    threshold = linux_ui->GetWindowDragThresholdPx() + 1;
  }
  auto drag_threshold_move_target =
      grab_coordinates + gfx::Vector2d(threshold, threshold);
  ui_controls::ForceUseScreenCoordinatesOnce();
  ASSERT_TRUE(
      ui_test_utils::SendMouseMoveSync(drag_threshold_move_target, window));

  // `move_target` is in screen coordinates.
  ui_controls::ForceUseScreenCoordinatesOnce();
  ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(move_target, window));

  ASSERT_TRUE(ui_test_utils::SendMouseEventsSync(
      ui_controls::MouseButton::LEFT, ui_controls::MouseButtonState::UP,
      window));
#endif  // BUILDFLAG(IS_LINUX)
}

// The vertical offset needed to cross TabDragController's detach threshold
// and trigger a real detach into a temporary new browser.
int GetDetachY(TabStrip* tab_strip) {
  return std::max(TabDragController::kTouchVerticalDetachMagnetism,
                  TabDragController::kVerticalDetachMagnetism) +
         tab_strip->height() + 1;
}

// Waits for a browser to be added or removed, e.g. when a new browser is
// created to hold dragged tabs or when such a browser is closed after
// attaching to another browser.
class BrowserChangeWaiter : public BrowserCollectionObserver {
 public:
  enum class ChangeType {
    kAdded,
    kRemoved,
  };

  explicit BrowserChangeWaiter(ChangeType type) : type_(type) {
    browser_collection_observation_.Observe(
        GlobalBrowserCollection::GetInstance());
  }
  BrowserChangeWaiter(const BrowserChangeWaiter&) = delete;
  BrowserChangeWaiter& operator=(const BrowserChangeWaiter&) = delete;
  ~BrowserChangeWaiter() override = default;

  // The closure must ensure the drag session/move loop ends.
  void Wait(base::OnceClosure closure) {
    closure_ = std::move(closure);
    run_loop_.Run();
  }

  // BrowserCollectionObserver:
  void OnBrowserCreated(BrowserWindowInterface* browser) override {
    if (type_ == ChangeType::kAdded) {
      Quit();
    }
  }

  void OnBrowserClosed(BrowserWindowInterface* browser) override {
    if (type_ == ChangeType::kRemoved) {
      Quit();
    }
  }

 private:
  void Quit() {
    if (quit_called_) {
      return;
    }
    // Browser addition/removal callbacks can be called multiple times, so
    // make sure that `closure_` still gets to run just once.
    quit_called_ = true;
    if (closure_) {
      // For ChangeType::kRemoved, the browser is still closing and
      // synchronously running the closure now can lead to reentrancy issues,
      // so we instead PostTask() it, and only quit the RunLoop after the
      // closure has run.
      base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE,
          base::BindOnce(
              [](base::OnceClosure closure, base::OnceClosure quit_closure) {
                base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
                    FROM_HERE,
                    std::move(closure).Then(std::move(quit_closure)));
              },
              std::move(closure_), run_loop_.QuitClosure()));
    } else {
      run_loop_.Quit();
    }
  }

  ChangeType type_;
  bool quit_called_ = false;
  base::OnceClosure closure_;
  base::RunLoop run_loop_{base::RunLoop::Type::kNestableTasksAllowed};
  base::ScopedObservation<GlobalBrowserCollection, BrowserCollectionObserver>
      browser_collection_observation_{this};
};

}  // namespace

// Drags a tree-tab parent (with its child) from `browser()` into a second,
// already-open window, driven through real synthetic OS input events (not
// through TabDragController's private API - see the discussion in PR 38439
// for why a plain browser_test can't safely reach AttachToNewContext()'s
// tree-node branch any other way that doesn't risk entering the native
// move-loop machinery unexpectedly).
class TreeTabDragControllerTest
    : public TabDragControllerInteractiveTestMixin<InProcessBrowserTest> {
 public:
  TreeTabDragControllerTest() {
    tree_tab_feature_list_.InitAndEnableFeature(tabs::kBraveTreeTab);
  }

  void SetUp() override {
#if defined(USE_AURA)
    // This needs to be disabled as it can interfere with when events are
    // processed. In particular if input throttling is turned on, then when
    // an event ack runs the event may not have been processed.
    aura::Env::set_initial_throttle_input_on_resize_for_testing(false);
#endif  // defined(USE_AURA)
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    browser()->GetProfile()->GetPrefs()->SetBoolean(
        brave_tabs::kVerticalTabsEnabled, true);
    browser()->GetProfile()->GetPrefs()->SetBoolean(
        brave_tabs::kTreeTabsEnabled, true);
  }

  // Creates a new Browser and resizes browser() and the new browser to be
  // side by side.
  Browser* CreateAnotherBrowserAndResize() {
    // Resize the two windows so they're right next to each other.
    //
    // If we're using ResizeUsingMouseEmulation(), it's important we resize
    // the first browser before creating the second one, else the second
    // browser might occlude the first browser's bottom right corner or tab
    // strip grab handle space, preventing us from resizing or moving it.
    const gfx::NativeWindow window = browser()->GetWindow()->GetNativeWindow();
    gfx::Rect work_area =
        display::Screen::Get()->GetDisplayNearestWindow(window).work_area();
    const gfx::Size size(work_area.width() / 3, work_area.height() / 2);
    gfx::Rect browser_rect(work_area.origin() + gfx::Vector2d(50, 50), size);

    if (PlatformSupportsScreenCoordinates()) {
      ui_test_utils::SetAndWaitForBounds(*browser(), browser_rect);
      browser_rect.set_x(browser_rect.right());
    } else {
      ResizeUsingMouseEmulation(browser(), browser_rect);

      // `container_bounds` doesn't include window decorations, so we can use
      // it to calculate the window decorations' width by comparing its
      // offset from the browser window's bounds.
      auto container_bounds = browser()
                                  ->tab_strip_model()
                                  ->GetActiveWebContents()
                                  ->GetContainerBounds();
      auto window_decoration_width =
          container_bounds.x() - browser()->GetWindow()->GetBounds().x();
      // We need to correct for the decorations drawn to the right of the
      // first window and to the left of the second window.
      browser_rect.set_x(browser_rect.right() - 2 * window_decoration_width);
    }

    Browser* browser2 = CreateBrowser(browser()->GetProfile());
    if (PlatformSupportsScreenCoordinates()) {
      ui_test_utils::SetAndWaitForBounds(*browser2, browser_rect);
    } else {
      ResizeUsingMouseEmulation(browser2, browser_rect);
    }
    return browser2;
  }

  // Like DragInputToCenter(), but posts `task` once the move completes
  // instead of blocking.
  bool DragInputToCenterNotifyWhenDone(const views::View* view,
                                       base::OnceClosure task,
                                       gfx::Vector2d offset = {}) {
    gfx::Point location =
        ui_test_utils::GetCenterInScreenCoordinates(view) + offset;
    return ui_controls::SendMouseMoveNotifyWhenDone(
        location.x(), location.y(), std::move(task), GetWindowHint(view));
  }

  bool DragInputToAsync(const gfx::Point& location,
                        gfx::NativeWindow window_hint) {
    return ui_controls::SendMouseMove(location.x(), location.y(), window_hint);
  }

 private:
  base::test::ScopedFeatureList tree_tab_feature_list_;
};

namespace {

// Waits for the tab that was pressed on `browser()` to detach into a
// temporary new browser, then drags that temporary browser onto
// `target_browser`'s tab strip. This stops the nested move loop, since the
// second window merges into `target_browser`.
void DetachAndThenAttachToTargetWindow(
    TreeTabDragControllerTest* test,
    BrowserWindowInterface* not_attached_browser,
    BrowserWindowInterface* target_browser) {
  TabStrip* const not_attached_tab_strip =
      GetTabStripForBrowser(not_attached_browser);
  CHECK(not_attached_tab_strip);
  TabStrip* const target_tab_strip = GetTabStripForBrowser(target_browser);
  CHECK(target_tab_strip);

  EXPECT_FALSE(IsDragSessionActive(not_attached_tab_strip));
  EXPECT_FALSE(IsDragSessionActive(target_tab_strip));
  EXPECT_TRUE(TabDragController::IsActive());

  BrowserWindowInterface* new_browser =
      ui_test_utils::GetBrowserNotInSet({not_attached_browser, target_browser});
  if (!new_browser) {
    new_browser = ui_test_utils::WaitForBrowserToOpen();
  }
  TabStrip* const new_tab_strip = GetTabStripForBrowser(new_browser);
  EXPECT_TRUE(IsDragSessionActive(new_tab_strip));

  // Drag to target_tab_strip. This should stop the nested loop from dragging
  // the window.
  if (PlatformSupportsScreenCoordinates()) {
    const gfx::Rect old_window_bounds =
        not_attached_tab_strip->GetWidget()->GetWindowBoundsInScreen();
    const gfx::Rect target_bounds = target_tab_strip->GetBoundsInScreen();
    gfx::Point target_point = target_bounds.CenterPoint();
    target_point.set_x(
        std::max(target_point.x(), old_window_bounds.right() + 1));
    EXPECT_TRUE(target_bounds.Contains(target_point));
    EXPECT_TRUE(test->DragInputToAsync(target_point,
                                       test->GetWindowHint(target_tab_strip)));
  } else {
    EXPECT_TRUE(test->DragInputToCenter(target_tab_strip));
  }
}

}  // namespace

#if BUILDFLAG(IS_MAC)
// Flaky on Mac CI, okay on local build.
#define MAYBE_DragParentWithChild_ToSeparateWindow_KeepsChildNested \
  DISABLED_DragParentWithChild_ToSeparateWindow_KeepsChildNested
#else
#define MAYBE_DragParentWithChild_ToSeparateWindow_KeepsChildNested \
  DragParentWithChild_ToSeparateWindow_KeepsChildNested
#endif

IN_PROC_BROWSER_TEST_F(
    TreeTabDragControllerTest,
    MAYBE_DragParentWithChild_ToSeparateWindow_KeepsChildNested) {
  BraveTabStripModel& source_model =
      *static_cast<BraveTabStripModel*>(browser()->tab_strip_model());

  tabs::TabInterface* const parent_tab = source_model.GetTabAtIndex(0);
  auto child_interface = std::make_unique<tabs::TabModel>(
      content::WebContents::Create(
          content::WebContents::CreateParams(browser()->GetProfile())),
      &source_model);
  child_interface->set_opener(parent_tab);
  source_model.AddTab(std::move(child_interface), -1,
                      ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  tabs::TabInterface* const child_tab = source_model.GetTabAtIndex(1);
  ASSERT_EQ(2, source_model.count());
  ASSERT_EQ(child_tab->GetParentCollection()->GetParentCollection(),
            parent_tab->GetParentCollection());

  // Add and activate an unrelated third tab, so the parent tab is NOT the
  // current selection when we press on it below.
  auto sibling = std::make_unique<tabs::TabModel>(
      content::WebContents::Create(
          content::WebContents::CreateParams(browser()->GetProfile())),
      &source_model);
  source_model.AddTab(std::move(sibling), -1, ui::PAGE_TRANSITION_AUTO_BOOKMARK,
                      ADD_NONE);
  source_model.ActivateTabAt(2);
  ASSERT_FALSE(source_model.IsTabSelected(0));
  ASSERT_FALSE(source_model.IsTabSelected(1));

  const tree_tab::TreeTabNodeId parent_node_id =
      static_cast<const tabs::TreeTabNodeTabCollection*>(
          parent_tab->GetParentCollection())
          ->node()
          .id();
  const tree_tab::TreeTabNodeId child_node_id =
      static_cast<const tabs::TreeTabNodeTabCollection*>(
          child_tab->GetParentCollection())
          ->node()
          .id();

  TabStrip* const tab_strip = GetTabStripForBrowser(browser());
  CHECK(tab_strip);
  tab_strip->StopAnimating();

  Browser* const browser2 = CreateAnotherBrowserAndResize();
  ui_test_utils::WaitForBrowserSetLastActive(browser2);
  TabStrip* const tab_strip2 = GetTabStripForBrowser(browser2);
  CHECK(tab_strip2);

  // Tab::OnMousePressed() calls SelectTab() (selects the whole subtree, per
  // BraveTabStripModel's selection logic) and then MaybeStartDrag().
  Tab* const parent_tab_view = tab_strip->tab_at(0);
  ASSERT_TRUE(PressInputAtCenter(parent_tab_view));
  ASSERT_TRUE(source_model.IsTabSelected(0));
  ASSERT_TRUE(source_model.IsTabSelected(1));
  ASSERT_FALSE(source_model.IsTabSelected(2));

  // Drag to the second window, which triggers
  // TabDragController::AttachToNewContext() - we expect moving the whole
  // tree tab, wrapped in a TreeTabNodeTabCollection, to work correctly.
  ASSERT_TRUE(DragInputToCenterNotifyWhenDone(
      parent_tab_view,
      base::BindOnce(&DetachAndThenAttachToTargetWindow, this, browser(),
                     browser2),
      gfx::Vector2d(0, GetDetachY(tab_strip))));

  // Once attached to the second window, the temporary window goes away, and
  // we release the input.
  BrowserChangeWaiter(BrowserChangeWaiter::ChangeType::kRemoved)
      .Wait(base::BindLambdaForTesting([=, this]() {
        ASSERT_TRUE(IsDragSessionActive(tab_strip2));
        ASSERT_FALSE(IsDragSessionActive(tab_strip));

        tab_strip2->StopAnimating();
        ASSERT_TRUE(DragInputToCenter(tab_strip2->tab_at(0)));
        ASSERT_TRUE(ReleaseInput());
      }));

  ASSERT_FALSE(IsDragSessionActive(tab_strip2));
  ASSERT_FALSE(IsDragSessionActive(tab_strip));
  ASSERT_FALSE(TabDragController::IsActive());

  BraveTabStripModel& dest_model =
      *static_cast<BraveTabStripModel*>(browser2->tab_strip_model());
  ASSERT_NE(dest_model.GetIndexOfTab(parent_tab), TabStripModel::kNoTab);
  ASSERT_NE(dest_model.GetIndexOfTab(child_tab), TabStripModel::kNoTab);

  // The AttachToNewContext() tree-node branch took effect: the child is still
  // nested under the parent's tree node, not flattened to a top-level sibling
  // or misrouted through the group/split insertion path.
  EXPECT_EQ(child_tab->GetParentCollection()->GetParentCollection(),
            parent_tab->GetParentCollection());
  ASSERT_EQ(parent_tab->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);

  EXPECT_TRUE(dest_model.tree_model()->GetNode(parent_node_id));
  EXPECT_TRUE(dest_model.tree_model()->GetNode(child_node_id));
}
