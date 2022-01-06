/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_USER_PREFS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_USER_PREFS_H_

#include "base/time/time.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"

namespace ledger {

class UserPrefs : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "user-prefs";

  bool ac_enabled();
  int ac_minimum_visits();
  base::TimeDelta ac_minimum_duration();
  double ac_amount();
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_USER_PREFS_H_
