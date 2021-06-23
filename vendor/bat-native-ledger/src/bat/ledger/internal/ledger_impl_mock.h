/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_IMPL_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_IMPL_MOCK_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger_client.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ledger {

class MockLedgerImpl : public LedgerImpl {
 public:
  explicit MockLedgerImpl(ledger::LedgerClient* client);

  ~MockLedgerImpl() override;

  MOCK_CONST_METHOD0(database, database::Database*());

  MOCK_METHOD2(Initialize, void(bool, ledger::ResultCallback));
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_IMPL_MOCK_H_
