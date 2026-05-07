/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode_top_overlay.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/edge_reveal/edge_hover_detector.h"
#include "brave/browser/ui/views/frame/edge_reveal/transient_widget_tracker.h"
#include "brave/browser/ui/views/frame/tab_strip_placement_coordinator.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr int kHotZoneThickness = 16;
constexpr base::TimeDelta kRevealAnimationDuration = base::Milliseconds(200);

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

TabStripPlacementCoordinator* GetPlacementCoordinator(
    BrowserView* browser_view) {
  CHECK(browser_view);
  return BraveBrowserView::From(browser_view)
      ->tab_strip_placement_coordinator();
}

void UpdateTabStripPlacement(BrowserView* browser_view) {
  if (auto* coordinator = GetPlacementCoordinator(browser_view)) {
    coordinator->UpdatePlacement();
  }
}

}  // namespace

FocusModeTopOverlay::RevealLock::RevealLock(
    base::WeakPtr<FocusModeTopOverlay> owner)
    : owner_(std::move(owner)) {
  owner_->OnRevealLockCreated();
}

FocusModeTopOverlay::RevealLock::RevealLock(const RevealLock&) = default;
FocusModeTopOverlay::RevealLock& FocusModeTopOverlay::RevealLock::operator=(
    const RevealLock&) = default;
FocusModeTopOverlay::RevealLock::RevealLock(RevealLock&&) = default;
FocusModeTopOverlay::RevealLock& FocusModeTopOverlay::RevealLock::operator=(
    RevealLock&&) = default;

FocusModeTopOverlay::RevealLock::~RevealLock() {
  if (owner_) {
    owner_->OnRevealLockDestroyed();
  }
}

FocusModeTopOverlay::FocusModeTopOverlay(base::PassKey<BraveBrowserView>,
                                         BrowserView* browser_view)
    : browser_view_(browser_view),
      shadow_(this, gfx::RoundedCornersF(0), kShadow) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(true);

  auto* coordinator = GetPlacementCoordinator(browser_view_);
  CHECK(coordinator);
  coordinator->SetPlacement(TabStripPlacementKind::kTopContainer,
                            browser_view_->top_container(), 0);

  animation_.SetSlideDuration(kRevealAnimationDuration);
  animation_.Reset(1.0);

  SetVisible(false);
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

  CHECK(GetWidget());

  active_ = true;

  // Move tab strip into top container, if appropriate.
  UpdateTabStripPlacement(browser_view_);

  // Move top container into overlay.
  auto* top_container = browser_view_->top_container();
  CHECK(top_container);
  top_container_index_ = browser_view_->GetIndexOf(top_container);
  AddChildView(top_container);

  top_container_observation_.Observe(top_container);

  hover_detector_ = std::make_unique<EdgeHoverDetector>(
      GetWidget(),
      base::BindRepeating(&FocusModeTopOverlay::IsHoverPoint,
                          base::Unretained(this)),
      base::BindRepeating(&FocusModeTopOverlay::OnHoverStateChanged,
                          base::Unretained(this)));

  reveal_watcher_ = std::make_unique<EdgeRevealWatcher>(
      GetWidget(),
      base::BindRepeating(&FocusModeTopOverlay::EdgeContainsView,
                          base::Unretained(this)),
      base::BindRepeating(&FocusModeTopOverlay::OnRevealReasonsChanged,
                          base::Unretained(this)));

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
  if (auto* top_container = top_container_observation_.GetSource()) {
    size_t max_index = browser_view_->children().size();
    browser_view_->AddChildViewAt(
        top_container,
        std::min(top_container_index_.value_or(max_index), max_index));
    top_container_observation_.Reset();
  }

  // Restore tab strip placement if necessary.
  UpdateTabStripPlacement(browser_view_);

  SetVisible(false);
  NotifyRevealFractionChanged();
}

void FocusModeTopOverlay::RevealTemporarily(base::TimeDelta duration) {
  if (!active_) {
    return;
  }
  temporary_reveal_lock_ = AcquireRevealLock();
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
  hover_detector_.reset();
  reveal_watcher_.reset();
  temporary_reveal_lock_.reset();
  temporary_reveal_timer_.Stop();
  animation_.Reset(1.0);
}

void FocusModeTopOverlay::SetRevealLock(std::optional<RevealLock>& lock,
                                        bool should_lock) {
  if (!should_lock) {
    lock.reset();
  } else if (!lock) {
    lock = AcquireRevealLock();
  }
}

void FocusModeTopOverlay::OnRevealLockCreated() {
  reveal_lock_count_ += 1;
  LOG(ERROR) << "Reveal lock count " << reveal_lock_count_;
  UpdateRevealState();
}

void FocusModeTopOverlay::OnRevealLockDestroyed() {
  if (reveal_lock_count_ > 0) {
    reveal_lock_count_ -= 1;
    LOG(ERROR) << "Reveal lock count " << reveal_lock_count_;
    UpdateRevealState();
  }
}

bool FocusModeTopOverlay::ShouldReveal() const {
  constexpr EdgeRevealWatcher::RevealReasons kForceRevealReasons = {
      EdgeRevealWatcher::RevealReason::kFocus,
      EdgeRevealWatcher::RevealReason::kAnchoredBubble};

  return hover_detector_->hovering() || reveal_lock_count_ > 0 ||
         reveal_watcher_->reasons().HasAny(kForceRevealReasons);
}

bool FocusModeTopOverlay::IsHoverPoint(const gfx::Point& screen_point) const {
  gfx::Rect hot_zone = GetTopHotZone(GetWidget()->GetWindowBoundsInScreen());
  if (hot_zone.Contains(screen_point)) {
    return true;
  }
  if (GetRevealFraction() == 0.0) {
    return false;
  }
  return GetBoundsInScreen().Contains(screen_point);
}

void FocusModeTopOverlay::OnHoverStateChanged(bool hovering) {
  LOG(ERROR) << "OnHoverStateChanged " << hovering;
  UpdateRevealState();
}

bool FocusModeTopOverlay::EdgeContainsView(const views::View* view) const {
  return Contains(view);
}

void FocusModeTopOverlay::OnRevealReasonsChanged(
    EdgeRevealWatcher::RevealReasons reasons) {
  LOG(ERROR) << "OnRevealReasonsChanged " << reasons.ToString();

  if (GetRevealFraction() > 0.0) {
    SetRevealLock(
        visible_transient_lock_,
        reasons.Has(EdgeRevealWatcher::RevealReason::kVisibleTransientChild));
  }

  UpdateRevealState();
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
  auto* top_container = top_container_observation_.GetSource();
  if (!top_container || !parent()) {
    return;
  }
  int height = top_container->bounds().height();
  int y_offset = -static_cast<int>((1.0 - GetRevealFraction()) * height);
  SetBoundsRect(gfx::Rect(0, y_offset, parent()->width(), height));
}

void FocusModeTopOverlay::NotifyRevealFractionChanged() {
  reveal_fraction_changed_callbacks_.Notify(GetRevealFraction());
}

void FocusModeTopOverlay::OnTemporaryRevealTimerElapsed() {
  temporary_reveal_lock_.reset();
  UpdateRevealState();
}

void FocusModeTopOverlay::OnViewBoundsChanged(views::View* observed_view) {
  UpdateBounds();
}

void FocusModeTopOverlay::OnViewIsDeleting(views::View* observed_view) {
  top_container_observation_.Reset();
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
