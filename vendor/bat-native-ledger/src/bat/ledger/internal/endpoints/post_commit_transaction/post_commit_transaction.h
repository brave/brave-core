/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_COMMIT_TRANSACTION_POST_COMMIT_TRANSACTION_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_COMMIT_TRANSACTION_POST_COMMIT_TRANSACTION_H_

#include <string>

#include "bat/ledger/internal/endpoints/request_builder.h"

namespace ledger {
class LedgerImpl;

namespace endpoints {

class PostCommitTransaction : public RequestBuilder {
 public:
  PostCommitTransaction(LedgerImpl*,
                        std::string&& token,
                        std::string&& address,
                        mojom::ExternalTransactionPtr);
  ~PostCommitTransaction() override;

 protected:
  std::string token_;
  std::string address_;
  mojom::ExternalTransactionPtr transaction_;
};

}  // namespace endpoints
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_COMMIT_TRANSACTION_POST_COMMIT_TRANSACTION_H_
