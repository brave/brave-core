/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/attestation/attestation.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::internal {
namespace attestation {

Attestation::Attestation(LedgerImpl& ledger) : ledger_(ledger) {}

Attestation::~Attestation() = default;

}  // namespace attestation
}  // namespace brave_rewards::internal
