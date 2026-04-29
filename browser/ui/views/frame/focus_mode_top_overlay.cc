/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode_top_overlay.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/views/tabs/tab_strip_placement_coordinator.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/events/event_observer.h"
#include "ui/views/event_monitor.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr int kHotZoneThickness = 16;
constexpr base::TimeDelta kRevealAnimationDuration = base::Milliseconds(200);
constexpr base::TimeDelta kMouseEnterDelay = base::Milliseconds(60);
constexpr base::TimeDelta kMouseExitDelay = base::Milliseconds(300);

constexpr ViewShadow::ShadowParameters kShadow{
    .offset_x = 0,
    .offset_y = 2,
    .blur_radius = 8,
    .shadow_color = SkColorSetA(SK_ColorBLACK, 0.15 * 255)};

gfx::Rect GetTopHotZone(const gfx::Rect& window_bounds) {
  gfx::Rect rect(window_bounds);
  rect.set_height(kHotZoneThickness);
  return rect;
}

}  // namespace

class FocusModeTopOverlay::WindowEventHandler : public ui::EventObserver {
 public:
  WindowEventHandler(FocusModeTopOverlay* overlay, gfx::NativeWindow window)
      : overlay_(overlay) {
    monitor_ = views::EventMonitor::CreateWindowMonitor(
        this, window,
        {ui::EventType::kMouseMoved, ui::EventType::kMouseExited});
  }
  WindowEventHandler(const WindowEventHandler&) = delete;
  WindowEventHandler& operator=(const WindowEventHandler&) = delete;
  ~WindowEventHandler() override = default;

 private:
  void OnEvent(const ui::Event& event) override {
    overlay_->OnWindowEvent(event);
  }

  raw_ptr<FocusModeTopOverlay> overlay_;
  std::unique_ptr<views::EventMonitor> monitor_;
};

FocusModeTopOverlay::FocusModeTopOverlay(
    views::View* top_container,
    TabStripPlacementCoordinator* tab_strip_placement)
    : top_container_(top_container),
      tab_strip_placement_(tab_strip_placement),
      shadow_(this, gfx::RoundedCornersF(0), kShadow) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(true);
  animation_.SetSlideDuration(kRevealAnimationDuration);
  animation_.Reset(1.0);
  SetVisible(false);

  tab_strip_placement_->SetPlacement(TabStripPlacementKind::kTopContainer,
                                     top_container_.get(), 0);
}

FocusModeTopOverlay::~FocusModeTopOverlay() {
  if (active_) {
    StopRevealing();
  }
}

void FocusModeTopOverlay::Activate() {
  if (active_) {
    return;
  }

  CHECK(top_container_);
  CHECK(GetWidget());

  active_ = true;

  // Move tab strip into top container, if appropriate.
  tab_strip_placement_->UpdatePlacement();

  // Move top container into overlay.
  auto* top_container = top_container_.get();
  top_container_parent_ = top_container->parent();
  top_container_index_ = top_container_parent_->GetIndexOf(top_container);
  AddChildView(top_container);

  view_observations_.AddObservation(top_container);

  window_event_handler_ = std::make_unique<WindowEventHandler>(
      this, GetWidget()->GetNativeWindow());

  if (auto* focus_manager = GetWidget()->GetFocusManager()) {
    focus_manager->AddFocusChangeListener(this);
  }

  animation_.Reset(1.0);
  SetVisible(true);
  UpdateBounds();
  NotifyRevealFractionChanged();
  UpdateRevealState();
}

void FocusModeTopOverlay::Deactivate() {
  if (!active_) {
    return;
  }

  active_ = false;
  StopRevealing();

  // Restore top container placement.
  if (auto* parent = top_container_parent_.get()) {
    size_t index = top_container_index_.value_or(parent->children().size());
    CHECK(index <= parent->children().size());
    parent->AddChildViewAt(top_container_.get(), index);
    top_container_parent_ = nullptr;
    top_container_index_.reset();
  }

  // Restore tab strip placement if necessary.
  tab_strip_placement_->UpdatePlacement();

  SetVisible(false);
  NotifyRevealFractionChanged();
}

void FocusModeTopOverlay::RevealTemporarily(base::TimeDelta duration) {
  if (!active_) {
    return;
  }
  temporary_reveal_ = true;
  UpdateRevealState();
  temporary_reveal_timer_.Start(
      FROM_HERE, duration,
      base::BindOnce(&FocusModeTopOverlay::OnTemporaryRevealTimerElapsed,
                     base::Unretained(this)));
}

double FocusModeTopOverlay::GetRevealFraction() const {
  return animation_.GetCurrentValue();
}

base::CallbackListSubscription
FocusModeTopOverlay::AddRevealFractionChangedCallback(
    RevealFractionChangedCallback callback) {
  return reveal_fraction_changed_callbacks_.Add(std::move(callback));
}

void FocusModeTopOverlay::StopRevealing() {
  if (auto* widget = GetWidget()) {
    if (auto* focus_manager = widget->GetFocusManager()) {
      focus_manager->RemoveFocusChangeListener(this);
    }
  }

  window_event_handler_.reset();
  mouse_hover_timer_.Stop();
  temporary_reveal_timer_.Stop();
  hovering_ = false;
  temporary_reveal_ = false;
  animation_.Reset(1.0);
  view_observations_.RemoveAllObservations();
}

bool FocusModeTopOverlay::ShouldReveal() const {
  return hovering_ || temporary_reveal_ || HasFocusInHostedViews();
}

bool FocusModeTopOverlay::HasFocusInHostedViews() const {
  if (auto* widget = GetWidget()) {
    if (auto* focus_manager = widget->GetFocusManager()) {
      if (auto* view = focus_manager->GetFocusedView()) {
        return Contains(view);
      }
    }
  }
  return false;
}

bool FocusModeTopOverlay::IsPointInHostedViews(gfx::Point point) const {
  if (animation_.GetCurrentValue() == 0.0) {
    return false;
  }
  return GetBoundsInScreen().Contains(point);
}

void FocusModeTopOverlay::UpdateRevealState() {
  if (!active_) {
    return;
  }
  if (ShouldReveal()) {
    animation_.Show();
  } else {
    animation_.Hide();
  }
}

void FocusModeTopOverlay::UpdateBounds() {
  if (!active_ || !parent() || !top_container_) {
    return;
  }
  int height = top_container_->bounds().height();
  double fraction = animation_.GetCurrentValue();
  int y_offset = -static_cast<int>((1.0 - fraction) * height);
  SetBoundsRect(gfx::Rect(0, y_offset, parent()->width(), height));
}

void FocusModeTopOverlay::NotifyRevealFractionChanged() {
  reveal_fraction_changed_callbacks_.Notify(animation_.GetCurrentValue());
}

void FocusModeTopOverlay::SetHoveringWithDelay(bool hovering) {
  if (hovering == hovering_) {
    mouse_hover_timer_.Stop();
    return;
  }
  if (mouse_hover_timer_.IsRunning()) {
    return;
  }
  mouse_hover_timer_.Start(
      FROM_HERE, hovering ? kMouseEnterDelay : kMouseExitDelay,
      base::BindOnce(&FocusModeTopOverlay::OnHoverTimerElapsed,
                     base::Unretained(this), hovering));
}

void FocusModeTopOverlay::OnHoverTimerElapsed(bool hovering) {
  hovering_ = hovering;
  UpdateRevealState();
}

void FocusModeTopOverlay::OnTemporaryRevealTimerElapsed() {
  temporary_reveal_ = false;
  UpdateRevealState();
}

void FocusModeTopOverlay::OnWindowEvent(const ui::Event& event) {
  if (!active_) {
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
      GetTopHotZone(GetWidget()->GetWindowBoundsInScreen());
  bool hovering = hot_zone.Contains(cursor) || IsPointInHostedViews(cursor);
  SetHoveringWithDelay(hovering);
}

void FocusModeTopOverlay::OnDidChangeFocus(views::View* before,
                                           views::View* after) {
  if (Contains(before) != Contains(after)) {
    UpdateRevealState();
  }
}

void FocusModeTopOverlay::OnViewBoundsChanged(views::View* observed_view) {
  UpdateBounds();
}

void FocusModeTopOverlay::AnimationProgressed(const gfx::Animation* animation) {
  UpdateBounds();
  NotifyRevealFractionChanged();
}

void FocusModeTopOverlay::AnimationEnded(const gfx::Animation* animation) {
  UpdateBounds();
  NotifyRevealFractionChanged();
}

BEGIN_METADATA(FocusModeTopOverlay)
END_METADATA
