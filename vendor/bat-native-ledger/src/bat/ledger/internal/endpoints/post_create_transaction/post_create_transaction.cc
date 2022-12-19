/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoints/post_create_transaction/post_create_transaction.h"

#include <utility>

namespace ledger::endpoints {
PostCreateTransaction::PostCreateTransaction(LedgerImpl* ledger,
                                             std::string&& token,
                                             std::string&& address,
                                             std::string&& destination,
                                             double amount)
    : RequestBuilder(ledger),
      token_(std::move(token)),
      address_(std::move(address)),
      destination_(std::move(destination)),
      amount_(amount) {}

PostCreateTransaction::~PostCreateTransaction() = default;

std::string PostCreateTransaction::ContentType() const {
  return kApplicationJson;
}

}  // namespace ledger::endpoints
