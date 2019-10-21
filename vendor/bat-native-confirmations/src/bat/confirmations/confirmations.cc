/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/confirmations.h"

#include "bat/confirmations/internal/confirmations_impl.h"

namespace confirmations {

ledger::Environment _environment = ledger::Environment::STAGING;
bool _is_debug = false;

const char _confirmations_resource_name[] = "confirmations.json";

// static
Confirmations* Confirmations::CreateInstance(
    ConfirmationsClient* confirmations_client) {
  return new ConfirmationsImpl(confirmations_client);
}

}  // namespace confirmations
