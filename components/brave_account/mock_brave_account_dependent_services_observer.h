// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_MOCK_BRAVE_ACCOUNT_DEPENDENT_SERVICES_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_MOCK_BRAVE_ACCOUNT_DEPENDENT_SERVICES_OBSERVER_H_

#include <vector>

#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/mojom/dependent_services.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_account {

class MockBraveAccountDependentServicesObserver
    : public mojom::DependentServicesObserver {
 public:
  MockBraveAccountDependentServicesObserver();
  ~MockBraveAccountDependentServicesObserver() override;

  MOCK_METHOD(void,
              OnDependentServicesChanged,
              (const std::vector<mojom::Service>& services),
              (override));

  mojo::PendingRemote<mojom::DependentServicesObserver> BindAndGetRemote();

  void FlushForTesting();

 private:
  mojo::Receiver<mojom::DependentServicesObserver> receiver_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_MOCK_BRAVE_ACCOUNT_DEPENDENT_SERVICES_OBSERVER_H_
