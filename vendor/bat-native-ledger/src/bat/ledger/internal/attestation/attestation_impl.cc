/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/attestation/attestation_androidx.h"
#include "bat/ledger/internal/attestation/attestation_desktop.h"
#include "bat/ledger/internal/attestation/attestation_impl.h"
#include "bat/ledger/internal/attestation/attestation_iosx.h"
#include "net/http/http_status_code.h"

namespace braveledger_attestation {

AttestationImpl::AttestationImpl(bat_ledger::LedgerImpl* ledger) :
    Attestation(ledger) {
  #if defined(OS_IOS)
    instance_ = std::make_unique<AttestationIOS>(ledger);
  #elif defined(OS_ANDROID)
    instance_ = std::make_unique<AttestationAndroid>(ledger);
  #else
    instance_ = std::make_unique<AttestationDesktop>(ledger);
  #endif
}

AttestationImpl::~AttestationImpl() = default;

void AttestationImpl::Start(
    const std::string& payload,
    StartCallback callback) {
  instance_->Start(payload, callback);
}

void AttestationImpl::Confirm(
    const std::string& result,
    ConfirmCallback callback) {
  instance_->Confirm(result, callback);
}

}  // namespace braveledger_attestation
