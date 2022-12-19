/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CREATE_TRANSACTION_POST_CREATE_TRANSACTION_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CREATE_TRANSACTION_POST_CREATE_TRANSACTION_H_

#include <string>

#include "bat/ledger/internal/endpoints/request_builder.h"

namespace ledger {
class LedgerImpl;

namespace endpoints {

class PostCreateTransaction : public RequestBuilder {
 public:
  PostCreateTransaction(LedgerImpl*,
                        std::string&& token,
                        std::string&& address,
                        mojom::ExternalTransactionPtr);
  ~PostCreateTransaction() override;

 private:
  std::string ContentType() const override;

 protected:
  inline static const std::string kFeeMessage =
      "5% transaction fee collected by Brave Software International";

  std::string token_;
  std::string address_;
  mojom::ExternalTransactionPtr transaction_;
};

}  // namespace endpoints
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CREATE_TRANSACTION_POST_CREATE_TRANSACTION_H_
