/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/response/response_uphold.h"
#include "bat/ledger/internal/uphold/uphold_transfer.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_uphold {

UpholdTransfer::UpholdTransfer(bat_ledger::LedgerImpl* ledger, Uphold* uphold) :
    ledger_(ledger),
    uphold_(uphold) {
}

UpholdTransfer::~UpholdTransfer() = default;

void UpholdTransfer::Start(
    const Transaction& transaction,
    ledger::TransactionCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet = GetWallet(std::move(wallets));
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto headers = RequestAuthorization(wallet->token);

  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/transactions",
      wallet->address.c_str());

  const std::string payload = base::StringPrintf(
      R"({
        "denomination": { "amount": %f, "currency": "BAT" },
        "destination": "%s",
        "message": "%s"
      })",
      transaction.amount,
      transaction.address.c_str(),
      transaction.message.c_str());

  auto create_callback = std::bind(&UpholdTransfer::OnCreateTransaction,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetAPIUrl(path),
      headers,
      payload,
      "application/json",
      ledger::UrlMethod::POST,
      create_callback);
}

void UpholdTransfer::OnCreateTransaction(
    const ledger::UrlResponse& response,
    ledger::TransactionCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  std::string id;
  const ledger::Result result =
      braveledger_response_util::ParseUpholdCreateTransaction(response, &id);

  if (result == ledger::Result::EXPIRED_TOKEN) {
    callback(ledger::Result::EXPIRED_TOKEN, "");
    uphold_->DisconnectWallet();
    return;
  }

  if (result != ledger::Result::LEDGER_OK) {
    // TODO(nejczdovc): add retry logic to all errors in this function
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  CommitTransaction(id, callback);
}

void UpholdTransfer::CommitTransaction(
    const std::string& transaction_id,
    ledger::TransactionCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet = GetWallet(std::move(wallets));
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  if (transaction_id.empty()) {
    BLOG(0, "Transaction id not found");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }
  auto headers = RequestAuthorization(wallet->token);

  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/transactions/%s/commit",
      wallet->address.c_str(),
      transaction_id.c_str());

  auto commit_callback = std::bind(&UpholdTransfer::OnCommitTransaction,
      this,
      _1,
      transaction_id,
      callback);
  ledger_->LoadURL(
      GetAPIUrl(path),
      headers,
      "",
      "application/json",
      ledger::UrlMethod::POST,
      commit_callback);
}

void UpholdTransfer::OnCommitTransaction(
    const ledger::UrlResponse& response,
    const std::string& transaction_id,
    ledger::TransactionCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  const ledger::Result result =
      braveledger_response_util::CheckUpholdCommitTransaction(response);

  if (result == ledger::Result::EXPIRED_TOKEN) {
    callback(ledger::Result::EXPIRED_TOKEN, "");
    uphold_->DisconnectWallet();
    return;
  }

  if (result != ledger::Result::LEDGER_OK) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, transaction_id);
}

}  // namespace braveledger_uphold
