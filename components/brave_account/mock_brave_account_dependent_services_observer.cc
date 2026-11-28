// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_account/mock_brave_account_dependent_services_observer.h"

namespace brave_account {

MockBraveAccountDependentServicesObserver::
    MockBraveAccountDependentServicesObserver() = default;
MockBraveAccountDependentServicesObserver::
    ~MockBraveAccountDependentServicesObserver() = default;

mojo::PendingRemote<mojom::DependentServicesObserver>
MockBraveAccountDependentServicesObserver::BindAndGetRemote() {
  return receiver_.BindNewPipeAndPassRemote();
}

void MockBraveAccountDependentServicesObserver::FlushForTesting() {
  receiver_.FlushForTesting();
}

}  // namespace brave_account
