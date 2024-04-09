/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/immersive_mode_controller_aura.h"

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/find_bar/find_bar.h"
#include "chrome/browser/ui/find_bar/find_bar_controller.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/border.h"
#include "ui/views/focus/focus_search.h"
#include "ui/views/widget/native_widget.h"

namespace {

class ImmersiveModeFocusSearchAura : public views::FocusSearch {
 public:
  explicit ImmersiveModeFocusSearchAura(BrowserView* browser_view);
  ImmersiveModeFocusSearchAura(const ImmersiveModeFocusSearchAura&) = delete;
  ImmersiveModeFocusSearchAura& operator=(const ImmersiveModeFocusSearchAura&) =
      delete;
  ~ImmersiveModeFocusSearchAura() override;

  // views::FocusSearch:
  views::View* FindNextFocusableView(
      views::View* starting_view,
      SearchDirection search_direction,
      TraversalDirection traversal_direction,
      StartingViewPolicy check_starting_view,
      AnchoredDialogPolicy can_go_into_anchored_dialog,
      views::FocusTraversable** focus_traversable,
      views::View** focus_traversable_view) override;

 private:
  raw_ptr<BrowserView> browser_view_;
};

class RevealedLock : public ImmersiveRevealedLock {
 public:
  explicit RevealedLock(SimpleImmersiveRevealedLock* lock) : lock_(lock) {}

  RevealedLock(const RevealedLock&) = delete;
  RevealedLock& operator=(const RevealedLock&) = delete;

 private:
  std::unique_ptr<SimpleImmersiveRevealedLock> lock_;
};

// Converts from ImmersiveModeControllerAura::AnimateReveal to
// ImmersiveFullscreenControllerAura::AnimateReveal.
ImmersiveFullscreenControllerAura::AnimateReveal
ToImmersiveFullscreenControllerAnimateReveal(
    ImmersiveModeControllerAura::AnimateReveal animate_reveal) {
  switch (animate_reveal) {
    case ImmersiveModeController::ANIMATE_REVEAL_YES:
      return ImmersiveFullscreenControllerAura::ANIMATE_REVEAL_YES;
    case ImmersiveModeController::ANIMATE_REVEAL_NO:
      return ImmersiveFullscreenControllerAura::ANIMATE_REVEAL_NO;
  }
  NOTREACHED_NORETURN();
}

}  // namespace

ImmersiveModeControllerAura::ImmersiveModeControllerAura() = default;

ImmersiveModeControllerAura::~ImmersiveModeControllerAura() {
  CHECK(!views::WidgetObserver::IsInObserverList());
}

void ImmersiveModeControllerAura::Init(BrowserView* browser_view) {
  browser_view_ = browser_view;
  focus_search_ = std::make_unique<ImmersiveModeFocusSearchAura>(browser_view);
  controller_.Init(this, browser_view_->frame(),
                   browser_view_->top_container());
}

void ImmersiveModeControllerAura::SetEnabled(bool enabled) {
  // TODO(simonhong): Toolbar should be visible in vertial tab mode.
  if (tabs::utils::ShouldShowVerticalTabs(browser_view_->browser())) {
    return;
  }
  ImmersiveFullscreenControllerAura::EnableForWidget(browser_view_->frame(),
                                                     enabled);

  if (enabled) {
    top_container_observation_.Observe(browser_view_->top_container());
    browser_frame_observation_.Observe(browser_view_->GetWidget());
    overlay_widget_observation_.Observe(browser_view_->overlay_widget());

    // Move the appropriate children from the browser widget to the overlay
    // widget. Make sure to call `Show()` on the overlay widget before enabling
    // immersive fullscreen. The call to `Show()` actually performs the
    // underlying window reparenting.
    MoveChildren(browser_view_->GetWidget(), browser_view_->overlay_widget());

    // `Show()` is needed because the overlay widget's compositor is still being
    // used, even though its content view has been moved to the AppKit
    // controlled fullscreen NSWindow.
    browser_view_->overlay_widget()->Show();

    // Move top chrome to the overlay view.
    browser_view_->OnImmersiveRevealStarted();

    browser_view_->GetWidget()->GetFocusManager()->AddFocusChangeListener(this);
    // Set up a root FocusTraversable that handles focus cycles between overlay
    // widgets and the browser widget.
    browser_view_->GetWidget()->SetFocusTraversableParent(this);
    browser_view_->GetWidget()->SetFocusTraversableParentView(browser_view_);
    browser_view_->overlay_widget()->SetFocusTraversableParent(this);
    browser_view_->overlay_widget()->SetFocusTraversableParentView(
        browser_view_->overlay_view());
    if (browser_view_->tab_overlay_widget()) {
      browser_view_->tab_overlay_widget()->SetFocusTraversableParent(this);
      browser_view_->tab_overlay_widget()->SetFocusTraversableParentView(
          browser_view_->tab_overlay_view());
    }

    // If the window is maximized OnViewBoundsChanged will not be called
    // when transitioning to full screen. Call it now.
    OnViewBoundsChanged(browser_view_->top_container());
  } else {
    top_container_observation_.Reset();
    browser_frame_observation_.Reset();
    overlay_widget_observation_.Reset();

    // Notify BrowserView about the fullscreen exit so that the top container
    // can be reparented, otherwise it might be destroyed along with the
    // overlay widget.
    for (Observer& observer : observers_) {
      observer.OnImmersiveFullscreenExited();
    }

    // Rollback the view shuffling from enablement.
    MoveChildren(browser_view_->overlay_widget(), browser_view_->GetWidget());
    browser_view_->overlay_widget()->Hide();

    browser_view_->GetWidget()->GetFocusManager()->RemoveFocusChangeListener(
        this);
    focus_lock_.reset();

    // Remove the root FocusTraversable.
    browser_view_->GetWidget()->SetFocusTraversableParent(nullptr);
    browser_view_->GetWidget()->SetFocusTraversableParentView(nullptr);
    browser_view_->overlay_widget()->SetFocusTraversableParent(nullptr);
    browser_view_->overlay_widget()->SetFocusTraversableParentView(nullptr);
    if (browser_view_->tab_overlay_widget()) {
      browser_view_->tab_overlay_widget()->SetFocusTraversableParent(nullptr);
      browser_view_->tab_overlay_widget()->SetFocusTraversableParentView(
          nullptr);
    }
  }
}

bool ImmersiveModeControllerAura::IsEnabled() const {
  return controller_.IsEnabled();
}

bool ImmersiveModeControllerAura::ShouldHideTopViews() const {
  return controller_.IsEnabled() && !controller_.IsRevealed();
}

bool ImmersiveModeControllerAura::IsRevealed() const {
  return controller_.IsRevealed();
}

int ImmersiveModeControllerAura::GetTopContainerVerticalOffset(
    const gfx::Size& top_container_size) const {
  if (!IsEnabled()) {
    return 0;
  }

  return static_cast<int>(top_container_size.height() *
                          (visible_fraction_ - 1));
}

std::unique_ptr<ImmersiveRevealedLock>
ImmersiveModeControllerAura::GetRevealedLock(AnimateReveal animate_reveal) {
  return std::make_unique<RevealedLock>(controller_.GetRevealedLock(
      ToImmersiveFullscreenControllerAnimateReveal(animate_reveal)));
}

void ImmersiveModeControllerAura::OnImmersiveRevealStarted() {
  visible_fraction_ = 0;

  for (Observer& observer : observers_) {
    observer.OnImmersiveRevealStarted();
  }
}

void ImmersiveModeControllerAura::OnImmersiveRevealEnded() {
  visible_fraction_ = 0;
  browser_view_->contents_web_view()->holder()->SetHitTestTopInset(0);

  for (Observer& observer : observers_) {
    observer.OnImmersiveRevealEnded();
  }
}

void ImmersiveModeControllerAura::OnImmersiveFullscreenEntered() {}

void ImmersiveModeControllerAura::OnImmersiveFullscreenExited() {
  browser_view_->contents_web_view()->holder()->SetHitTestTopInset(0);
  for (Observer& observer : observers_) {
    observer.OnImmersiveFullscreenExited();
  }
}

void ImmersiveModeControllerAura::SetVisibleFraction(double visible_fraction) {
  if (visible_fraction_ == visible_fraction) {
    return;
  }

  // Sets the top inset only when the top-of-window views is fully visible. This
  // means some gesture may not be recognized well during the animation, but
  // that's fine since a complicated gesture wouldn't be involved during the
  // animation duration. See: https://crbug.com/901544.
  if (browser_view_->GetSupportsTabStrip()) {
    if (visible_fraction == 1.0) {
      browser_view_->contents_web_view()->holder()->SetHitTestTopInset(
          browser_view_->top_container()->height());
    } else if (visible_fraction_ == 1.0) {
      browser_view_->contents_web_view()->holder()->SetHitTestTopInset(0);
    }
  }
  visible_fraction_ = visible_fraction;
  browser_view_->DeprecatedLayoutImmediately();
}

std::vector<gfx::Rect> ImmersiveModeControllerAura::GetVisibleBoundsInScreen()
    const {
  views::View* top_container_view = browser_view_->top_container();
  gfx::Rect top_container_view_bounds = top_container_view->GetLocalBounds();
  // TODO(tdanderson): Implement View::ConvertRectToScreen().
  gfx::Point top_container_view_bounds_in_screen_origin(
      top_container_view_bounds.origin());
  views::View::ConvertPointToScreen(
      top_container_view, &top_container_view_bounds_in_screen_origin);
  gfx::Rect top_container_view_bounds_in_screen(
      top_container_view_bounds_in_screen_origin,
      top_container_view_bounds.size());

  std::vector<gfx::Rect> bounds_in_screen;
  bounds_in_screen.push_back(top_container_view_bounds_in_screen);
  bounds_in_screen.push_back(find_bar_visible_bounds_in_screen_);
  return bounds_in_screen;
}

void ImmersiveModeControllerAura::OnFindBarVisibleBoundsChanged(
    const gfx::Rect& new_visible_bounds_in_screen) {
  find_bar_visible_bounds_in_screen_ = new_visible_bounds_in_screen;
}

bool ImmersiveModeControllerAura::ShouldStayImmersiveAfterExitingFullscreen() {
  return false;
}

void ImmersiveModeControllerAura::OnWidgetActivationChanged(
    views::Widget* widget,
    bool active) {}

int ImmersiveModeControllerAura::GetMinimumContentOffset() const {
  return 0;
}

int ImmersiveModeControllerAura::GetExtraInfobarOffset() const {
  return 0;
}

void ImmersiveModeControllerAura::OnWillChangeFocus(views::View* focused_before,
                                                    views::View* focused_now) {}

void ImmersiveModeControllerAura::OnDidChangeFocus(views::View* focused_before,
                                                   views::View* focused_now) {
  if (browser_view_->top_container()->Contains(focused_now) ||
      browser_view_->tab_overlay_view()->Contains(focused_now)) {
    if (!focus_lock_) {
      focus_lock_ = GetRevealedLock(ANIMATE_REVEAL_NO);
    }
  } else {
    focus_lock_.reset();
  }
}

void ImmersiveModeControllerAura::OnViewBoundsChanged(
    views::View* observed_view) {
  gfx::Rect bounds = observed_view->bounds();
  if (bounds.IsEmpty()) {
    return;
  }
  browser_view_->overlay_widget()->SetBounds(bounds);
}

void ImmersiveModeControllerAura::OnWidgetDestroying(views::Widget* widget) {
  SetEnabled(false);
}

void ImmersiveModeControllerAura::MoveChildren(views::Widget* from_widget,
                                               views::Widget* to_widget) {
  CHECK(from_widget && to_widget);

  // If the browser window is closing the native view is removed. Don't attempt
  // to move children.
  if (!from_widget->GetNativeView() || !to_widget->GetNativeView()) {
    return;
  }

  views::Widget::Widgets widgets;
  views::Widget::GetAllChildWidgets(from_widget->GetNativeView(), &widgets);
  for (views::Widget* widget : widgets) {
    if (ShouldMoveChild(widget)) {
      views::Widget::ReparentNativeView(widget->GetNativeView(),
                                        to_widget->GetNativeView());
    }
  }
}

bool ImmersiveModeControllerAura::ShouldMoveChild(views::Widget* child) {
  // Filter out widgets that should not be reparented.
  // The browser, overlay and tab overlay widgets all stay put.
  if (child == browser_view_->GetWidget() ||
      child == browser_view_->overlay_widget() ||
      child == browser_view_->tab_overlay_widget()) {
    return false;
  }

  // The find bar should be reparented if it exists.
  if (browser_view_->browser()->HasFindBarController()) {
    FindBarController* find_bar_controller =
        browser_view_->browser()->GetFindBarController();
    if (child == find_bar_controller->find_bar()->GetHostWidget()) {
      return true;
    }
  }

  if (child->GetNativeWindowProperty(views::kWidgetIdentifierKey) ==
      constrained_window::kConstrainedWindowWidgetIdentifier) {
    return true;
  }

  // Widgets that have an anchor view contained within top chrome should be
  // reparented.
  views::WidgetDelegate* widget_delegate = child->widget_delegate();
  if (!widget_delegate) {
    return false;
  }
  views::BubbleDialogDelegate* bubble_dialog =
      widget_delegate->AsBubbleDialogDelegate();
  if (!bubble_dialog) {
    return false;
  }
  // Both `top_container` and `tab_strip_region_view` are checked individually
  // because `tab_strip_region_view` is pulled out of `top_container` to be
  // displayed in the titlebar.
  views::View* anchor_view = bubble_dialog->GetAnchorView();
  if (anchor_view &&
      (browser_view_->top_container()->Contains(anchor_view) ||
       browser_view_->tab_strip_region_view()->Contains(anchor_view))) {
    return true;
  }

  // All other widgets will stay put.
  return false;
}

views::FocusSearch* ImmersiveModeControllerAura::GetFocusSearch() {
  return focus_search_.get();
}

views::FocusTraversable*
ImmersiveModeControllerAura::GetFocusTraversableParent() {
  return nullptr;
}

views::View* ImmersiveModeControllerAura::GetFocusTraversableParentView() {
  return nullptr;
}

ImmersiveModeFocusSearchAura::ImmersiveModeFocusSearchAura(
    BrowserView* browser_view)
    : views::FocusSearch(browser_view, true, true),
      browser_view_(browser_view) {}

ImmersiveModeFocusSearchAura::~ImmersiveModeFocusSearchAura() = default;

views::View* ImmersiveModeFocusSearchAura::FindNextFocusableView(
    views::View* starting_view,
    SearchDirection search_direction,
    TraversalDirection traversal_direction,
    StartingViewPolicy check_starting_view,
    AnchoredDialogPolicy can_go_into_anchored_dialog,
    views::FocusTraversable** focus_traversable,
    views::View** focus_traversable_view) {
  // Search in the `starting_view` traversable tree.
  views::FocusTraversable* starting_focus_traversable =
      starting_view->GetFocusTraversable();
  if (!starting_focus_traversable) {
    starting_focus_traversable =
        starting_view->GetWidget()->GetFocusTraversable();
  }

  views::View* v =
      starting_focus_traversable->GetFocusSearch()->FindNextFocusableView(
          starting_view, search_direction, traversal_direction,
          check_starting_view, can_go_into_anchored_dialog, focus_traversable,
          focus_traversable_view);

  if (v) {
    return v;
  }

  // If no next focusable view in the `starting_view` traversable tree,
  // jumps to the next widget.
  views::FocusManager* focus_manager =
      browser_view_->GetWidget()->GetFocusManager();

  // The focus cycles between overlay widget(s) and the browser widget.
  std::vector<views::Widget*> traverse_order = {browser_view_->overlay_widget(),
                                                browser_view_->GetWidget()};
  if (browser_view_->tab_overlay_widget()) {
    traverse_order.push_back(browser_view_->tab_overlay_widget());
  }

  auto current_widget_it = base::ranges::find_if(
      traverse_order, [starting_view](const views::Widget* widget) {
        return widget->GetRootView()->Contains(starting_view);
      });
  CHECK(current_widget_it != traverse_order.end());
  int current_widget_ind = current_widget_it - traverse_order.begin();

  bool reverse = search_direction == SearchDirection::kBackwards;
  int next_widget_ind =
      (current_widget_ind + (reverse ? -1 : 1) + traverse_order.size()) %
      traverse_order.size();
  return focus_manager->GetNextFocusableView(
      nullptr, traverse_order[next_widget_ind], reverse, true);
}

ImmersiveModeOverlayWidgetObserver::ImmersiveModeOverlayWidgetObserver(
    ImmersiveModeControllerAura* controller)
    : controller_(controller) {}

ImmersiveModeOverlayWidgetObserver::~ImmersiveModeOverlayWidgetObserver() =
    default;

void ImmersiveModeOverlayWidgetObserver::OnWidgetBoundsChanged(
    views::Widget* widget,
    const gfx::Rect& new_bounds) {
  // Update web dialog position when the overlay widget moves by invalidating
  // the browse view layout.
  controller_->browser_view()->InvalidateLayout();
}
