/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_ENVIRONMENT_CONFIG_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_ENVIRONMENT_CONFIG_H_

#include "bat/ledger/internal/core/bat_ledger_context.h"

namespace ledger {

class EnvironmentConfig : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "environment-config";

  const char* auto_contribute_sku() const;
  const char* uphold_token_order_address() const;
  const char* gemini_token_order_address() const;
  const char* payment_service_host() const;

 private:
  using Environment = BATLedgerContext::Environment;

  Environment env() const { return context().options().environment; }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_ENVIRONMENT_CONFIG_H_
