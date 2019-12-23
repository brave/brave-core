/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include "anon/anon.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/contribution/phase_one.h"
#include "bat/ledger/internal/contribution/phase_two.h"
#include "bat/ledger/internal/bat_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/properties/reconcile_request_properties.h"
#include "bat/ledger/internal/properties/transaction_properties.h"
#include "bat/ledger/internal/properties/unsigned_tx_properties.h"
#include "bat/ledger/internal/properties/wallet_info_properties.h"
#include "bat/ledger/internal/state/reconcile_request_state.h"
#include "bat/ledger/internal/state/transaction_state.h"
#include "bat/ledger/internal/state/unsigned_tx_state.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_contribution {

PhaseOne::PhaseOne(bat_ledger::LedgerImpl* ledger,
    Contribution* contribution) :
    ledger_(ledger),
    contribution_(contribution) {
  initAnonize();
}

PhaseOne::~PhaseOne() {
}

void PhaseOne::Start(const std::string& viewing_id) {
  const std::string user_id = ledger_->GetUserId();
  if (user_id.empty()) {
    auto reconcile = ledger_->GetReconcileById(viewing_id);
    Complete(ledger::Result::LEDGER_ERROR, viewing_id, reconcile.type);
    return;
  }

  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_RECONCILE);
  std::string url = braveledger_request_util::BuildUrl(
      (std::string)RECONCILE_CONTRIBUTION + user_id, PREFIX_V2);

  auto callback = std::bind(&PhaseOne::ReconcileCallback,
                            this,
                            viewing_id,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(url,
      std::vector<std::string>(),
      "",
      "",
      ledger::UrlMethod::GET,
      callback);
}

void PhaseOne::ReconcileCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  auto reconcile = ledger_->GetReconcileById(viewing_id);

  if (response_status_code != net::HTTP_OK || reconcile.viewing_id.empty()) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_RECONCILE,
                            viewing_id);
    return;
  }

  bool success = braveledger_bat_helper::getJSONValue(
      SURVEYOR_ID,
      response,
      &reconcile.surveyor_id);
  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_RECONCILE,
                            viewing_id);
    return;
  }

  success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    Complete(ledger::Result::LEDGER_ERROR, viewing_id, reconcile.type);
    return;
  }

  CurrentReconcile(viewing_id);
}

void PhaseOne::CurrentReconcile(const std::string& viewing_id) {
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_CURRENT);
  std::ostringstream amount;
  auto reconcile = ledger_->GetReconcileById(viewing_id);

  amount << reconcile.fee;

  std::string currency = ledger_->GetCurrency();
  std::string path = (std::string)WALLET_PROPERTIES +
                      ledger_->GetPaymentId() +
                      "?refresh=true" +
                      "&amount=" +
                      amount.str() +
                      "&altcurrency=" +
                      currency;

  auto callback = std::bind(&PhaseOne::CurrentReconcileCallback,
                            this,
                            viewing_id,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(
      braveledger_request_util::BuildUrl(path, PREFIX_V2),
      std::vector<std::string>(),
      "",
      "",
      ledger::UrlMethod::GET,
      callback);
}

void PhaseOne::CurrentReconcileCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_CURRENT,
                            viewing_id);
    return;
  }

  auto reconcile = ledger_->GetReconcileById(viewing_id);

  bool success = braveledger_bat_helper::getJSONRates(response,
                                                      &reconcile.rates);
  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_CURRENT,
                            viewing_id);
    return;
  }

  ledger::UnsignedTxProperties unsigned_tx;
  const ledger::UnsignedTxState unsigned_tx_state;
  success = unsigned_tx_state.FromJsonResponse(response, &unsigned_tx);

  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_CURRENT,
                            viewing_id);
    return;
  }

  if (unsigned_tx.amount.empty() &&
      unsigned_tx.currency.empty() &&
      unsigned_tx.destination.empty()) {
    // We don't have any unsigned transactions
    contribution_->AddRetry(ledger::ContributionRetry::STEP_CURRENT,
                            viewing_id);
    return;
  }

  reconcile.amount = unsigned_tx.amount;
  reconcile.currency = unsigned_tx.currency;
  reconcile.destination = unsigned_tx.destination;

  if (ledger::is_testing) {
    std::ostringstream amount;
    amount << reconcile.fee;
    reconcile.amount = amount.str();
  }

  success = ledger_->UpdateReconcile(reconcile);

  if (!success) {
    Complete(ledger::Result::LEDGER_ERROR, viewing_id, reconcile.type);
    return;
  }

  ReconcilePayload(viewing_id);
}

void PhaseOne::ReconcilePayload(const std::string& viewing_id) {
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_PAYLOAD);
  auto reconcile = ledger_->GetReconcileById(viewing_id);
  ledger::WalletInfoProperties wallet_info = ledger_->GetWalletInfo();

  ledger::UnsignedTxProperties unsigned_tx;
  unsigned_tx.amount = reconcile.amount;
  unsigned_tx.currency = reconcile.currency;
  unsigned_tx.destination = reconcile.destination;
  const ledger::UnsignedTxState unsigned_tx_state;
  std::string octets = unsigned_tx_state.ToJson(unsigned_tx);

  std::string header_digest = "SHA-256=" +
      braveledger_bat_helper::getBase64(
          braveledger_bat_helper::getSHA256(octets));

  std::vector<std::string> header_keys;
  header_keys.push_back("digest");
  std::vector<std::string> header_values;
  header_values.push_back(header_digest);

  std::vector<uint8_t> secret_key = braveledger_bat_helper::getHKDF(
      wallet_info.key_info_seed);
  std::vector<uint8_t> public_key;
  std::vector<uint8_t> new_secret_key;
  bool success = braveledger_bat_helper::getPublicKeyFromSeed(
      secret_key,
      &public_key,
      &new_secret_key);
  if (!success) {
    // TODO(nejczdovc) what should we do in this case?
    return;
  }

  std::string headerSignature = braveledger_bat_helper::sign(
      header_keys,
      header_values,
      "primary",
      new_secret_key);

  ledger::ReconcileRequestProperties reconcile_request;
  reconcile_request.type = "httpSignature";
  reconcile_request.signed_tx_headers_digest = header_digest;
  reconcile_request.signed_tx_headers_signature = headerSignature;
  reconcile_request.signed_tx_body = unsigned_tx;
  reconcile_request.signed_tx_octets = octets;
  reconcile_request.viewing_id = reconcile.viewing_id;
  reconcile_request.surveyor_id = reconcile.surveyor_id;
  const ledger::ReconcileRequestState reconcile_request_state;
  std::string payload_stringify =
      reconcile_request_state.ToJson(reconcile_request);

  std::vector<std::string> wallet_header;
  wallet_header.push_back("Content-Type: application/json; charset=UTF-8");
  std::string path = (std::string)WALLET_PROPERTIES + ledger_->GetPaymentId();

  auto callback = std::bind(&PhaseOne::ReconcilePayloadCallback,
                            this,
                            viewing_id,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(
      braveledger_request_util::BuildUrl(path, PREFIX_V2),
      wallet_header,
      payload_stringify,
      "application/json; charset=utf-8",
      ledger::UrlMethod::PUT,
      callback);
}

void PhaseOne::ReconcilePayloadCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  const auto reconcile = ledger_->GetReconcileById(viewing_id);

  if (response_status_code != net::HTTP_OK) {
    if (response_status_code == net::HTTP_REQUESTED_RANGE_NOT_SATISFIABLE) {
      Complete(
          ledger::Result::CONTRIBUTION_AMOUNT_TOO_LOW,
          viewing_id,
          reconcile.type);
    } else {
      contribution_->AddRetry(ledger::ContributionRetry::STEP_PAYLOAD,
                              viewing_id);
    }
    return;
  }

  ledger::TransactionProperties transaction;
  const ledger::TransactionState transaction_state;
  bool success = transaction_state.FromJsonResponse(response, &transaction);
  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_PAYLOAD,
                            viewing_id);
    return;
  }

  transaction.viewing_id = reconcile.viewing_id;
  transaction.surveyor_id = reconcile.surveyor_id;
  transaction.contribution_rates = reconcile.rates;

  if (ledger::is_testing) {
    transaction.contribution_probi =
        braveledger_bat_util::ConvertToProbi(reconcile.amount);
  }

  ledger::Transactions transactions = ledger_->GetTransactions();
  transactions.push_back(transaction);
  ledger_->SetTransactions(transactions);
  RegisterViewing(viewing_id);
}

void PhaseOne::RegisterViewing(const std::string& viewing_id) {
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_REGISTER);
  auto callback = std::bind(&PhaseOne::RegisterViewingCallback,
                            this,
                            viewing_id,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(
      braveledger_request_util::BuildUrl(
        (std::string)REGISTER_VIEWING, PREFIX_V2),
      std::vector<std::string>(),
      "",
      "",
      ledger::UrlMethod::GET,
      callback);
}

void PhaseOne::RegisterViewingCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_REGISTER,
                            viewing_id);
    return;
  }

  auto reconcile = ledger_->GetReconcileById(viewing_id);

  bool success = braveledger_bat_helper::getJSONValue(REGISTRARVK_FIELDNAME,
                                                      response,
                                                      &reconcile.registrar_vk);
  DCHECK(!reconcile.registrar_vk.empty());
  if (!success || reconcile.registrar_vk.empty()) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_REGISTER,
                            viewing_id);
    return;
  }

  reconcile.anonize_viewing_id = reconcile.viewing_id;
  reconcile.anonize_viewing_id.erase(
      std::remove(reconcile.anonize_viewing_id.begin(),
                  reconcile.anonize_viewing_id.end(),
                  '-'),
      reconcile.anonize_viewing_id.end());
  reconcile.anonize_viewing_id.erase(12, 1);
  reconcile.proof = GetAnonizeProof(reconcile.registrar_vk,
                                     reconcile.anonize_viewing_id,
                                     &reconcile.pre_flight);

  success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    Complete(ledger::Result::LEDGER_ERROR, viewing_id, reconcile.type);
    return;
  }

  ViewingCredentials(viewing_id);
}

std::string PhaseOne::GetAnonizeProof(
    const std::string& registrar_VK,
    const std::string& id,
    std::string* pre_flight) {
  const char* cred = makeCred(id.c_str());
  if (cred != nullptr) {
    *pre_flight = std::string(cred);
      // should fix in
      // https://github.com/brave-intl/bat-native-anonize/issues/11
      free((void*)cred); // NOLINT
  } else {
    return "";
  }
  const char* proof_temp = registerUserMessage(pre_flight->c_str(),
                                              registrar_VK.c_str());
  std::string proof;
  if (proof_temp != nullptr) {
    proof = proof_temp;
    // should fix in
    // https://github.com/brave-intl/bat-native-anonize/issues/11
    free((void*)proof_temp); // NOLINT
  } else {
    return "";
  }

  return proof;
}

void PhaseOne::ViewingCredentials(const std::string& viewing_id) {
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_VIEWING);
  auto reconcile = ledger_->GetReconcileById(viewing_id);

  std::string keys[1] = {"proof"};
  std::string values[1] = {reconcile.proof};
  std::string proof_stringified = braveledger_bat_helper::stringify(keys,
                                                                    values,
                                                                    1);

  std::string url = braveledger_request_util::BuildUrl(
      (std::string)REGISTER_VIEWING +
      "/" +
      reconcile.anonize_viewing_id,
      PREFIX_V2);

  auto callback = std::bind(&PhaseOne::ViewingCredentialsCallback,
                            this,
                            viewing_id,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(url,
      std::vector<std::string>(),
      proof_stringified,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      callback);
}

void PhaseOne::ViewingCredentialsCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_VIEWING,
                            viewing_id);
    return;
  }

  auto reconcile = ledger_->GetReconcileById(viewing_id);

  std::string verification;
  bool success = braveledger_bat_helper::getJSONValue(VERIFICATION_FIELDNAME,
                                                      response,
                                                      &verification);
  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_VIEWING,
                            viewing_id);
    return;
  }

  const char* master_user_token = registerUserFinal(
      reconcile.anonize_viewing_id.c_str(),
      verification.c_str(),
      reconcile.pre_flight.c_str(),
      reconcile.registrar_vk.c_str());

  if (master_user_token != nullptr) {
    reconcile.master_user_token = master_user_token;
    // should fix in
    // https://github.com/brave-intl/bat-native-anonize/issues/11
    free((void*)master_user_token); // NOLINT
  }

  success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    Complete(ledger::Result::LEDGER_ERROR, viewing_id, reconcile.type);
    return;
  }

  std::vector<std::string> surveyors;
  success = braveledger_bat_helper::getJSONList(SURVEYOR_IDS,
                                                response,
                                                &surveyors);
  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_VIEWING,
                            viewing_id);
    return;
  }

  std::string probi = "0";
  // Save the rest values to transactions
  ledger::Transactions transactions = ledger_->GetTransactions();

  for (size_t i = 0; i < transactions.size(); i++) {
    if (transactions[i].viewing_id != reconcile.viewing_id) {
      continue;
    }

    transactions[i].anonize_viewing_id = reconcile.anonize_viewing_id;
    transactions[i].registrar_vk = reconcile.registrar_vk;
    transactions[i].master_user_token = reconcile.master_user_token;
    transactions[i].surveyor_ids = surveyors;
    probi = transactions[i].contribution_probi;
  }

  ledger_->SetTransactions(transactions);
  Complete(ledger::Result::LEDGER_OK,
           reconcile.viewing_id,
           reconcile.type,
           probi);
}

void PhaseOne::Complete(
    ledger::Result result,
    const std::string& viewing_id,
    const ledger::RewardsType type,
    const std::string& probi) {
  const bool error = result != ledger::Result::LEDGER_OK;
  const double amount = braveledger_bat_util::ProbiToDouble(probi);
  ledger_->ReconcileComplete(result, amount, viewing_id, type, error);

  if (error) {
    return;
  }

  ledger_->AddReconcileStep(
      viewing_id,
      ledger::ContributionRetry::STEP_WINNERS);

  contribution_->StartPhaseTwo(viewing_id);
}

}  // namespace braveledger_contribution
