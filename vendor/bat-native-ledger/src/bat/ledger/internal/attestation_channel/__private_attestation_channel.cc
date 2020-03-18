/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/attestation_channel/private_attestation_channel.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace braveledger_attestation_channel {

PrivateAttestationChannel::PrivateAttestationChannel(bat_ledger::LedgerImpl* ledger) : 
    ledger_(ledger),
    attestation_timer_id_(0u) {
}

PrivateAttestationChannel::~PrivateAttestationChannel() = default;

}  // namespace braveledger_attestation_channel
