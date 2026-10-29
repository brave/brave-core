// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/bookmarks/floating_bookmark_bar_controller.h"

#include <algorithm>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/bookmarks/brave_bookmark_bar_view.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_container_view.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"
#include "chrome/browser/ui/omnibox/omnibox_popup_state_manager.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/transform.h"
#include "ui/views/border.h"
#include "ui/views/event_monitor.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr int kOuterMargin = 5;
constexpr int kCornerRadius = 4;
constexpr int kRevealOffset = 8;
constexpr float kLightThemeShadowOpacity = 0.65f;
constexpr base::TimeDelta kShowDelay = base::Milliseconds(150);
constexpr base::TimeDelta kHideDelay = base::Milliseconds(400);
constexpr base::TimeDelta kShowDuration = base::Milliseconds(160);
constexpr base::TimeDelta kHideDuration = base::Milliseconds(120);
constexpr ViewShadow::ShadowParameters kShadow{
    .offset_x = 0,
    .offset_y = 2,
    .blur_radius = 8,
    .shadow_color = SkColorSetA(SK_ColorBLACK, 38)};

class FloatingBookmarkBarBorder : public views::Border {
 public:
  FloatingBookmarkBarBorder()
      : border_(views::CreateRoundedRectBorder(
            kRoundedCornersContentsOutlineThickness,
            kCornerRadius,
            kColorBraveContentsOutline)) {
    set_color(border_->color());
  }

  void Paint(const views::View& view, gfx::Canvas* canvas) override {
    border_->Paint(view, canvas);
  }

  // The outline must not change native bookmark item alignment or page bounds.
  gfx::Insets GetInsets() const override { return {}; }
  gfx::Size GetMinimumSize() const override { return {}; }

  void OnViewThemeChanged(views::View* view) override {
    border_->OnViewThemeChanged(view);
  }

 private:
  const std::unique_ptr<views::Border> border_;
};

}  // namespace

FloatingBookmarkBarController::FloatingBookmarkBarController(
    BraveBrowserView& browser_view,
    BraveBookmarkBarView& bookmark_bar)
    : browser_view_(browser_view), bookmark_bar_(bookmark_bar) {
  created_layer_ = !bookmark_bar_->layer();
  bookmark_bar_->SetPaintToLayer();
  bookmark_bar_->layer()->SetFillsBoundsOpaquely(false);
  const gfx::RoundedCornersF corner_radii(kCornerRadius);
  bookmark_bar_->layer()->SetRoundedCornerRadius(corner_radii);
  bookmark_bar_->layer()->SetMasksToBounds(true);
  bookmark_bar_->SetBorder(std::make_unique<FloatingBookmarkBarBorder>());
  shadow_ =
      std::make_unique<ViewShadow>(&bookmark_bar_.get(), corner_radii, kShadow);
  shadow_->SetVisible(false);
  bookmark_bar_observation_.Observe(&bookmark_bar_.get());
  bookmark_bar_->SetVisible(false);
  widget_observation_.Observe(browser_view_->GetWidget());
  focus_observation_.Observe(browser_view_->GetFocusManager());
  if (auto* tabs = browser_view_->vertical_tab_strip_container_view()) {
    vertical_tabs_observation_.Observe(tabs);
  }

  // These callbacks are owned by this controller and removed before it dies.
  event_monitor_ = views::EventMonitor::CreateWindowMonitor(
      this, browser_view_->GetWidget()->GetNativeWindow(),
      {ui::EventType::kMouseMoved, ui::EventType::kMouseExited,
       ui::EventType::kMousePressed, ui::EventType::kMouseReleased,
       ui::EventType::kMouseDragged, ui::EventType::kKeyPressed,
       ui::EventType::kKeyReleased});
  last_mouse_location_ = event_monitor_->GetLastMouseLocation();
  reveal_watcher_ = std::make_unique<EdgeRevealWatcher>(
      browser_view_->GetWidget(),
      base::BindRepeating(
          [](BraveBookmarkBarView* bar, const views::View* view) {
            return bar->Contains(view);
          },
          &bookmark_bar_.get()),
      base::BindRepeating(
          &FloatingBookmarkBarController::OnRevealReasonsChanged,
          base::Unretained(this)));
  popup_state_subscription_ =
      browser_view_->GetLocationBarView()
          ->GetOmniboxController()
          ->popup_state_manager()
          ->AddPopupStateChangedCallback(base::BindRepeating(
              [](FloatingBookmarkBarController* controller, OmniboxPopupState,
                 OmniboxPopupState) { controller->UpdateRevealState(); },
              base::Unretained(this)));
  UpdateActiveContents(browser_view_->GetActiveWebContents());
}

FloatingBookmarkBarController::~FloatingBookmarkBarController() {
  popup_state_subscription_ = {};
  Observe(nullptr);
  focus_observation_.Reset();
  widget_observation_.Reset();
  vertical_tabs_observation_.Reset();
  bookmark_bar_observation_.Reset();
  reveal_watcher_.reset();
  event_monitor_.reset();
  show_timer_.Stop();
  hide_timer_.Stop();
  animation_.Reset();
  shadow_.reset();
  bookmark_bar_->layer()->SetOpacity(1.0f);
  bookmark_bar_->layer()->SetTransform(gfx::Transform());
  bookmark_bar_->layer()->SetRoundedCornerRadius(gfx::RoundedCornersF());
  bookmark_bar_->layer()->SetMasksToBounds(false);
  if (created_layer_) {
    bookmark_bar_->DestroyLayer();
  }
  bookmark_bar_->SetBorder(nullptr);
  bookmark_bar_->SetVisible(true);
}

// static
gfx::Rect FloatingBookmarkBarController::CalculateBounds(
    const gfx::Rect& contents_bounds,
    const gfx::Rect& toolbar_bounds,
    std::optional<gfx::Rect> vertical_tabs_bounds,
    int margin,
    int preferred_height) {
  gfx::Rect bounds = contents_bounds;
  bounds.SetVerticalBounds(toolbar_bounds.bottom(), contents_bounds.bottom());
  if (vertical_tabs_bounds && bounds.Intersects(*vertical_tabs_bounds)) {
    if (vertical_tabs_bounds->CenterPoint().x() < bounds.CenterPoint().x()) {
      bounds.SetHorizontalBounds(
          std::max(bounds.x(), vertical_tabs_bounds->right()), bounds.right());
    } else {
      bounds.SetHorizontalBounds(
          bounds.x(), std::min(bounds.right(), vertical_tabs_bounds->x()));
    }
  }
  bounds.Inset(gfx::Insets(margin));
  bounds.set_height(std::min(bounds.height(), preferred_height));
  return bounds;
}

void FloatingBookmarkBarController::UpdateLayout() {
  std::optional<gfx::Rect> vertical_tabs_bounds;
  if (auto* tabs = browser_view_->vertical_tab_strip_container_view();
      tabs && tabs->IsDrawn()) {
    vertical_tabs_bounds = tabs->GetBoundsInScreen();
  }
  bounds_in_screen_ = CalculateBounds(
      browser_view_->multi_contents_view()->GetBoundsInScreen(),
      browser_view_->toolbar()->GetBoundsInScreen(), vertical_tabs_bounds,
      kOuterMargin, bookmark_bar_->GetPreferredSize().height());
  trigger_in_screen_ = browser_view_->toolbar()->GetBoundsInScreen();
  // Cover the gap so moving from the toolbar into the bar cannot dismiss it.
  trigger_in_screen_.set_height(trigger_in_screen_.height() + kOuterMargin);

  gfx::Rect local_bounds = views::View::ConvertRectFromScreen(
      &browser_view_.get(), bounds_in_screen_);
  local_bounds.set_x(browser_view_->GetMirroredXForRect(local_bounds));
  bookmark_bar_->SetBoundsRect(local_bounds);
  if (!CanReveal()) {
    show_timer_.Stop();
    hide_timer_.Stop();
    hovering_ = false;
    dragging_ = false;
    animation_.Reset();
  }
  UpdateAnimation();
  UpdateRevealState();
}

void FloatingBookmarkBarController::UpdateActiveContents(
    content::WebContents* contents) {
  if (web_contents() != contents) {
    Observe(contents);
    new_tab_dismissed_ = false;
    hover_dismissed_ = false;
  }
  UpdateLayout();
}

void FloatingBookmarkBarController::DidFinishNavigation(
    content::NavigationHandle* navigation) {
  if (!navigation->HasCommitted() || !navigation->IsInPrimaryMainFrame()) {
    return;
  }
  if (!IsNewTabPage()) {
    new_tab_dismissed_ = false;
  }
  UpdateLayout();
}

bool FloatingBookmarkBarController::IsNewTabPage() const {
  return web_contents() && NewTabUI::IsNewTab(web_contents()->GetVisibleURL());
}

bool FloatingBookmarkBarController::CanReveal(bool for_keyboard) const {
  if (!widget_observation_.IsObserving()) {
    return false;
  }
  const auto* focused = browser_view_->GetFocusManager()->GetFocusedView();
  const auto* location_bar = browser_view_->GetLocationBarView();
  const auto* omnibox = location_bar->GetOmniboxController();
  // Widget visibility callbacks can precede the popup state's update. Avoid
  // running IsPopupOpen()'s widget-consistency validation during that
  // transition.
  const bool popup_open =
      omnibox->popup_state_manager()->popup_state() != OmniboxPopupState::kNone;
  const bool editing_address =
      browser_view_->GetWidget()->IsActive() &&
      location_bar->Contains(focused) &&
      (omnibox->edit_model()->user_input_in_progress() || popup_open);
  return !modal_visible_ && (for_keyboard || !editing_address) &&
         !bounds_in_screen_.IsEmpty() && !browser_view_->IsFullscreen() &&
         browser_view_->top_container()->parent() == &browser_view_.get() &&
         browser_view_->toolbar()->IsDrawn();
}

bool FloatingBookmarkBarController::IsRevealed() const {
  return animation_.GetCurrentValue() > 0.0;
}

bool FloatingBookmarkBarController::IsHoverPoint(
    const gfx::Point& screen_point) const {
  if (trigger_in_screen_.Contains(screen_point)) {
    return true;
  }
  if (!IsRevealed()) {
    return false;
  }
  // Include the margin between the toolbar and bar so crossing it cannot hide.
  gfx::Rect hover_bounds = bounds_in_screen_;
  hover_bounds.SetVerticalBounds(trigger_in_screen_.y(), hover_bounds.bottom());
  return hover_bounds.Contains(screen_point);
}

void FloatingBookmarkBarController::OnEvent(const ui::Event& event) {
  if (event.type() == ui::EventType::kKeyPressed &&
      event.AsKeyEvent()->key_code() == ui::VKEY_ESCAPE) {
    if (!reveal_reasons_.Has(
            EdgeRevealWatcher::RevealReason::kVisibleChildWidget)) {
      if (IsRevealed() || animation_.IsShowing()) {
        new_tab_dismissed_ = true;
        hover_dismissed_ = true;
      }
      if (bookmark_bar_->Contains(
              browser_view_->GetFocusManager()->GetFocusedView())) {
        if (auto* contents = browser_view_->GetActiveWebContents()) {
          contents->Focus();
        }
      }
      hovering_ = false;
      SetRevealed(false);
    }
    return;
  }
  if (event.type() == ui::EventType::kMouseDragged) {
    dragging_ = true;
  } else if (event.type() == ui::EventType::kMouseReleased ||
             event.type() == ui::EventType::kMouseMoved) {
    dragging_ = false;
  }
  const gfx::Point mouse_location = event_monitor_->GetLastMouseLocation();
  // Tab and focus changes synthesize moves without real pointer movement.
  const bool pointer_moved = mouse_location != last_mouse_location_;
  last_mouse_location_ = mouse_location;
  hovering_ = event.type() != ui::EventType::kMouseExited &&
              IsHoverPoint(mouse_location);
  if (!hovering_) {
    hover_dismissed_ = false;
  }
  // The New Tab reveal ends once the pointer moves into the page, so the bar
  // cannot keep covering page controls. Toolbar hover still reveals it.
  if (!hovering_ && pointer_moved &&
      event.type() == ui::EventType::kMouseMoved && IsNewTabPage() &&
      browser_view_->multi_contents_view()->GetBoundsInScreen().Contains(
          mouse_location)) {
    new_tab_dismissed_ = true;
  }
  UpdateRevealState();
}

void FloatingBookmarkBarController::OnViewBoundsChanged(views::View* view) {
  UpdateLayout();
}

void FloatingBookmarkBarController::OnDidChangeFocus(views::View* before,
                                                     views::View* now) {
  UpdateRevealState();
}

void FloatingBookmarkBarController::OnViewIsDeleting(views::View* view) {
  vertical_tabs_observation_.Reset();
}

void FloatingBookmarkBarController::OnWidgetActivationChanged(
    views::Widget* widget,
    bool /*active*/) {
  hovering_ = IsHoverPoint(event_monitor_->GetLastMouseLocation());
  UpdateLayout();
  UpdateRevealState();
}

void FloatingBookmarkBarController::OnWidgetWindowModalVisibilityChanged(
    views::Widget* widget,
    bool visible) {
  modal_visible_ = visible;
  UpdateLayout();
  if (!visible) {
    UpdateRevealState();
  }
}

void FloatingBookmarkBarController::OnWidgetDestroying(views::Widget* widget) {
  popup_state_subscription_ = {};
  event_monitor_.reset();
  show_timer_.Stop();
  hide_timer_.Stop();
  focus_observation_.Reset();
  widget_observation_.Reset();
}

void FloatingBookmarkBarController::OnRevealReasonsChanged(
    EdgeRevealWatcher::RevealReasons reasons) {
  reveal_reasons_ = reasons;
  UpdateRevealState();
}

void FloatingBookmarkBarController::UpdateRevealState() {
  const bool child_visible =
      reveal_reasons_.Has(EdgeRevealWatcher::RevealReason::kVisibleChildWidget);
  const bool focus_or_bubble =
      (browser_view_->GetWidget()->IsActive() &&
       reveal_reasons_.Has(EdgeRevealWatcher::RevealReason::kFocus)) ||
      reveal_reasons_.Has(EdgeRevealWatcher::RevealReason::kAnchoredBubble);
  const auto* widget = browser_view_->GetWidget();
  const bool drag_active = dragging_ || (widget && widget->dragged_view());
  if (!CanReveal() || (child_visible && !IsRevealed())) {
    SetRevealed(false);
    return;
  }
  if ((IsNewTabPage() && !new_tab_dismissed_) || focus_or_bubble ||
      (IsRevealed() && child_visible)) {
    SetRevealed(true);
  } else if (hovering_ && !hover_dismissed_ && !drag_active) {
    hide_timer_.Stop();
    if (IsRevealed()) {
      SetRevealed(true);
    } else if (!show_timer_.IsRunning()) {
      show_timer_.Start(FROM_HERE, kShowDelay, this,
                        &FloatingBookmarkBarController::RevealAfterHover);
    }
  } else {
    show_timer_.Stop();
    if ((IsRevealed() || animation_.IsShowing()) && !hide_timer_.IsRunning()) {
      hide_timer_.Start(FROM_HERE, kHideDelay, this,
                        &FloatingBookmarkBarController::Hide);
    }
  }
}

void FloatingBookmarkBarController::RevealAfterHover() {
  hovering_ = IsHoverPoint(event_monitor_->GetLastMouseLocation());
  if (hovering_ && !hover_dismissed_ && CanReveal() && !dragging_ &&
      !browser_view_->GetWidget()->dragged_view() &&
      !reveal_reasons_.Has(
          EdgeRevealWatcher::RevealReason::kVisibleChildWidget)) {
    SetRevealed(true);
  }
}

void FloatingBookmarkBarController::Hide() {
  dragging_ = browser_view_->GetWidget()->dragged_view() != nullptr;
  if (dragging_ && IsRevealed()) {
    UpdateRevealState();
    return;
  }
  if (CanReveal() && IsHoverPoint(event_monitor_->GetLastMouseLocation())) {
    hovering_ = true;
    SetRevealed(true);
    return;
  }
  SetRevealed(false);
}

void FloatingBookmarkBarController::RevealForKeyboard() {
  if (CanReveal(/*for_keyboard=*/true)) {
    new_tab_dismissed_ = false;
    hover_dismissed_ = false;
    // Release omnibox focus before revealing so layout cannot suppress the bar
    // while BrowserView transfers keyboard focus into it.
    if (browser_view_->GetLocationBarView()->Contains(
            browser_view_->GetFocusManager()->GetFocusedView())) {
      browser_view_->GetFocusManager()->ClearFocus();
    }
    SetRevealed(true);
  }
}

void FloatingBookmarkBarController::SetRevealed(bool revealed) {
  show_timer_.Stop();
  hide_timer_.Stop();
  animation_.SetSlideDuration(revealed ? kShowDuration : kHideDuration);
  animation_.SetTweenType(revealed ? gfx::Tween::LINEAR_OUT_SLOW_IN
                                   : gfx::Tween::FAST_OUT_LINEAR_IN);
  if (!gfx::Animation::ShouldRenderRichAnimation()) {
    animation_.Reset(revealed ? 1.0 : 0.0);
  } else if (revealed) {
    animation_.Show();
  } else {
    animation_.Hide();
  }
  UpdateAnimation();
}

void FloatingBookmarkBarController::UpdateAnimation() {
  const double fraction = animation_.GetCurrentValue();
  bookmark_bar_->layer()->SetOpacity(fraction);
  gfx::Transform transform;
  transform.Translate(0, -kRevealOffset * (1.0 - fraction));
  bookmark_bar_->layer()->SetTransform(transform);
  const float shadow_opacity =
      color_utils::IsDark(bookmark_bar_->GetColorProvider()->GetColor(
          kColorBookmarkBarBackground))
          ? 1.0f
          : kLightThemeShadowOpacity;
  shadow_->SetOpacity(fraction * shadow_opacity);
  shadow_->SetTransform(transform);
  const bool visible = fraction > 0.0 || animation_.IsShowing();
  shadow_->SetVisible(visible);
  bookmark_bar_->SetVisible(visible);
}

void FloatingBookmarkBarController::OnViewThemeChanged(views::View* view) {
  UpdateAnimation();
}

void FloatingBookmarkBarController::AnimationProgressed(
    const gfx::Animation* animation) {
  UpdateAnimation();
}

void FloatingBookmarkBarController::AnimationEnded(
    const gfx::Animation* animation) {
  UpdateAnimation();
}
