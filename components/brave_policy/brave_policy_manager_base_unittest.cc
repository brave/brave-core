/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_policy_manager_base.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_policy {

namespace {

// Minimal `BravePolicyManagerBase` subclass with a settable initialization
// flag, used to exercise the registry without depending on real singletons.
class FakePolicyManager : public BravePolicyManagerBase {
 public:
  FakePolicyManager() = default;
  ~FakePolicyManager() override = default;

  bool IsInitialized() const override { return initialized_; }
  void SetInitialized(bool initialized) { initialized_ = initialized; }

 private:
  bool initialized_ = false;
};

}  // namespace

// Construction self-registers and destruction unregisters, so the registry
// size returns to its baseline after the manager goes out of scope.
TEST(BravePolicyManagerBaseTest,
     ConstructionRegistersAndDestructionUnregisters) {
  const int baseline = BravePolicyManagerBase::RegistrySizeForTesting();
  {
    FakePolicyManager fake;
    EXPECT_EQ(baseline + 1, BravePolicyManagerBase::RegistrySizeForTesting());
  }
  EXPECT_EQ(baseline, BravePolicyManagerBase::RegistrySizeForTesting());
}

// `AllInitialized` returns false as long as any registered manager reports
// `IsInitialized() == false`.
TEST(BravePolicyManagerBaseTest, AllInitializedFalseWithUninitializedManager) {
  FakePolicyManager fake;
  EXPECT_FALSE(BravePolicyManagerBase::AllInitialized());
}

// Flipping a manager to initialized removes it as a blocker for
// `AllInitialized`. The overall result depends on whatever else is in the
// registry, but adding *another* uninitialized manager forces it back to
// false regardless of prior state.
TEST(BravePolicyManagerBaseTest, AllInitializedFalseWhenAnyUninitialized) {
  FakePolicyManager fake_initialized;
  fake_initialized.SetInitialized(true);

  FakePolicyManager fake_uninitialized;
  EXPECT_FALSE(BravePolicyManagerBase::AllInitialized());
}

// After a manager is destroyed, it no longer participates in the
// `AllInitialized` calculation: an uninitialized manager that goes out of
// scope can no longer hold the result false.
TEST(BravePolicyManagerBaseTest,
     DestroyedManagerNoLongerAffectsAllInitialized) {
  FakePolicyManager fake_outer;
  fake_outer.SetInitialized(true);
  const bool baseline = BravePolicyManagerBase::AllInitialized();
  {
    FakePolicyManager fake_inner;  // uninitialized
    EXPECT_FALSE(BravePolicyManagerBase::AllInitialized());
  }
  // `fake_inner` is destroyed; only `fake_outer` (initialized) remains
  // among test-controlled fakes, so `AllInitialized` must equal the
  // baseline observed before `fake_inner` existed.
  EXPECT_EQ(baseline, BravePolicyManagerBase::AllInitialized());
}

}  // namespace brave_policy
