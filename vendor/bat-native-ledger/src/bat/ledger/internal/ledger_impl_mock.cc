/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_impl_mock.h"

namespace bat_ledger {

MockLogStreamImpl::MockLogStreamImpl(
    const char* file,
    const int line,
    const ledger::LogLevel log_level) {
  (void)file;
  (void)line;
  (void)log_level;
}

std::ostream& MockLogStreamImpl::stream() {
  return std::cout;
}

MockLedgerImpl::MockLedgerImpl(ledger::LedgerClient* client) :
    LedgerImpl(client)  {
}

MockLedgerImpl::~MockLedgerImpl() = default;

std::unique_ptr<ledger::LogStream> MockLedgerImpl::Log(
    const char* file,
    int line,
    const ledger::LogLevel log_level) const {
  return std::make_unique<MockLogStreamImpl>(file, line, log_level);
}

}  // namespace bat_ledger
