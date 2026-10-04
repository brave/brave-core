// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_account/mock_brave_account_authentication_observer.h"

namespace brave_account {

MockBraveAccountAuthenticationObserver::
    MockBraveAccountAuthenticationObserver() = default;
MockBraveAccountAuthenticationObserver::
    ~MockBraveAccountAuthenticationObserver() = default;

mojo::PendingRemote<mojom::AuthenticationObserver>
MockBraveAccountAuthenticationObserver::BindAndGetRemote() {
  return receiver_.BindNewPipeAndPassRemote();
}

void MockBraveAccountAuthenticationObserver::FlushForTesting() {
  receiver_.FlushForTesting();
}

}  // namespace brave_account
