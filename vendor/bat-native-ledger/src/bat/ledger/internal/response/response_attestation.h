/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_RESPONSE_RESPONSE_ATTESTATION_H_
#define BRAVELEDGER_RESPONSE_RESPONSE_ATTESTATION_H_

#include <string>

#include "base/values.h"
#include "bat/ledger/mojom_structs.h"

namespace braveledger_response_util {

ledger::Result CheckStartAttestation(const ledger::UrlResponse& response);

ledger::Result ParseCaptcha(
    const ledger::UrlResponse& response,
    base::Value* result);

ledger::Result ParseCaptchaImage(
    const ledger::UrlResponse& response,
    std::string* encoded_image);

ledger::Result CheckConfirmAttestation(
    const ledger::UrlResponse& response);

}  // namespace braveledger_response_util

#endif  // BRAVELEDGER_RESPONSE_RESPONSE_ATTESTATION_H_
