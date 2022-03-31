/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LOGGING_EVENT_LOG_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LOGGING_EVENT_LOG_UTIL_H_

#include <string>

#include "bat/ledger/mojom_structs.h"

namespace ledger {
namespace log {
std::string GetEventLogKeyForLinkingResult(type::Result result);
}  // namespace log
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LOGGING_EVENT_LOG_UTIL_H_
