/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_RANDOMIZER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_RANDOMIZER_H_

#include "bat/ledger/internal/core/bat_ledger_context.h"

namespace ledger {

class Randomizer : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "randomizer";

  virtual double Uniform01();

  virtual uint64_t Geometric(double period);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_RANDOMIZER_H_
