/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_policy_manager_base.h"

#include <algorithm>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"

namespace brave_policy {

namespace {

// Function-local static to avoid static-init-order issues with derived
// `base::NoDestructor` singletons that may register during static init.
std::vector<raw_ptr<BravePolicyManagerBase>>& Registry() {
  static base::NoDestructor<std::vector<raw_ptr<BravePolicyManagerBase>>>
      registry;
  return *registry;
}

}  // namespace

// static
bool BravePolicyManagerBase::AllInitialized() {
  for (const auto& manager : Registry()) {
    if (!manager->IsInitialized()) {
      return false;
    }
  }
  return true;
}

// static
int BravePolicyManagerBase::RegistrySizeForTesting() {
  return static_cast<int>(Registry().size());
}

BravePolicyManagerBase::BravePolicyManagerBase() {
  Registry().push_back(this);
}

BravePolicyManagerBase::~BravePolicyManagerBase() {
  std::erase(Registry(), this);
}

}  // namespace brave_policy
