// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_MOCK_BRAVE_ACCOUNT_DEPENDENT_SERVICE_REGISTRAR_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_MOCK_BRAVE_ACCOUNT_DEPENDENT_SERVICE_REGISTRAR_H_

#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/mojom/dependent_services.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_account {

class MockBraveAccountDependentServiceRegistrar
    : public mojom::DependentServiceRegistrar {
 public:
  MockBraveAccountDependentServiceRegistrar();
  ~MockBraveAccountDependentServiceRegistrar() override;

  MOCK_METHOD(
      void,
      Register,
      (mojom::Service service,
       mojo::PendingReceiver<mojom::DependentServiceRegistration> registration),
      (override));

  mojo::PendingRemote<mojom::DependentServiceRegistrar> BindAndGetRemote();

 private:
  mojo::Receiver<mojom::DependentServiceRegistrar> receiver_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_MOCK_BRAVE_ACCOUNT_DEPENDENT_SERVICE_REGISTRAR_H_
