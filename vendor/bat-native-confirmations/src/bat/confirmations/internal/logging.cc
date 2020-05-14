/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/logging.h"

namespace confirmations {

ConfirmationsClient* g_confirmations_client = nullptr;  // NOT OWNED

void set_confirmations_client_for_logging(
    ConfirmationsClient* confirmations_client) {
  DCHECK(confirmations_client);
  g_confirmations_client = confirmations_client;
}

void Log(
    const char* file,
    const int line,
    const int verbose_level,
    const std::string& message) {
  if (!g_confirmations_client) {
    return;
  }

  g_confirmations_client->Log(file, line, verbose_level, message);
}

}  // namespace confirmations
