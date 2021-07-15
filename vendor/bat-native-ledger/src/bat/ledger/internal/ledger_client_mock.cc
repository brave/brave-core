/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_client_mock.h"

namespace ledger {

MockLedgerClient::MockLedgerClient() = default;

MockLedgerClient::~MockLedgerClient() = default;

absl::optional<std::string> MockLedgerClient::EncryptString(
    const std::string& value) {
  return FakeEncryption::EncryptString(value);
}

absl::optional<std::string> MockLedgerClient::DecryptString(
    const std::string& value) {
  return FakeEncryption::DecryptString(value);
}

}  // namespace ledger
