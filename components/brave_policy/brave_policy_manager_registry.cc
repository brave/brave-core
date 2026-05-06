/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_policy_manager_registry.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"

namespace brave_policy {

namespace {

// Function-local static to avoid static-init-order issues with policy
// managers that may register during static init via `base::NoDestructor`
// singletons.
std::vector<raw_ptr<BravePolicyManagerRegistration>>& Registry() {
  static base::NoDestructor<
      std::vector<raw_ptr<BravePolicyManagerRegistration>>>
      registry;
  return *registry;
}

}  // namespace

// static
bool BravePolicyManagerRegistry::AllInitialized() {
  return std::ranges::all_of(Registry(), [](const auto& registration) {
    return registration->IsInitialized();
  });
}

// static
size_t BravePolicyManagerRegistry::SizeForTesting() {
  return Registry().size();
}

BravePolicyManagerRegistration::BravePolicyManagerRegistration(
    IsInitializedCallback is_initialized)
    : is_initialized_(std::move(is_initialized)) {
  Registry().push_back(this);
}

BravePolicyManagerRegistration::~BravePolicyManagerRegistration() {
  std::erase(Registry(), this);
}

bool BravePolicyManagerRegistration::IsInitialized() const {
  return is_initialized_.Run();
}

}  // namespace brave_policy
