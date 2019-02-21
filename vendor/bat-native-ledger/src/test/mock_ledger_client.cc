/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>
#include <string>

#include "mock_ledger_client.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {

static uint64_t next_id = 1;

MockLedgerClient::MockLedgerClient() :
    ledger_(ledger::Ledger::CreateInstance(this)) {
}

MockLedgerClient::~MockLedgerClient() {
}

void MockLedgerClient::CreateWallet() {
  ledger_->CreateWallet();
}

std::string MockLedgerClient::GenerateGUID() const {
  return "guid";
}

void MockLedgerClient::Shutdown() {
  ledger_.reset();
}

void MockLedgerClient::OnWalletInitialized(ledger::Result result) {
}

void MockLedgerClient::OnReconcileComplete(ledger::Result result,
                                           const std::string& viewing_id) {
}

void MockLedgerClient::LoadLedgerState(ledger::LedgerCallbackHandler* handler) {
  handler->OnLedgerStateLoaded(ledger::Result::OK, ledger_state_);
}

void MockLedgerClient::LoadPublisherState(
    ledger::LedgerCallbackHandler* handler) {
  handler->OnLedgerStateLoaded(ledger::Result::OK, publisher_state_);
}

void MockLedgerClient::SaveLedgerState(const std::string& ledger_state,
                                       ledger::LedgerCallbackHandler* handler) {
  ledger_state_ = ledger_state;
  handler->OnLedgerStateSaved(ledger::Result::OK);
}

void MockLedgerClient::SavePublisherState(
    const std::string& publisher_state,
    ledger::LedgerCallbackHandler* handler) {
  publisher_state_ = publisher_state;
  handler->OnPublisherStateSaved(ledger::Result::OK);
}

void MockLedgerClient::LoadURL(const std::string& url,
                               const std::vector<std::string>& headers,
                               const std::string& content,
                               const std::string& contentType,
                               const ledger::URL_METHOD method,
                               ledger::LoadURLCallback callback) {
  callback(true, "{}", {});
}

}  // namespace bat_ledger
