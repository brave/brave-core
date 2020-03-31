/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/request/attestation_requests.h"
#include "bat/ledger/internal/request/request_util.h"

namespace braveledger_request_util  {

  std::string GetServerPublicKey() {
    return BuildUrl("/meta", PREFIX_V1, ServerTypes::PRIVATE_CHANNEL_ONE);
  }

  std::string GetStartProtocolUrl() {
    return BuildUrl("/attestation/start", PREFIX_V1, ServerTypes::PRIVATE_CHANNEL_ONE);
  }

  std::string GetResultProtocolUrl() {
    return BuildUrl("/attestation/result", PREFIX_V1, ServerTypes::PRIVATE_CHANNEL_ONE);
  }
}
