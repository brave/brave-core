/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_claim.h"

#include <map>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "bat/ledger/internal/common/security_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_promotion.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/response/response_wallet.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace {

std::string GeneratePayload(
    const std::string& user_funds,
    const std::string& address,
    const std::string& anon_address,
    const std::vector<uint8_t>& seed) {
  const std::string amount = user_funds.empty() ? "0" : user_funds;

  base::Value denomination(base::Value::Type::DICTIONARY);
  denomination.SetStringKey("amount", amount);
  denomination.SetStringKey("currency", "BAT");

  base::Value octets(base::Value::Type::DICTIONARY);
  octets.SetKey("denomination", std::move(denomination));
  octets.SetStringKey("destination", address);
  std::string octets_json;
  base::JSONWriter::Write(octets, &octets_json);

  const std::string header_digest =
      braveledger_helper::Security::DigestValue(octets_json);

  std::vector<std::map<std::string, std::string>> headers;
  headers.push_back({{"digest", header_digest}});

  const std::string header_signature = braveledger_helper::Security::Sign(
      headers,
      "primary",
      seed);

  base::Value signed_reqeust(base::Value::Type::DICTIONARY);
  signed_reqeust.SetStringKey("octets", octets_json);
  signed_reqeust.SetKey("body", std::move(octets));

  base::Value headers_dict(base::Value::Type::DICTIONARY);
  headers_dict.SetStringKey("digest", header_digest);
  headers_dict.SetStringKey("signature", header_signature);
  signed_reqeust.SetKey("headers", std::move(headers_dict));

  std::string signed_request_json;
  base::JSONWriter::Write(signed_reqeust, &signed_request_json);

  std::string signed_request_base64;
  base::Base64Encode(signed_request_json, &signed_request_base64);

  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("signedLinkingRequest", signed_request_base64);
  payload.SetStringKey("anonymousAddress", anon_address);
  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
}

}  // namespace

namespace braveledger_wallet {

WalletClaim::WalletClaim(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    uphold_(std::make_unique<braveledger_uphold::Uphold>(ledger))  {
}

WalletClaim::~WalletClaim() = default;

void WalletClaim::Start(ledger::ResultCallback callback) {
  ledger_->wallet()->FetchBalance(std::bind(&WalletClaim::OnBalance,
      this,
      _1,
      _2,
      callback));
}

void WalletClaim::OnBalance(
    const ledger::Result result,
    ledger::BalancePtr balance,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK || !balance) {
    BLOG(0, "Anon funds transfer failed");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (ledger_->state()->GetAnonTransferChecked() &&
      balance->user_funds == "0") {
    BLOG(1, "Second ping with zero balance");
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet_ptr = braveledger_uphold::GetWallet(std::move(wallets));

  if (!wallet_ptr) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (!wallet_ptr->anon_address.empty()) {
    BLOG(1, "Anon address already exists");
    TransferFunds(
        ledger::Result::LEDGER_OK,
        balance->user_funds,
        callback);
    return;
  }

  auto anon_callback = std::bind(&WalletClaim::TransferFunds,
      this,
      _1,
      balance->user_funds,
      callback);

  uphold_->CreateAnonAddressIfNecessary(anon_callback);
}

void WalletClaim::TransferFunds(
    const ledger::Result result,
    const std::string user_funds,
    ledger::ResultCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet_ptr = braveledger_uphold::GetWallet(std::move(wallets));
  if (!wallet_ptr) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (result != ledger::Result::LEDGER_OK ||
      wallet_ptr->anon_address.empty()) {
    BLOG(0, "Anon address is missing");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto url_callback = std::bind(&WalletClaim::OnTransferFunds,
      this,
      _1,
      callback);

  const std::string url = braveledger_request_util::GetClaimWalletURL(
      ledger_->state()->GetPaymentId());

  const std::string payload = GeneratePayload(
      user_funds,
      wallet_ptr->address,
      wallet_ptr->anon_address,
      ledger_->state()->GetRecoverySeed());

  ledger_->LoadURL(
      url,
      {},
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void WalletClaim::OnTransferFunds(
    const ledger::UrlResponse& response,
    ledger::ResultCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  const auto result = braveledger_response_util::CheckClaimWallet(
      response);

  if (result == ledger::Result::LEDGER_OK) {
    ledger_->state()->SetAnonTransferChecked(true);
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  if (result == ledger::Result::ALREADY_EXISTS) {
    ledger_->state()->SetAnonTransferChecked(true);
    callback(ledger::Result::ALREADY_EXISTS);
    return;
  }

  callback(ledger::Result::LEDGER_ERROR);
}

}  // namespace braveledger_wallet
