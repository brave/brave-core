/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/uphold/uphold_transfer.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_uphold {

UpholdTransfer::UpholdTransfer(bat_ledger::LedgerImpl* ledger, Uphold* uphold) :
    ledger_(ledger),
    uphold_(uphold) {
}

UpholdTransfer::~UpholdTransfer() {
}

void UpholdTransfer::Start(
    const Transaction& transaction,
    ledger::ExternalWalletPtr wallet,
    ledger::TransactionCallback callback) {
  if (!wallet) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  CreateTransaction(transaction, std::move(wallet), callback);
}

void UpholdTransfer::CreateTransaction(
    const Transaction& transaction,
    ledger::ExternalWalletPtr wallet,
    ledger::TransactionCallback callback) {
  auto headers = RequestAuthorization(wallet->token);

  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/transactions",
      wallet->address.c_str());

  const std::string payload = base::StringPrintf(
      "{ "
      "  \"denomination\": { \"amount\": %f, \"currency\": \"BAT\" }, "
      "  \"destination\": \"%s\", "
      "  \"message\": \"%s\" "
      "}",
      transaction.amount,
      transaction.address.c_str(),
      transaction.message.c_str());

  auto create_callback = std::bind(&UpholdTransfer::OnCreateTransaction,
                            this,
                            _1,
                            _2,
                            _3,
                            *wallet,
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
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const ledger::ExternalWallet& wallet,
    ledger::TransactionCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN, "");
    uphold_->DisconnectWallet();
    return;
  }

  if (response_status_code != net::HTTP_ACCEPTED) {
    // TODO(nejczdovc): add retry logic to all errors in this function
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  const auto* id = dictionary->FindStringKey("id");
  if (!id) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  CommitTransaction(*id, wallet, callback);
}

void UpholdTransfer::CommitTransaction(const std::string& transaction_id,
                                       const ledger::ExternalWallet& wallet,
                                       ledger::TransactionCallback callback) {
  auto headers = RequestAuthorization(wallet.token);

  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/transactions/%s/commit",
      wallet.address.c_str(),
      transaction_id.c_str());

  auto commit_callback = std::bind(&UpholdTransfer::OnCommitTransaction,
                            this,
                            _1,
                            _2,
                            _3,
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
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::string& transaction_id,
    ledger::TransactionCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN, "");
    uphold_->DisconnectWallet();
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, transaction_id);
}

}  // namespace braveledger_uphold
