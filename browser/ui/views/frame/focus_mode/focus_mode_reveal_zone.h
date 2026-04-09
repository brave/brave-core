/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_REVEAL_ZONE_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_REVEAL_ZONE_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/views/frame/focus_mode/focus_mode_types.h"
#include "ui/gfx/animation/animation_delegate.h"

namespace gfx {
class SlideAnimation;
}

// Per-edge state machine and animation for Focus Mode reveal/hide. Each zone
// independently tracks its own RevealState and manages a ref-counted set of
// reveal locks. Modeled on chromeos::ImmersiveFullscreenController.
class FocusModeRevealZone : public gfx::AnimationDelegate {
 public:
  enum class State { kClosed, kSlidingOpen, kRevealed, kSlidingClosed };

  using FractionChangedCallback = base::RepeatingCallback<void(double)>;

  explicit FocusModeRevealZone(FractionChangedCallback on_fraction_changed);
  ~FocusModeRevealZone() override;

  FocusModeRevealZone(const FocusModeRevealZone&) = delete;
  FocusModeRevealZone& operator=(const FocusModeRevealZone&) = delete;

  State state() const { return state_; }
  double visible_fraction() const { return visible_fraction_; }

  // Slides the zone open. No-op if already kReveled or kSlidingOpen. Reverses
  // a running close animation if kSlidingClosed.
  void MaybeStartReveal(FocusModeTransition transition);

  // Slides the zone closed. No-op if any locks are held, or if already kClosed
  // or kSlidingClosed.
  void MaybeEndReveal(FocusModeTransition transition);

  // Increment / decrement the lock ref-count. LockRevealedState triggers a
  // reveal; UnlockRevealedState triggers a hide when the count reaches zero.
  void LockRevealedState(FocusModeTransition transition);
  void UnlockRevealedState();

  // Force the zone back to kClosed / fraction 0 and invalidate all
  // outstanding WeakPtrs (making any live FocusModeRevealedLocks no-ops).
  void Reset();

  base::WeakPtr<FocusModeRevealZone> GetWeakPtr();

  // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

 private:
  void OnSlideOpenAnimationCompleted();
  void OnSlideClosedAnimationCompleted();
  void SetVisibleFraction(double fraction);
  base::TimeDelta GetAnimationDuration(FocusModeTransition transition) const;

  State state_ = State::kClosed;
  int lock_count_ = 0;
  double visible_fraction_ = 0.0;
  FractionChangedCallback on_fraction_changed_;
  std::unique_ptr<gfx::SlideAnimation> animation_;
  base::WeakPtrFactory<FocusModeRevealZone> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_REVEAL_ZONE_H_
