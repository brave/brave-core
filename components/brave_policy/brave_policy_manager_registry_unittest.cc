/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_policy_manager_registry.h"

#include "base/functional/bind.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_policy {

namespace {

// Minimal policy-manager-like host used to exercise the registry without
// depending on real singletons. Owns a registration whose callback reports
// the host's initialized flag.
class FakePolicyManager {
 public:
  FakePolicyManager()
      : initialized_(false),
        registration_(base::BindRepeating(&FakePolicyManager::IsInitialized,
                                          base::Unretained(this))) {}

  bool IsInitialized() const { return initialized_; }
  void SetInitialized(bool initialized) { initialized_ = initialized; }

 private:
  // `initialized_` must be declared before `registration_` so it is
  // initialized first; the registration captures a callback that reads
  // `initialized_`.
  bool initialized_;
  BravePolicyManagerRegistration registration_;
};

}  // namespace

// Construction adds an entry to the registry; destruction removes it.
TEST(BravePolicyManagerRegistryTest,
     ConstructionRegistersAndDestructionUnregisters) {
  const size_t baseline = BravePolicyManagerRegistry::SizeForTesting();
  {
    FakePolicyManager fake;
    EXPECT_EQ(baseline + 1, BravePolicyManagerRegistry::SizeForTesting());
  }
  EXPECT_EQ(baseline, BravePolicyManagerRegistry::SizeForTesting());
}

// `AllInitialized` returns false as long as any registered manager
// reports not initialized.
TEST(BravePolicyManagerRegistryTest,
     AllInitializedFalseWithUninitializedManager) {
  FakePolicyManager fake;
  EXPECT_FALSE(BravePolicyManagerRegistry::AllInitialized());
}

// Even with an initialized manager present, an uninitialized one
// elsewhere forces `AllInitialized()` false.
TEST(BravePolicyManagerRegistryTest, AllInitializedFalseWhenAnyUninitialized) {
  FakePolicyManager fake_initialized;
  fake_initialized.SetInitialized(true);

  FakePolicyManager fake_uninitialized;
  EXPECT_FALSE(BravePolicyManagerRegistry::AllInitialized());
}

// After an uninitialized manager is destroyed, it no longer holds
// `AllInitialized()` false: the result reverts to the baseline observed
// before it existed.
TEST(BravePolicyManagerRegistryTest,
     DestroyedManagerNoLongerAffectsAllInitialized) {
  FakePolicyManager fake_outer;
  fake_outer.SetInitialized(true);
  const bool baseline = BravePolicyManagerRegistry::AllInitialized();
  {
    FakePolicyManager fake_inner;  // uninitialized
    EXPECT_FALSE(BravePolicyManagerRegistry::AllInitialized());
  }
  EXPECT_EQ(baseline, BravePolicyManagerRegistry::AllInitialized());
}

}  // namespace brave_policy
