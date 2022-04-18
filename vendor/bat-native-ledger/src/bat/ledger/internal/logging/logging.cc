/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/logging/logging.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"
#include "bat/ledger/ledger_client.h"

namespace ledger {

static base::LazyInstance<base::ThreadLocalPointer<LedgerClient>>::Leaky
    g_ledger_client = LAZY_INSTANCE_INITIALIZER;  // NOT OWNED

void set_ledger_client_for_logging(LedgerClient* ledger_client) {
  g_ledger_client.Pointer()->Set(ledger_client);
}

void Log(const char* file,
         const int line,
         const int verbose_level,
         const std::string& message) {
  auto* ledger_client = g_ledger_client.Pointer()->Get();
  if (!ledger_client) {
    return;
  }

  ledger_client->Log(file, line, verbose_level, message);
}

}  // namespace ledger
