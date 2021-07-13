/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/endpoint/gemini/gemini_server.h"
#include "bat/ledger/internal/gemini/gemini.h"
#include "bat/ledger/internal/gemini/gemini_transfer.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ledger {
namespace gemini {

GeminiTransfer::GeminiTransfer(LedgerImpl* ledger)
    : ledger_(ledger),
      gemini_server_(std::make_unique<endpoint::GeminiServer>(ledger)) {}

GeminiTransfer::~GeminiTransfer() = default;

void GeminiTransfer::Start(const Transaction& transaction,
                           client::TransactionCallback callback) {
  auto wallet = ledger_->gemini()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  auto url_callback =
      std::bind(&GeminiTransfer::OnCreateTransaction, this, _1, _2, callback);
  gemini_server_->post_transaction()->Request(wallet->token, transaction,
                                              url_callback);
}

void GeminiTransfer::OnCreateTransaction(const type::Result result,
                                         const std::string& id,
                                         client::TransactionCallback callback) {
  if (result == type::Result::EXPIRED_TOKEN) {
    ledger_->gemini()->DisconnectWallet();
    callback(type::Result::EXPIRED_TOKEN, "");
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  callback(type::Result::LEDGER_OK, id);
}

}  // namespace gemini
}  // namespace ledger
