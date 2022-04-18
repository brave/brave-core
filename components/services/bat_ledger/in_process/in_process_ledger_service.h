/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_IN_PROCESS_IN_PROCESS_LEDGER_SERVICE_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_IN_PROCESS_IN_PROCESS_LEDGER_SERVICE_H_
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"

namespace bat_ledger {

namespace features {
bool UseInProcessLedgerService();
}

void MakeInProcessLedgerService(
    mojo::PendingReceiver<mojom::BatLedgerService> receiver);
}  // namespace bat_ledger

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_IN_PROCESS_IN_PROCESS_LEDGER_SERVICE_H_
