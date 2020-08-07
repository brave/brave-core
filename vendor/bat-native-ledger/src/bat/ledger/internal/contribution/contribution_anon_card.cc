/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "bat/ledger/internal/contribution/contribution_anon_card.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_sku.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/response/response_sku.h"
#include "bat/ledger/internal/sku/sku_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

std::string GetTransactionPayload(
    const double amount,
    const std::string& order_id,
    const std::string& destination,
    const std::string& payment_id,
    const std::vector<uint8_t>& seed) {
  base::Value denomination(base::Value::Type::DICTIONARY);
  denomination.SetStringKey("amount", base::StringPrintf("%g", amount));
  denomination.SetStringKey("currency", "BAT");

  base::Value octets(base::Value::Type::DICTIONARY);
  octets.SetKey("denomination", std::move(denomination));
  octets.SetStringKey("destination", destination);

  std::string octets_json;
  base::JSONWriter::Write(octets, &octets_json);

  const auto sign_headers = braveledger_request_util::GetSignHeaders(
      order_id,
      octets_json,
      "primary",
      seed,
      true);

  base::Value headers(base::Value::Type::DICTIONARY);
  headers.SetStringKey("digest", sign_headers.at("digest"));
  headers.SetStringKey("idempotency-key", order_id);
  headers.SetStringKey("signature", sign_headers.at("signature"));

  base::Value transaction(base::Value::Type::DICTIONARY);
  transaction.SetKey("headers", std::move(headers));
  transaction.SetStringKey("octets", octets_json);

  std::string transaction_json;
  base::JSONWriter::Write(transaction, &transaction_json);
  std::string transaction_base64;
  base::Base64Encode(transaction_json, &transaction_base64);

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", payment_id);
  body.SetStringKey("kind", braveledger_sku::ConvertTransactionTypeToString(
      ledger::SKUTransactionType::ANONYMOUS_CARD));
  body.SetStringKey("transaction", transaction_base64);

  std::string body_json;
  base::JSONWriter::Write(body, &body_json);

  return body_json;
}

}  // namespace

namespace braveledger_contribution {

ContributionAnonCard::ContributionAnonCard(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

ContributionAnonCard::~ContributionAnonCard() = default;

void ContributionAnonCard::SendTransaction(
    const double amount,
    const std::string& order_id,
    const std::string& destination,
    ledger::TransactionCallback callback) {
  const std::string payload = GetTransactionPayload(
      amount,
      order_id,
      destination,
      ledger_->state()->GetPaymentId(),
      ledger_->state()->GetRecoverySeed());

  auto url_callback = std::bind(&ContributionAnonCard::OnSendTransaction,
      this,
      _1,
      callback);

  const std::string url = braveledger_request_util::GetCreateTransactionURL(
      order_id,
      ledger::SKUTransactionType::ANONYMOUS_CARD);

  ledger_->LoadURL(
      url,
      {},
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void ContributionAnonCard::OnSendTransaction(
    const ledger::UrlResponse& response,
    ledger::TransactionCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  const ledger::Result result =
      braveledger_response_util::CheckSendExternalTransaction(response);
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Problem sending transaction");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, "");
}

}  // namespace braveledger_contribution
