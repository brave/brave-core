/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/frame/layout/brave_browser_view_tabbed_layout_impl.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/grit/brave_components_strings.h"
#include "components/input/native_web_keyboard_event.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/events/event_observer.h"
#include "ui/events/types/event_type.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/border.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/event_monitor.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget.h"

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#endif

namespace {

using ShowSidebarOption = sidebar::SidebarService::ShowSidebarOption;

sidebar::SidebarService* GetSidebarService(BraveBrowser* browser) {
  return sidebar::SidebarServiceFactory::GetForProfile(browser->profile());
}

}  // namespace

class SidebarContainerView::BrowserWindowEventObserver
    : public ui::EventObserver {
 public:
  explicit BrowserWindowEventObserver(SidebarContainerView& host)
      : host_(host) {}
  ~BrowserWindowEventObserver() override = default;
  BrowserWindowEventObserver(const BrowserWindowEventObserver&) = delete;
  BrowserWindowEventObserver& operator=(const BrowserWindowEventObserver&) =
      delete;

  void OnEvent(const ui::Event& event) override {
    DCHECK(event.IsMouseEvent());
    const auto* mouse_event = event.AsMouseEvent();

    gfx::Point window_event_position = mouse_event->location();
    // Convert window position to sidebar view's coordinate and check whether
    // it's included in sidebar ui or not.
    // If it's not included and sidebar could be hidden, stop monitoring and
    // hide UI.
    views::View::ConvertPointFromWidget(host_->sidebar_control_view_,
                                        &window_event_position);
    if (!host_->sidebar_control_view_->GetLocalBounds().Contains(
            window_event_position) &&
        !host_->ShouldForceShowSidebar()) {
      host_->StopBrowserWindowEventMonitoring();
      host_->HideSidebar();
    }
  }

 private:
  const raw_ref<SidebarContainerView> host_;
};

SidebarContainerView::SidebarContainerView(Browser* browser)
    : views::AnimationDelegateViews(this),
      browser_(browser),
      browser_window_event_observer_(
          std::make_unique<BrowserWindowEventObserver>(*this)) {
  constexpr int kAnimationDurationMS = 150;
  width_animation_.SetSlideDuration(base::Milliseconds(kAnimationDurationMS));

  SetNotifyEnterExitOnChild(true);
  SetUseDefaultFillLayout(true);
  SetBackground(views::CreateSolidBackground(kColorToolbar));
}

SidebarContainerView::~SidebarContainerView() = default;

void SidebarContainerView::Init() {
  initialized_ = true;

  AddChildViews();
  SetSidebarShowOption(
      GetSidebarService(GetBraveBrowser())->GetSidebarShowOption());
}

void SidebarContainerView::SetSidebarOnLeft(bool sidebar_on_left) {
  DCHECK(initialized_);

  if (sidebar_on_left_ == sidebar_on_left) {
    return;
  }

  sidebar_on_left_ = sidebar_on_left;

  DCHECK(sidebar_control_view_);
  sidebar_control_view_->SetSidebarOnLeft(sidebar_on_left_);
}

bool SidebarContainerView::IsSidebarVisible() const {
  return sidebar_control_view_ && sidebar_control_view_->GetVisible();
}

void SidebarContainerView::SetSidebarControlViewVisibilityChangedCallback(
    base::RepeatingClosure callback) {
  sidebar_control_view_visibility_changed_callback_ = std::move(callback);
}

void SidebarContainerView::ChildVisibilityChanged(views::View* child) {
  if (child == sidebar_control_view_ &&
      sidebar_control_view_visibility_changed_callback_) {
    sidebar_control_view_visibility_changed_callback_.Run();
  }
}

void SidebarContainerView::ShowSidebarOnMouseOver(
    const gfx::PointF& point_in_screen) {
  if (IsSidebarVisible()) {
    return;
  }

  if (show_sidebar_option_ != ShowSidebarOption::kShowOnMouseOver) {
    return;
  }

  gfx::RectF mouse_event_detect_bounds(
      BraveBrowserView::From(BrowserView::GetBrowserViewForBrowser(browser_))
          ->GetBoundingBoxInScreenForMouseOverHandling());

  constexpr int kHotCornerWidth = 7;
  const int inset = mouse_event_detect_bounds.width() - kHotCornerWidth;
  if (sidebar_on_left_) {
    mouse_event_detect_bounds.Inset(gfx::InsetsF::TLBR(0, 0, 0, inset));
  } else {
    mouse_event_detect_bounds.Inset(gfx::InsetsF::TLBR(0, inset, 0, 0));
  }

  if (mouse_event_detect_bounds.Contains(point_in_screen)) {
    ShowSidebar();
    return;
  }
}

void SidebarContainerView::UpdateBorder() {
  sidebar_control_view_->UpdateBorder();
}

void SidebarContainerView::SetSidebarShowOption(ShowSidebarOption show_option) {
  DVLOG(2) << __func__;

  show_sidebar_option_ = show_option;

  if (show_sidebar_option_ == ShowSidebarOption::kShowAlways) {
    if (!IsSidebarVisible()) {
      ShowSidebar(AnimationStyle::kImmediate);
    }
    return;
  }

  if (show_sidebar_option_ == ShowSidebarOption::kShowNever) {
    HideSidebar();
    return;
  }

  // kShowOnMouseOver
  if (!IsMouseHovered()) {
    HideSidebar();
  }
  return;
}

void SidebarContainerView::UpdateSidebarItemsState() {
  // control view has items.
  sidebar_control_view_->Update();
}

void SidebarContainerView::UpdateSidebarVisibility() {
  // Pinning is an explicit user gesture, not a hover — snap to visible
  // without animation, mirroring kShowAlways above. Otherwise, fall through
  // and follow the current show option.
  if (browser_->GetFeatures().sidebar_controller()->sidebar_pinned()) {
    ShowSidebar(AnimationStyle::kImmediate);
  } else {
    // Refresh sidebar visibility with current show option.
    SetSidebarShowOption(
        GetSidebarService(GetBraveBrowser())->GetSidebarShowOption());
  }
}

void SidebarContainerView::MenuClosed() {
  DVLOG(1) << __func__;

  // Don't need to to auto hide sidebar UI for other options.
  if (show_sidebar_option_ != ShowSidebarOption::kShowOnMouseOver) {
    return;
  }

  // Don't hide sidebar with below conditions.
  if (IsMouseHovered() || ShouldForceShowSidebar()) {
    return;
  }

  HideSidebar();
}

void SidebarContainerView::AddChildViews() {
  sidebar_control_view_ = AddChildView(
      std::make_unique<SidebarControlView>(this, GetBraveBrowser()));
  sidebar_control_view_->SetPaintToLayer();

  // To prevent showing layered-children while its bounds is invisible.
  sidebar_control_view_->layer()->SetMasksToBounds(true);

  // Hide by default. Visibility will be controlled by show options callback
  // later.
  sidebar_control_view_->SetVisible(false);
}

gfx::Size SidebarContainerView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
    if (!initialized_ || !sidebar_control_view_->GetVisible() ||
        IsFullscreenByTab()) {
      return gfx::Size();
    }

    if (!width_animation_.is_animating()) {
      return View::CalculatePreferredSize(available_size);
    }

    return {gfx::Tween::IntValueBetween(
                width_animation_.GetCurrentValue(), 0,
                sidebar_control_view_->GetPreferredSize().width()),
            0};
}

bool SidebarContainerView::IsFullscreenByTab() const {
  DCHECK(browser_->GetFeatures().exclusive_access_manager() &&
         browser_->GetFeatures()
             .exclusive_access_manager()
             ->fullscreen_controller());
  return browser_->GetFeatures()
      .exclusive_access_manager()
      ->fullscreen_controller()
      ->IsWindowFullscreenForTabOrPending();
}

bool SidebarContainerView::ShouldForceShowSidebar() const {
  // Don't hide sidebar when it's pinned.
  return browser_->GetFeatures().sidebar_controller()->sidebar_pinned() ||
         sidebar_control_view_->IsItemReorderingInProgress() ||
         sidebar_control_view_->IsBubbleWidgetVisible();
}

void SidebarContainerView::OnMouseEntered(const ui::MouseEvent& event) {
  if (show_sidebar_option_ != ShowSidebarOption::kShowOnMouseOver) {
    return;
  }

  // Cancel hide schedule when mouse entered again quickly.
  sidebar_hide_timer_.Stop();
}

void SidebarContainerView::OnMouseExited(const ui::MouseEvent& event) {
  if (show_sidebar_option_ != ShowSidebarOption::kShowOnMouseOver) {
    return;
  }

  // When context menu is shown, this view can get this exited callback.
  // In that case, ignore this callback because mouse is still in this view.
  if (IsMouseHovered()) {
    return;
  }

  if (ShouldForceShowSidebar()) {
    StartBrowserWindowEventMonitoring();
    return;
  }

  // Give some delay for hiding to prevent flickering by open/hide quickly.
  // when mouse is moved around the sidebar.
  constexpr int kHideDelayInMS = 400;
  sidebar_hide_timer_.Start(FROM_HERE, base::Milliseconds(kHideDelayInMS),
                            base::BindOnce(&SidebarContainerView::HideSidebar,
                                           base::Unretained(this)));
}

void SidebarContainerView::AnimationProgressed(
    const gfx::Animation* animation) {
  PreferredSizeChanged();
}

void SidebarContainerView::AnimationEnded(const gfx::Animation* animation) {
    if (width_animation_.GetCurrentValue() == 0) {
      sidebar_control_view_->SetVisible(false);
    }
    PreferredSizeChanged();
}

void SidebarContainerView::ShowSidebar(AnimationStyle animation) {
  // Don't need to show again if it's showing now.
  if (width_animation_.is_animating() && width_animation_.IsShowing()) {
    return;
  }

  if (width_animation_.is_animating() && width_animation_.IsClosing()) {
    width_animation_.Stop();
  } else {
    // Otherwise, reset animation to start from the beginning.
    width_animation_.Reset();
  }

  sidebar_control_view_->SetVisible(true);

  if (ShouldUseAnimation() && animation == AnimationStyle::kAnimated) {
    width_animation_.Show();
  } else {
    PreferredSizeChanged();
  }

  // Re-sync item highlight state; ink drops are reset when the control view
  // becomes invisible during hide.
  UpdateSidebarItemsState();
}

void SidebarContainerView::HideSidebar() {
  // Don't need to close again if it's closing now.
  if (width_animation_.is_animating() && width_animation_.IsClosing()) {
    return;
  }

  // Stop showing animation and start closing immediately from there.
  if (width_animation_.is_animating() && width_animation_.IsShowing()) {
    width_animation_.Stop();
  } else {
    // Otherwise, reset animation to hide from the end.
    width_animation_.Reset(1.0);
  }

  sidebar_hide_timer_.Stop();

  if (ShouldUseAnimation()) {
    width_animation_.Hide();
  } else {
    sidebar_control_view_->SetVisible(false);
    PreferredSizeChanged();
  }
  return;
}

bool SidebarContainerView::ShouldUseAnimation() {
  return gfx::Animation::ShouldRenderRichAnimation();
}

void SidebarContainerView::StartBrowserWindowEventMonitoring() {
  if (browser_window_event_monitor_) {
    return;
  }

  DVLOG(1) << __func__;
  browser_window_event_monitor_ = views::EventMonitor::CreateWindowMonitor(
      browser_window_event_observer_.get(), GetWidget()->GetNativeWindow(),
      {ui::EventType::kMouseMoved});
}

void SidebarContainerView::StopBrowserWindowEventMonitoring() {
  DVLOG(1) << __func__;
  browser_window_event_monitor_.reset();
}

BraveBrowser* SidebarContainerView::GetBraveBrowser() const {
  return static_cast<BraveBrowser*>(browser_.get());
}

BEGIN_METADATA(SidebarContainerView)
END_METADATA
