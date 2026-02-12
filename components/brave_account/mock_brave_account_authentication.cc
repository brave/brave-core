// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_account/mock_brave_account_authentication.h"

namespace brave_account {

MockBraveAccountAuthentication::MockBraveAccountAuthentication() = default;
MockBraveAccountAuthentication::~MockBraveAccountAuthentication() = default;

mojo::PendingRemote<mojom::Authentication>
MockBraveAccountAuthentication::BindAndGetRemote() {
  return receiver_.BindNewPipeAndPassRemote();
}

}  // namespace brave_account
