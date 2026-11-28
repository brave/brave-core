/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_dependent_services.h"

#include <utility>

#include "base/check_deref.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"

namespace brave_account {

BraveAccountDependentServices::BraveAccountDependentServices() {
  registrations_.set_disconnect_handler(base::BindRepeating(
      &BraveAccountDependentServices::NotifyObservers, base::Unretained(this)));
}

BraveAccountDependentServices::~BraveAccountDependentServices() = default;

void BraveAccountDependentServices::BindInterface(
    mojo::PendingReceiver<mojom::DependentServices> pending_receiver) {
  services_receivers_.Add(this, std::move(pending_receiver));
}

void BraveAccountDependentServices::BindInterface(
    mojo::PendingReceiver<mojom::DependentServiceRegistrar> pending_receiver) {
  registrar_receivers_.Add(this, std::move(pending_receiver));
}

void BraveAccountDependentServices::AddObserver(
    mojo::PendingRemote<mojom::DependentServicesObserver> observer) {
  const auto observer_id = observers_.Add(std::move(observer));
  CHECK_DEREF(observers_.Get(observer_id))
      .OnDependentServicesChanged(GetServices());
}

void BraveAccountDependentServices::Register(
    mojom::Service service,
    mojo::PendingReceiver<mojom::DependentServiceRegistration> registration) {
  registrations_.Add(this, std::move(registration), service);
  NotifyObservers();
}

std::vector<mojom::Service> BraveAccountDependentServices::GetServices() const {
  return base::MakeFlatSet<mojom::Service>(
             registrations_.GetAllContexts(), {},
             [](const auto& entry) { return *entry.second; })
      .extract();
}

void BraveAccountDependentServices::NotifyObservers() {
  for (const auto services = GetServices(); const auto& observer : observers_) {
    observer->OnDependentServicesChanged(services);
  }
}

}  // namespace brave_account
