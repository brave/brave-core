/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_POLICY_MANAGER_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_POLICY_MANAGER_REGISTRY_H_

#include <cstddef>

#include "base/functional/callback.h"

namespace brave_policy {

// Process-wide registry of Brave-side policy managers. Used by callers
// (e.g. `BraveProfilePolicyProvider`) that need to wait for every
// participating manager to report initialized before refreshing policies.
//
// Managers participate by holding a `BravePolicyManagerRegistration`
// member that captures their `IsInitialized()` predicate. Registration
// happens automatically in the registration's constructor and
// unregistration in its destructor.
//
// Single-threaded by design: registration happens during early
// browser-process startup on the UI thread, before worker threads access
// these managers.
class BravePolicyManagerRegistry {
 public:
  // Returns true once every registered manager's `IsInitialized()`
  // callback returns true.
  static bool AllInitialized();

  // Returns the number of registered managers. For tests only.
  static size_t SizeForTesting();
};

// Registers itself with `BravePolicyManagerRegistry` on construction and
// unregisters on destruction. Construct as a member of any policy-manager
// class to participate in `AllInitialized()`.
//
// Member ordering matters: declare this *after* anything the
// `IsInitialized()` callback reads from, so the predicate isn't invoked
// against a partially constructed object during a future enclosing-class
// refactor.
class BravePolicyManagerRegistration {
 public:
  using IsInitializedCallback = base::RepeatingCallback<bool()>;

  explicit BravePolicyManagerRegistration(IsInitializedCallback is_initialized);
  ~BravePolicyManagerRegistration();

  BravePolicyManagerRegistration(const BravePolicyManagerRegistration&) =
      delete;
  BravePolicyManagerRegistration& operator=(
      const BravePolicyManagerRegistration&) = delete;

  bool IsInitialized() const;

 private:
  const IsInitializedCallback is_initialized_;
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_POLICY_MANAGER_REGISTRY_H_
