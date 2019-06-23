/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/strings/string_split.h"
#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/uphold/uphold_contribution.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;


namespace braveledger_uphold {

UpholdContribution::UpholdContribution(bat_ledger::LedgerImpl* ledger,
                                       Uphold* uphold) :
    ledger_(ledger),
    uphold_(uphold) {
}

UpholdContribution::~UpholdContribution() {
}

void UpholdContribution::Start(const std::string &viewing_id,
                               ledger::ExternalWalletPtr wallet) {
  viewing_id_ = viewing_id;

  if (!wallet) {
    Complete(ledger::Result::LEDGER_ERROR, false);
    return;
  }

  wallet_ = std::move(wallet);
  const auto reconcile = ledger_->GetReconcileById(viewing_id_);

  for (const auto& item : reconcile.directions_) {
    const std::string address =
        ledger_->GetPublisherAddress(item.publisher_key_);
    if (address.empty()) {
      Complete(ledger::Result::LEDGER_ERROR, false);
      return;
    }

    // TODO add 5% fee

    auto callback = std::bind(&UpholdContribution::Complete, this, _1, _2);

    CreateTransaction(static_cast<double>(item.amount_), address, callback);
  }
}

void UpholdContribution::TransferFunds(double amount,
                                       const std::string& address,
                                       ledger::ExternalWalletPtr wallet,
                                       TransactionCallback callback) {
  if (!wallet) {
    callback(ledger::Result::LEDGER_ERROR, false);
    return;
  }

  wallet_ = std::move(wallet);

  CreateTransaction(amount, address, callback);
}

void UpholdContribution::CreateTransaction(double amount,
                                           const std::string& address,
                                           TransactionCallback callback) {
  if (!wallet_) {
    callback(ledger::Result::LEDGER_ERROR, false);
    return;
  }

  auto headers = uphold_->RequestAuthorization(wallet_->token);

  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/transactions",
      wallet_->address.c_str());

  const std::string payload = base::StringPrintf(
      "{ "
      "  \"denomination\": { \"amount\": %f, \"currency\": \"BAT\" }, "
      "  \"destination\": \"%s\" "
      "}",
      amount,
      address.c_str());

  auto create_callbakc = std::bind(&UpholdContribution::OnCreateTransaction,
                            this,
                            _1,
                            _2,
                            _3,
                            callback);
  ledger_->LoadURL(
      uphold_->GetAPIUrl(path),
      headers,
      payload,
      "application/json",
      ledger::URL_METHOD::POST,
      create_callbakc);
}

void UpholdContribution::OnCreateTransaction(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    TransactionCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_ACCEPTED) {
    // TODO(nejczdovc): add retry logic to all errors in this function
    callback(ledger::Result::LEDGER_ERROR, false);
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    callback(ledger::Result::LEDGER_ERROR, false);
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    callback(ledger::Result::LEDGER_ERROR, false);
    return;
  }

  auto* id = dictionary->FindKey("id");
  std::string transaction_id;
  if (!id && !id->is_string()) {
    callback(ledger::Result::LEDGER_ERROR, false);
    return;
  }

  transaction_id = id->GetString();
  CommitTransaction(transaction_id, callback);
}

void UpholdContribution::CommitTransaction(const std::string& transaction_id,
                                           TransactionCallback callback) {
  auto headers = uphold_->RequestAuthorization(wallet_->token);

  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/transactions/%s/commit",
      wallet_->address.c_str(),
      transaction_id.c_str());

  auto commit_callback = std::bind(&UpholdContribution::OnCommitTransaction,
                            this,
                            _1,
                            _2,
                            _3,
                            callback);
  ledger_->LoadURL(
      uphold_->GetAPIUrl(path),
      headers,
      "",
      "application/json",
      ledger::URL_METHOD::POST,
      commit_callback);
}

void UpholdContribution::OnCommitTransaction(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    TransactionCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_OK) {
    // TODO(nejczdovc): add retry logic to all errors in this function
    callback(ledger::Result::LEDGER_ERROR, true);
    return;
  }

  callback(ledger::Result::LEDGER_OK, true);
}

// TODO add test
std::string UpholdContribution::ConvertToProbi(const std::string& amount) {
  auto vec = base::SplitString(
      amount, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  const std::string probi = "000000000000000000";

  if (vec.size() == 1) {
    return vec.at(0) + probi;
  }

  const auto before_dot = vec.at(0);
  const auto after_dot = vec.at(1);
  const auto rest_probi = probi.substr(after_dot.size());

  return before_dot + after_dot + rest_probi;
}

void UpholdContribution::Complete(ledger::Result result, bool created) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id_);
  const auto amount = ConvertToProbi(std::to_string(reconcile.fee_));

  ledger_->OnReconcileComplete(result,
                               viewing_id_,
                               amount,
                               reconcile.category_);

  if (result != ledger::Result::LEDGER_OK) {
    if (!viewing_id_.empty()) {
      ledger_->RemoveReconcileById(viewing_id_);
    }
    return;
  }
}

}  // namespace braveledger_uphold
