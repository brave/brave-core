/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_INITIALIZATION_WAITER_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_INITIALIZATION_WAITER_H_

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"

namespace brave_policy {

// Fires a one-shot callback when a `policy::PolicyService` finishes
// initialising the configured `policy::PolicyDomain`. Lets a service defer
// work until policy is ready without inheriting from
// `policy::PolicyService::Observer` itself.
//
// Usage:
//   auto waiter = std::make_unique<PolicyInitializationWaiter>(policy_service);
//   waiter->Wait(base::BindOnce(&Foo::Start, weak_ptr));
//
// A null `policy_service` is treated as already-ready, which keeps tests and
// no-policy code paths simple.
class PolicyInitializationWaiter : public policy::PolicyService::Observer {
 public:
  explicit PolicyInitializationWaiter(
      policy::PolicyService* policy_service,
      policy::PolicyDomain domain = policy::POLICY_DOMAIN_CHROME);
  PolicyInitializationWaiter(const PolicyInitializationWaiter&) = delete;
  PolicyInitializationWaiter& operator=(const PolicyInitializationWaiter&) =
      delete;
  ~PolicyInitializationWaiter() override;

  // Registers `on_ready` to fire once when the configured domain finishes
  // initialising. Runs synchronously if the waiter is already ready at call
  // time; otherwise posted asynchronously to the current sequence when
  // initialisation completes. A second call replaces any previously
  // registered callback.
  void Wait(base::OnceClosure on_ready);

 private:
  // policy::PolicyService::Observer:
  void OnPolicyServiceInitialized(policy::PolicyDomain domain) override;

  void StopObserving();

  const policy::PolicyDomain domain_;
  bool is_ready_ = false;
  base::OnceClosure on_ready_;
  raw_ptr<policy::PolicyService> observed_policy_service_ =
      nullptr;  // Not owned.
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_INITIALIZATION_WAITER_H_
