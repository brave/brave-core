/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"

namespace bat_ledger {

namespace features {
bool UseInProcessLedgerService();
}

void MakeInProcessLedgerService(
    mojo::PendingReceiver<mojom::BatLedgerService> receiver);
}  // namespace bat_ledger
