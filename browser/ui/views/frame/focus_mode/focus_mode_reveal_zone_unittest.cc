/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode/focus_mode_reveal_zone.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/test/task_environment.h"
#include "brave/browser/ui/views/frame/focus_mode/focus_mode_revealed_lock.h"
#include "brave/browser/ui/views/frame/focus_mode/focus_mode_types.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/scoped_animation_duration_scale_mode.h"

class FocusModeRevealZoneTest : public testing::Test {
 public:
  FocusModeRevealZoneTest()
      : zero_duration_(gfx::ScopedAnimationDurationScaleMode::ZERO_DURATION),
        zone_(base::BindRepeating(&FocusModeRevealZoneTest::OnFractionChanged,
                                  base::Unretained(this))) {}

  void OnFractionChanged(double fraction) {
    last_fraction_ = fraction;
    ++fraction_change_count_;
    fraction_history_.push_back(fraction);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  gfx::ScopedAnimationDurationScaleMode zero_duration_;
  FocusModeRevealZone zone_;
  double last_fraction_ = -1.0;
  int fraction_change_count_ = 0;
  std::vector<double> fraction_history_;
};

TEST_F(FocusModeRevealZoneTest, InitialState) {
  EXPECT_EQ(FocusModeRevealZone::State::kClosed, zone_.state());
  EXPECT_EQ(0.0, zone_.visible_fraction());
  EXPECT_EQ(0, fraction_change_count_);
}

TEST_F(FocusModeRevealZoneTest, LockRevealsAndUnlockCloses) {
  zone_.LockRevealedState(FocusModeTransition::kInstant);
  EXPECT_EQ(FocusModeRevealZone::State::kRevealed, zone_.state());
  EXPECT_EQ(1.0, zone_.visible_fraction());
  EXPECT_EQ(1.0, last_fraction_);

  zone_.UnlockRevealedState();
  EXPECT_EQ(FocusModeRevealZone::State::kClosed, zone_.state());
  EXPECT_EQ(0.0, zone_.visible_fraction());
  EXPECT_EQ(0.0, last_fraction_);
}

TEST_F(FocusModeRevealZoneTest, MultipleLocks) {
  zone_.LockRevealedState(FocusModeTransition::kInstant);
  zone_.LockRevealedState(FocusModeTransition::kInstant);
  EXPECT_EQ(FocusModeRevealZone::State::kRevealed, zone_.state());

  zone_.UnlockRevealedState();
  EXPECT_EQ(FocusModeRevealZone::State::kRevealed, zone_.state());
  EXPECT_EQ(1.0, zone_.visible_fraction());

  zone_.UnlockRevealedState();
  EXPECT_EQ(FocusModeRevealZone::State::kClosed, zone_.state());
  EXPECT_EQ(0.0, zone_.visible_fraction());
}

TEST_F(FocusModeRevealZoneTest, MaybeStartRevealNoOpWhenRevealed) {
  zone_.LockRevealedState(FocusModeTransition::kInstant);
  ASSERT_EQ(FocusModeRevealZone::State::kRevealed, zone_.state());

  int count = fraction_change_count_;
  zone_.MaybeStartReveal(FocusModeTransition::kInstant);
  EXPECT_EQ(count, fraction_change_count_);
}

TEST_F(FocusModeRevealZoneTest, MaybeEndRevealBlockedByLock) {
  zone_.LockRevealedState(FocusModeTransition::kInstant);
  zone_.MaybeEndReveal(FocusModeTransition::kInstant);
  EXPECT_EQ(FocusModeRevealZone::State::kRevealed, zone_.state());
  EXPECT_EQ(1.0, zone_.visible_fraction());
}

TEST_F(FocusModeRevealZoneTest, MaybeEndRevealNoOpWhenClosed) {
  zone_.MaybeEndReveal(FocusModeTransition::kInstant);
  EXPECT_EQ(FocusModeRevealZone::State::kClosed, zone_.state());
  EXPECT_EQ(0, fraction_change_count_);
}

TEST_F(FocusModeRevealZoneTest, RAIILockAcquireAndRelease) {
  {
    auto lock = std::make_unique<FocusModeRevealedLock>(
        zone_.GetWeakPtr(), FocusModeTransition::kInstant);
    EXPECT_EQ(FocusModeRevealZone::State::kRevealed, zone_.state());
    EXPECT_EQ(1.0, zone_.visible_fraction());
  }
  EXPECT_EQ(FocusModeRevealZone::State::kClosed, zone_.state());
  EXPECT_EQ(0.0, zone_.visible_fraction());
}

TEST_F(FocusModeRevealZoneTest, RAIILockMultiple) {
  auto lock1 = std::make_unique<FocusModeRevealedLock>(
      zone_.GetWeakPtr(), FocusModeTransition::kInstant);
  auto lock2 = std::make_unique<FocusModeRevealedLock>(
      zone_.GetWeakPtr(), FocusModeTransition::kInstant);
  EXPECT_EQ(FocusModeRevealZone::State::kRevealed, zone_.state());

  lock1.reset();
  EXPECT_EQ(FocusModeRevealZone::State::kRevealed, zone_.state());

  lock2.reset();
  EXPECT_EQ(FocusModeRevealZone::State::kClosed, zone_.state());
}

TEST_F(FocusModeRevealZoneTest, LockSurvivesZoneReset) {
  auto lock = std::make_unique<FocusModeRevealedLock>(
      zone_.GetWeakPtr(), FocusModeTransition::kInstant);
  EXPECT_EQ(FocusModeRevealZone::State::kRevealed, zone_.state());

  zone_.Reset();
  EXPECT_EQ(FocusModeRevealZone::State::kClosed, zone_.state());
  EXPECT_EQ(0.0, zone_.visible_fraction());

  // Lock's weak ptr is now invalid — destructor must not crash.
  lock.reset();
  EXPECT_EQ(FocusModeRevealZone::State::kClosed, zone_.state());
}

TEST_F(FocusModeRevealZoneTest, Reset) {
  zone_.LockRevealedState(FocusModeTransition::kInstant);
  ASSERT_EQ(FocusModeRevealZone::State::kRevealed, zone_.state());

  zone_.Reset();
  EXPECT_EQ(FocusModeRevealZone::State::kClosed, zone_.state());
  EXPECT_EQ(0.0, zone_.visible_fraction());
}

TEST_F(FocusModeRevealZoneTest, AnimateWithZeroDuration) {
  // ZERO_DURATION is set in the fixture. Animated calls should still complete
  // synchronously via the Reset()+callback path.
  zone_.LockRevealedState(FocusModeTransition::kAnimated);
  EXPECT_EQ(FocusModeRevealZone::State::kRevealed, zone_.state());
  EXPECT_EQ(1.0, zone_.visible_fraction());

  zone_.UnlockRevealedState();
  EXPECT_EQ(FocusModeRevealZone::State::kClosed, zone_.state());
  EXPECT_EQ(0.0, zone_.visible_fraction());
}

TEST_F(FocusModeRevealZoneTest, FractionCallbackFires) {
  zone_.LockRevealedState(FocusModeTransition::kInstant);
  zone_.UnlockRevealedState();

  // Open fires fraction 1.0, close fires fraction 0.0.
  ASSERT_EQ(2, fraction_change_count_);
  EXPECT_EQ(1.0, fraction_history_[0]);
  EXPECT_EQ(0.0, fraction_history_[1]);
}

TEST_F(FocusModeRevealZoneTest, NoRedundantFractionCallbacks) {
  // Two locks opening from CLOSED: only the first triggers the fraction change
  // since the second is a no-op on an already-REVEALED zone.
  zone_.LockRevealedState(FocusModeTransition::kInstant);
  int count = fraction_change_count_;
  zone_.LockRevealedState(FocusModeTransition::kInstant);
  EXPECT_EQ(count, fraction_change_count_);
}
