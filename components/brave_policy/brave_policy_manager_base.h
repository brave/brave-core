/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_POLICY_MANAGER_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_POLICY_MANAGER_BASE_H_

namespace brave_policy {

// Base class for Brave-side policy managers (e.g. `BraveOriginPolicyManager`,
// `AdBlockOnlyModePolicyManager`). Each derived manager self-registers in
// the constructor so a single shared `AllInitialized()` query covers every
// existing manager.
//
// This is what callers like `BraveProfilePolicyProvider` use to gate a
// policy refresh on every observed manager being ready, instead of having
// to hard-code a list of `IsInitialized()` checks (and silently regress
// when a new manager is added).
//
// Derived classes must override `IsInitialized()` to report whether their
// own initialization (typically an explicit `Init(...)` call) has completed.
//
// Single-threaded by design: registration happens in the manager's
// constructor, which runs during early browser-process startup on the UI
// thread before any worker threads access these singletons.
class BravePolicyManagerBase {
 public:
  // Returns true once every registered manager reports `IsInitialized()`
  // true. Use this from policy providers / observers that depend on
  // multiple managers being ready before pushing a complete bundle.
  static bool AllInitialized();

  // Returns the number of registered managers. For tests only.
  static int RegistrySizeForTesting();

  BravePolicyManagerBase(const BravePolicyManagerBase&) = delete;
  BravePolicyManagerBase& operator=(const BravePolicyManagerBase&) = delete;

  virtual bool IsInitialized() const = 0;

 protected:
  BravePolicyManagerBase();
  virtual ~BravePolicyManagerBase();
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_POLICY_MANAGER_BASE_H_
