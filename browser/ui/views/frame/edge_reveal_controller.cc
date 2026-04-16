/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/edge_reveal_controller.h"

#include <algorithm>

#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/event_monitor.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr int kHotZoneThickness = 7;
constexpr base::TimeDelta kRevealAnimationDuration = base::Milliseconds(200);

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

EdgeRevealController::EdgeRevealController(Edge edge, views::Widget* widget)
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

void EdgeRevealController::AddRevealableView(views::View* view) {
  revealable_views_.emplace(view);
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
    observers_.Notify(&Observer::OnEdgeRevealFractionChanged, 0.0);
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
    hovering_ = false;
    animation_.Reset(1.0);
    observers_.Notify(&Observer::OnEdgeRevealFractionChanged, 1.0);
  }
}

double EdgeRevealController::GetRevealFraction() const {
  return animation_.GetCurrentValue();
}

bool EdgeRevealController::IsRevealed() const {
  return !enabled_ || GetRevealFraction() > 0.0;
}

bool EdgeRevealController::ShouldReveal() const {
  return hovering_ || HasFocusInRevealableViews() ||
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

bool EdgeRevealController::ContainsView(const views::View* view) const {
  return std::any_of(
      revealable_views_.begin(), revealable_views_.end(),
      [view](const auto& revealable) { return revealable->Contains(view); });
}

void EdgeRevealController::ScanForAnchoredBubbles() {
  for (auto& child_widget :
       views::Widget::GetAllChildWidgets(widget_->GetNativeView())) {
    if (child_widget == widget_ || !child_widget->IsVisible()) {
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

// ui::EventObserver
void EdgeRevealController::OnEvent(const ui::Event& event) {
  if (!enabled_) {
    return;
  }

  if (event.type() == ui::EventType::kMouseExited) {
    if (hovering_) {
      hovering_ = false;
      UpdateRevealState();
    }
    return;
  }

  if (event.type() != ui::EventType::kMouseMoved) {
    return;
  }

  const gfx::Point cursor = display::Screen::Get()->GetCursorScreenPoint();
  const gfx::Rect hot_zone =
      GetHotZoneForEdge(edge_, widget_->GetWindowBoundsInScreen());

  bool hovering = hot_zone.Contains(cursor);

  // Also treat the area occupied by revealed views as part of the zone.
  if (!hovering && IsRevealed()) {
    for (auto& view : revealable_views_) {
      gfx::Rect bounds = view->GetBoundsInScreen();
      if (bounds.Contains(cursor)) {
        hovering = true;
        break;
      }
    }
  }

  if (hovering != hovering_) {
    hovering_ = hovering;
    UpdateRevealState();
  }
}

// views::FocusChangeListener
void EdgeRevealController::OnDidChangeFocus(views::View* before,
                                            views::View* after) {
  ScanForAnchoredBubbles();
  UpdateRevealState();
}

// views::WidgetObserver — tracks bubble lifecycle.
void EdgeRevealController::OnWidgetVisibilityChanged(views::Widget* widget,
                                                     bool visible) {
  UpdateRevealState();
  if (!visible) {
    widget->RemoveObserver(this);
    observed_bubble_widgets_.erase(widget);
  }
}

void EdgeRevealController::OnWidgetDestroying(views::Widget* widget) {
  widget->RemoveObserver(this);
  observed_bubble_widgets_.erase(widget);
  UpdateRevealState();
}

// gfx::AnimationDelegate
void EdgeRevealController::AnimationProgressed(
    const gfx::Animation* animation) {
  observers_.Notify(&Observer::OnEdgeRevealFractionChanged,
                    animation_.GetCurrentValue());
}

void EdgeRevealController::AnimationEnded(const gfx::Animation* animation) {
  observers_.Notify(&Observer::OnEdgeRevealFractionChanged,
                    animation_.GetCurrentValue());
}
