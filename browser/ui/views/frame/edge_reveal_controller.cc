/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/edge_reveal_controller.h"

#include <algorithm>
#include <utility>

#include "ui/compositor/layer.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/event_monitor.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr int kHotZoneThickness = 16;
constexpr base::TimeDelta kRevealAnimationDuration = base::Milliseconds(200);
constexpr base::TimeDelta kMouseEnterDelay = base::Milliseconds(60);
constexpr base::TimeDelta kMouseExitDelay = base::Milliseconds(300);

gfx::Rect GetHotZoneForEdge(EdgeRevealController::Edge edge,
                            const gfx::Rect& window_bounds) {
  using Edge = EdgeRevealController::Edge;
  gfx::Rect rect(window_bounds);
  switch (edge) {
    case Edge::kTop:
      rect.set_height(kHotZoneThickness);
      break;
    case Edge::kLeft:
      rect.set_width(kHotZoneThickness);
      break;
    case Edge::kBottom:
      rect.set_y(rect.height() - kHotZoneThickness);
      break;
    case Edge::kRight:
      rect.set_x(rect.width() - kHotZoneThickness);
      break;
  }
  return rect;
}

}  // namespace

class EdgeRevealController::RevealableState {
 public:
  RevealableState(views::View& view, RevealableOptions options)
      : view_(view), options_(std::move(options)), has_layer_(!!view.layer()) {}

  ~RevealableState() { OnRevealDisabled(); }

  void OnRevealEnabled() {
    if (!view_->layer() && options_.paint_to_layer) {
      view_->SetPaintToLayer();
      view_->layer()->SetFillsBoundsOpaquely(false);
    }
  }

  void OnRevealDisabled() {
    if (!has_layer_ && view_->layer()) {
      view_->DestroyLayer();
    }
  }

 private:
  raw_ref<views::View> view_;
  RevealableOptions options_;
  bool has_layer_ = false;
};

EdgeRevealController::EdgeRevealController(Edge edge, views::Widget& widget)
    : edge_(edge), widget_(widget) {
  animation_.SetSlideDuration(kRevealAnimationDuration);
  animation_.Reset(1.0);
}

EdgeRevealController::~EdgeRevealController() {
  if (auto* focus_manager = widget_->GetFocusManager()) {
    focus_manager->RemoveFocusChangeListener(this);
  }
  for (auto& bubble_widget : observed_bubble_widgets_) {
    bubble_widget->RemoveObserver(this);
  }
}

void EdgeRevealController::AddRevealableView(views::View* view,
                                             RevealableOptions options) {
  auto& state = revealable_views_[view];
  if (state) {
    state.reset();
  }
  state = std::make_unique<RevealableState>(*view, std::move(options));
  if (enabled_) {
    state->OnRevealEnabled();
  }
}

void EdgeRevealController::RemoveRevealableView(views::View* view) {
  revealable_views_.erase(view);
}

void EdgeRevealController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void EdgeRevealController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void EdgeRevealController::SetEnabled(bool enabled) {
  if (enabled_ == enabled) {
    return;
  }

  enabled_ = enabled;

  if (enabled_) {
    event_monitor_ = views::EventMonitor::CreateWindowMonitor(
        this, widget_->GetNativeWindow(),
        {ui::EventType::kMouseMoved, ui::EventType::kMouseExited});
    if (auto* focus_manager = widget_->GetFocusManager()) {
      focus_manager->AddFocusChangeListener(this);
    }
    animation_.Reset(1.0);
    observers_.Notify(&Observer::OnEdgeRevealFractionChanged, 1.0);
    for (auto& [view, state] : revealable_views_) {
      state->OnRevealEnabled();
    }
    UpdateRevealState();
  } else {
    event_monitor_.reset();
    if (auto* focus_manager = widget_->GetFocusManager()) {
      focus_manager->RemoveFocusChangeListener(this);
    }
    for (auto& bubble_widget : observed_bubble_widgets_) {
      bubble_widget->RemoveObserver(this);
    }
    observed_bubble_widgets_.clear();
    mouse_hover_timer_.Stop();
    temporary_reveal_timer_.Stop();
    hovering_ = false;
    temporary_reveal_ = false;
    animation_.Reset(1.0);
    observers_.Notify(&Observer::OnEdgeRevealFractionChanged, 1.0);
    for (auto& [view, state] : revealable_views_) {
      state->OnRevealDisabled();
    }
  }
}

double EdgeRevealController::GetRevealFraction() const {
  return animation_.GetCurrentValue();
}

bool EdgeRevealController::IsRevealed() const {
  return !enabled_ || GetRevealFraction() > 0.0;
}

void EdgeRevealController::RevealTemporarily(base::TimeDelta duration) {
  if (!enabled_) {
    return;
  }
  temporary_reveal_ = true;
  UpdateRevealState();
  temporary_reveal_timer_.Start(
      FROM_HERE, duration,
      base::BindOnce(&EdgeRevealController::OnTemporaryRevealTimerElapsed,
                     base::Unretained(this)));
}

bool EdgeRevealController::ShouldReveal() const {
  return hovering_ || temporary_reveal_ || HasFocusInRevealableViews() ||
         !observed_bubble_widgets_.empty();
}

bool EdgeRevealController::HasFocusInRevealableViews() const {
  if (auto* focus_manager = widget_->GetFocusManager()) {
    if (auto* view = focus_manager->GetFocusedView()) {
      return ContainsView(view);
    }
  }
  return false;
}

bool EdgeRevealController::IsPointInRevealableViews(gfx::Point point) const {
  if (!IsRevealed()) {
    return false;
  }
  for (auto& [view, state] : revealable_views_) {
    if (view->GetBoundsInScreen().Contains(point)) {
      return true;
    }
  }
  return false;
}

bool EdgeRevealController::ContainsView(const views::View* view) const {
  return std::any_of(
      revealable_views_.begin(), revealable_views_.end(),
      [view](const auto& entry) { return entry.first->Contains(view); });
}

void EdgeRevealController::UpdateRevealState() {
  if (!enabled_) {
    return;
  }
  if (ShouldReveal()) {
    animation_.Show();
  } else {
    animation_.Hide();
  }
}

void EdgeRevealController::ScanForAnchoredBubbles() {
  for (auto& child_widget :
       views::Widget::GetAllChildWidgets(widget_->GetNativeView())) {
    if (child_widget.get() == &widget_.get() || !child_widget->IsVisible()) {
      continue;
    }
    if (observed_bubble_widgets_.contains(child_widget)) {
      continue;
    }
    auto* bubble = child_widget->widget_delegate()->AsBubbleDialogDelegate();
    if (bubble && bubble->GetAnchorView() &&
        ContainsView(bubble->GetAnchorView())) {
      child_widget->AddObserver(this);
      observed_bubble_widgets_.emplace(child_widget.get());
    }
  }
}

void EdgeRevealController::SetHoveringWithDelay(bool hovering) {
  if (hovering == hovering_) {
    mouse_hover_timer_.Stop();
    return;
  }
  if (mouse_hover_timer_.IsRunning()) {
    return;
  }
  mouse_hover_timer_.Start(
      FROM_HERE, hovering ? kMouseEnterDelay : kMouseExitDelay,
      base::BindOnce(&EdgeRevealController::OnHoverTimerElapsed,
                     base::Unretained(this), hovering));
}

void EdgeRevealController::OnHoverTimerElapsed(bool hovering) {
  hovering_ = hovering;
  if (!hovering_) {
    ScanForAnchoredBubbles();
  }
  UpdateRevealState();
}

void EdgeRevealController::OnTemporaryRevealTimerElapsed() {
  temporary_reveal_ = false;
  UpdateRevealState();
}

void EdgeRevealController::OnEvent(const ui::Event& event) {
  if (!enabled_) {
    return;
  }

  if (event.type() == ui::EventType::kMouseExited) {
    SetHoveringWithDelay(false);
    return;
  }

  if (event.type() != ui::EventType::kMouseMoved) {
    return;
  }

  const gfx::Point cursor = display::Screen::Get()->GetCursorScreenPoint();
  const gfx::Rect hot_zone =
      GetHotZoneForEdge(edge_, widget_->GetWindowBoundsInScreen());
  bool hovering = hot_zone.Contains(cursor) || IsPointInRevealableViews(cursor);

  SetHoveringWithDelay(hovering);
}

void EdgeRevealController::OnDidChangeFocus(views::View* before,
                                            views::View* after) {
  if (ContainsView(before) != ContainsView(after)) {
    ScanForAnchoredBubbles();
    UpdateRevealState();
  }
}

void EdgeRevealController::OnWidgetVisibilityChanged(views::Widget* widget,
                                                     bool visible) {
  if (!visible) {
    widget->RemoveObserver(this);
    observed_bubble_widgets_.erase(widget);
    UpdateRevealState();
  }
}

void EdgeRevealController::OnWidgetDestroying(views::Widget* widget) {
  widget->RemoveObserver(this);
  observed_bubble_widgets_.erase(widget);
  UpdateRevealState();
}

void EdgeRevealController::AnimationProgressed(
    const gfx::Animation* animation) {
  observers_.Notify(&Observer::OnEdgeRevealFractionChanged,
                    animation_.GetCurrentValue());
}

void EdgeRevealController::AnimationEnded(const gfx::Animation* animation) {
  observers_.Notify(&Observer::OnEdgeRevealFractionChanged,
                    animation_.GetCurrentValue());
}
