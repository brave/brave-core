/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer_transfer.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_server.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace bitflyer {

BitflyerTransfer::BitflyerTransfer(LedgerImpl* ledger)
    : ledger_(ledger),
      bitflyer_server_(std::make_unique<endpoint::BitflyerServer>(ledger)) {}

BitflyerTransfer::~BitflyerTransfer() = default;

void BitflyerTransfer::Start(const Transaction& transaction,
                             client::TransactionCallback callback) {
  auto wallet =
      ledger_->bitflyer()->GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return callback(mojom::Result::LEDGER_ERROR, "");
  }

  auto url_callback =
      std::bind(&BitflyerTransfer::OnCreateTransaction, this, _1, _2, callback);
  bitflyer_server_->post_transaction()->Request(wallet->token, transaction,
                                                false, url_callback);
}

void BitflyerTransfer::OnCreateTransaction(
    mojom::Result result,
    const std::string& id,
    client::TransactionCallback callback) {
  if (!ledger_->bitflyer()->GetWalletIf({mojom::WalletStatus::kConnected})) {
    return callback(mojom::Result::LEDGER_ERROR, "");
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    if (!ledger_->bitflyer()->LogOutWallet()) {
      BLOG(0,
           "Failed to disconnect " << constant::kWalletBitflyer << " wallet!");
      return callback(mojom::Result::LEDGER_ERROR, "");
    }

    return callback(mojom::Result::EXPIRED_TOKEN, "");
  }

  if (result != mojom::Result::LEDGER_OK) {
    callback(mojom::Result::LEDGER_ERROR, "");
    return;
  }

  callback(mojom::Result::LEDGER_OK, id);
}

}  // namespace bitflyer
}  // namespace ledger
