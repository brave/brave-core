/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DEPENDENT_SERVICES_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DEPENDENT_SERVICES_H_

#include <vector>

#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/mojom/dependent_services.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace brave_account {

// Tracks which services currently use Brave Account, so the Account UI can
// list them. Account itself knows nothing about the services: each one
// registers through `mojom::DependentServiceRegistrar` while it uses Account,
// and holds the `mojom::DependentServiceRegistration` for as long as it does.
// Closing it unregisters the service. A service that can run with or without
// Account (e.g. Sync) registers when it starts using Account and closes the
// registration when it stops. Others close it in `Shutdown()`.
//
// Like `BraveAccountService`, this class's only public entry points are the
// `BindInterface()` overloads. WebUI binds `mojom::DependentServices` to read
// the list, and only browser code binds `mojom::DependentServiceRegistrar`.
class BraveAccountDependentServices
    : public KeyedService,
      public mojom::DependentServices,
      public mojom::DependentServiceRegistrar,
      public mojom::DependentServiceRegistration {
 public:
  BraveAccountDependentServices();

  BraveAccountDependentServices(const BraveAccountDependentServices&) = delete;
  BraveAccountDependentServices& operator=(
      const BraveAccountDependentServices&) = delete;

  ~BraveAccountDependentServices() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::DependentServices> pending_receiver);

  void BindInterface(
      mojo::PendingReceiver<mojom::DependentServiceRegistrar> pending_receiver);

 private:
  // mojom::DependentServices:
  void AddObserver(
      mojo::PendingRemote<mojom::DependentServicesObserver> observer) override;

  // mojom::DependentServiceRegistrar:
  void Register(mojom::Service service,
                mojo::PendingReceiver<mojom::DependentServiceRegistration>
                    registration) override;

  std::vector<mojom::Service> GetServices() const;

  void NotifyObservers();

  mojo::ReceiverSet<mojom::DependentServices> services_receivers_;
  mojo::ReceiverSet<mojom::DependentServiceRegistrar> registrar_receivers_;
  mojo::ReceiverSet<mojom::DependentServiceRegistration, mojom::Service>
      registrations_;
  mojo::RemoteSet<mojom::DependentServicesObserver> observers_;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DEPENDENT_SERVICES_H_
