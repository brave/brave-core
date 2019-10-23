/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/attestation/attestation_iosx.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_attestation {

AttestationIOS::AttestationIOS(bat_ledger::LedgerImpl* ledger) :
    Attestation(ledger) {
}

AttestationIOS::~AttestationIOS() = default;

void AttestationIOS::Start(
    const std::string& payload,
    StartCallback callback) {

}

void AttestationIOS::OnStart(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    StartCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, response);
}

void AttestationIOS::Confirm(
    const std::string& result,
    ConfirmCallback callback) {

}

}  // namespace braveledger_attestation
