/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_RESPONSE_RESPONSE_CREDENTIALS_H_
#define BRAVELEDGER_RESPONSE_RESPONSE_CREDENTIALS_H_

#include "base/values.h"
#include "bat/ledger/mojom_structs.h"

namespace braveledger_response_util {

ledger::Result ParseSignedCreds(
    const ledger::UrlResponse& response,
    base::Value* result);

}  // namespace braveledger_response_util

#endif  // BRAVELEDGER_RESPONSE_RESPONSE_CREDENTIALS_H_
