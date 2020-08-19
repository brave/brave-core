/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/logging/logging.h"

#include "bat/ledger/ledger_client.h"

namespace ledger {

LedgerClient* g_ledger_client = nullptr;  // NOT OWNED

void set_ledger_client_for_logging(
    LedgerClient* ledger_client) {
  DCHECK(ledger_client);
  g_ledger_client = ledger_client;
}

void Log(
    const char* file,
    const int line,
    const int verbose_level,
    const std::string& message) {
  if (!g_ledger_client) {
    return;
  }

  g_ledger_client->Log(file, line, verbose_level, message);
}

}  // namespace ledger
