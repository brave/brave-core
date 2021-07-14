/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_GEMINI_SERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_GEMINI_SERVER_H_

#include <memory>

#include "bat/ledger/internal/endpoint/gemini/post_account/post_account_gemini.h"
#include "bat/ledger/internal/endpoint/gemini/post_balance/post_balance_gemini.h"
#include "bat/ledger/internal/endpoint/gemini/post_oauth/post_oauth_gemini.h"
#include "bat/ledger/internal/endpoint/gemini/post_transaction/post_transaction_gemini.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {

class GeminiServer {
 public:
  explicit GeminiServer(LedgerImpl* ledger);
  ~GeminiServer();

  gemini::PostAccount* post_account() const;

  gemini::PostBalance* post_balance() const;

  gemini::PostOauth* post_oauth() const;

  gemini::PostTransaction* post_transaction() const;

 private:
  std::unique_ptr<gemini::PostAccount> post_account_;
  std::unique_ptr<gemini::PostBalance> post_balance_;
  std::unique_ptr<gemini::PostOauth> post_oauth_;
  std::unique_ptr<gemini::PostTransaction> post_transaction_;
};

}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_GEMINI_SERVER_H_
