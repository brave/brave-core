/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/confirmations.h"

#include "bat/confirmations/internal/confirmations_impl.h"

namespace confirmations {

bool _is_production = false;
bool _is_debug = false;

const char _confirmations_name[] = "confirmations.json";

// static
Confirmations* Confirmations::CreateInstance(
    ConfirmationsClient* confirmations_client) {
  return new ConfirmationsImpl(confirmations_client);
}

}  // namespace confirmations
