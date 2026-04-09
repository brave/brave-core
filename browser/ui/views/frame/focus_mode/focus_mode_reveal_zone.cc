/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode/focus_mode_reveal_zone.h"

#include <utility>

#include "base/check_op.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/scoped_animation_duration_scale_mode.h"

namespace {

constexpr base::TimeDelta kRevealDuration = base::Milliseconds(200);

}  // namespace

FocusModeRevealZone::FocusModeRevealZone(
    FractionChangedCallback on_fraction_changed)
    : on_fraction_changed_(std::move(on_fraction_changed)),
      animation_(std::make_unique<gfx::SlideAnimation>(this)) {}

FocusModeRevealZone::~FocusModeRevealZone() = default;

void FocusModeRevealZone::MaybeStartReveal(FocusModeTransition transition) {
  if (state_ == State::kRevealed || state_ == State::kSlidingOpen) {
    return;
  }

  state_ = State::kSlidingOpen;

  base::TimeDelta duration = GetAnimationDuration(transition);
  if (duration.is_positive()) {
    animation_->SetSlideDuration(duration);
    animation_->Show();
  } else {
    animation_->Reset(1);
    OnSlideOpenAnimationCompleted();
  }
}

void FocusModeRevealZone::MaybeEndReveal(FocusModeTransition transition) {
  if (lock_count_ != 0) {
    return;
  }

  if (state_ == State::kClosed || state_ == State::kSlidingClosed) {
    return;
  }

  state_ = State::kSlidingClosed;

  base::TimeDelta duration = GetAnimationDuration(transition);
  if (duration.is_positive()) {
    animation_->SetSlideDuration(duration);
    animation_->Hide();
  } else {
    animation_->Reset(0);
    OnSlideClosedAnimationCompleted();
  }
}

void FocusModeRevealZone::LockRevealedState(FocusModeTransition transition) {
  ++lock_count_;
  MaybeStartReveal(transition);
}

void FocusModeRevealZone::UnlockRevealedState() {
  --lock_count_;
  DCHECK_GE(lock_count_, 0);
  if (lock_count_ == 0) {
    MaybeEndReveal(FocusModeTransition::kAnimated);
  }
}

void FocusModeRevealZone::Reset() {
  animation_->Stop();
  state_ = State::kClosed;
  lock_count_ = 0;
  SetVisibleFraction(0.0);
  weak_factory_.InvalidateWeakPtrs();
}

base::WeakPtr<FocusModeRevealZone> FocusModeRevealZone::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void FocusModeRevealZone::AnimationProgressed(const gfx::Animation* animation) {
  SetVisibleFraction(animation->GetCurrentValue());
}

void FocusModeRevealZone::AnimationEnded(const gfx::Animation* animation) {
  if (state_ == State::kSlidingOpen) {
    OnSlideOpenAnimationCompleted();
  } else if (state_ == State::kSlidingClosed) {
    OnSlideClosedAnimationCompleted();
  }
}

void FocusModeRevealZone::OnSlideOpenAnimationCompleted() {
  DCHECK_EQ(State::kSlidingOpen, state_);
  state_ = State::kRevealed;
  SetVisibleFraction(1.0);
}

void FocusModeRevealZone::OnSlideClosedAnimationCompleted() {
  DCHECK_EQ(State::kSlidingClosed, state_);
  state_ = State::kClosed;
  SetVisibleFraction(0.0);
}

void FocusModeRevealZone::SetVisibleFraction(double fraction) {
  if (visible_fraction_ == fraction) {
    return;
  }
  visible_fraction_ = fraction;
  on_fraction_changed_.Run(fraction);
}

base::TimeDelta FocusModeRevealZone::GetAnimationDuration(
    FocusModeTransition transition) const {
  if (transition == FocusModeTransition::kInstant) {
    return base::TimeDelta();
  }
  return gfx::ScopedAnimationDurationScaleMode::duration_multiplier() *
         kRevealDuration;
}
