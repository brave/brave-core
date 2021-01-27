/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/bitflyer/bitflyer_transfer.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_server.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ledger {
namespace bitflyer {

BitflyerTransfer::BitflyerTransfer(LedgerImpl* ledger)
    : ledger_(ledger),
      bitflyer_server_(std::make_unique<endpoint::BitflyerServer>(ledger)) {}

BitflyerTransfer::~BitflyerTransfer() = default;

void BitflyerTransfer::Start(const Transaction& transaction,
                             client::TransactionCallback callback) {
  auto wallet = GetWallet(ledger_);
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  auto url_callback =
      std::bind(&BitflyerTransfer::OnCreateTransaction, this, _1, _2, callback);
  bitflyer_server_->post_transaction()->Request(wallet->token, transaction,
                                                false, url_callback);
}

void BitflyerTransfer::OnCreateTransaction(
    const type::Result result,
    const std::string& id,
    client::TransactionCallback callback) {
  if (result == type::Result::EXPIRED_TOKEN) {
    callback(type::Result::EXPIRED_TOKEN, "");
    ledger_->bitflyer()->DisconnectWallet();
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    // TODO(nejczdovc): add retry logic to all errors in this function
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  callback(type::Result::LEDGER_OK, id);
}

}  // namespace bitflyer
}  // namespace ledger
