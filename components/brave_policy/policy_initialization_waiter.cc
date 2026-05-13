/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/policy_initialization_waiter.h"

#include <utility>

namespace brave_policy {

PolicyInitializationWaiter::PolicyInitializationWaiter(
    policy::PolicyService* policy_service,
    policy::PolicyDomain domain)
    : domain_(domain) {
  if (!policy_service || policy_service->IsInitializationComplete(domain_)) {
    is_ready_ = true;
    return;
  }
  policy_service->AddObserver(domain_, this);
  observed_policy_service_ = policy_service;
}

PolicyInitializationWaiter::~PolicyInitializationWaiter() {
  StopObserving();
}

void PolicyInitializationWaiter::Wait(base::OnceClosure on_ready) {
  if (is_ready_) {
    std::move(on_ready).Run();
    return;
  }
  on_ready_ = std::move(on_ready);
}

void PolicyInitializationWaiter::OnPolicyServiceInitialized(
    policy::PolicyDomain domain) {
  if (domain != domain_) {
    return;
  }
  is_ready_ = true;
  StopObserving();
  if (on_ready_) {
    std::move(on_ready_).Run();
  }
}

void PolicyInitializationWaiter::StopObserving() {
  if (!observed_policy_service_) {
    return;
  }
  observed_policy_service_->RemoveObserver(domain_, this);
  observed_policy_service_ = nullptr;
}

}  // namespace brave_policy
