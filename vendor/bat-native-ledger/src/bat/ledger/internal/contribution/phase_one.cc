/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include "anon/anon.h"
#include "bat/ledger/internal/contribution/phase_one.h"
#include "bat/ledger/internal/contribution/phase_two.h"
#include "bat/ledger/internal/ledger_impl.h"
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
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_RECONCILE);
  std::string url = braveledger_bat_helper::buildURL(
      (std::string)RECONCILE_CONTRIBUTION + ledger_->GetUserId(), PREFIX_V2);

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
      ledger::URL_METHOD::GET,
      callback);
}

void PhaseOne::ReconcileCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  auto reconcile = ledger_->GetReconcileById(viewing_id);

  if (response_status_code != net::HTTP_OK || reconcile.viewingId_.empty()) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_RECONCILE,
                            viewing_id);
    return;
  }

  bool success = braveledger_bat_helper::getJSONValue(
      SURVEYOR_ID,
      response,
      &reconcile.surveyorInfo_.surveyorId_);
  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_RECONCILE,
                            viewing_id);
    return;
  }

  success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    Complete(ledger::Result::LEDGER_ERROR,
             viewing_id,
             reconcile.type_);
    return;
  }

  CurrentReconcile(viewing_id);
}

void PhaseOne::CurrentReconcile(const std::string& viewing_id) {
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_CURRENT);
  std::ostringstream amount;
  auto reconcile = ledger_->GetReconcileById(viewing_id);

  amount << reconcile.fee_;

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
      braveledger_bat_helper::buildURL(path, PREFIX_V2),
      std::vector<std::string>(),
      "",
      "",
      ledger::URL_METHOD::GET,
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
                                                      &reconcile.rates_);
  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_CURRENT,
                            viewing_id);
    return;
  }

  braveledger_bat_helper::UNSIGNED_TX unsigned_tx;
  success = braveledger_bat_helper::getJSONUnsignedTx(response, &unsigned_tx);
  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_CURRENT,
                            viewing_id);
    return;
  }

  if (unsigned_tx.amount_.empty() &&
      unsigned_tx.currency_.empty() &&
      unsigned_tx.destination_.empty()) {
    // We don't have any unsigned transactions
    contribution_->AddRetry(ledger::ContributionRetry::STEP_CURRENT,
                            viewing_id);
    return;
  }

  reconcile.amount_ = unsigned_tx.amount_;
  reconcile.currency_ = unsigned_tx.currency_;
  reconcile.destination_ = unsigned_tx.destination_;

  if (ledger::is_testing) {
    std::ostringstream amount;
    amount << reconcile.fee_;
    reconcile.amount_ = amount.str();
  }

  success = ledger_->UpdateReconcile(reconcile);

  if (!success) {
    Complete(ledger::Result::LEDGER_ERROR,
             viewing_id,
             reconcile.type_);
    return;
  }

  ReconcilePayload(viewing_id);
}

void PhaseOne::ReconcilePayload(const std::string& viewing_id) {
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_PAYLOAD);
  auto reconcile = ledger_->GetReconcileById(viewing_id);
  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();

  braveledger_bat_helper::UNSIGNED_TX unsigned_tx;
  unsigned_tx.amount_ = reconcile.amount_;
  unsigned_tx.currency_ = reconcile.currency_;
  unsigned_tx.destination_ = reconcile.destination_;
  std::string octets = braveledger_bat_helper::stringifyUnsignedTx(unsigned_tx);

  std::string header_digest = "SHA-256=" +
      braveledger_bat_helper::getBase64(
          braveledger_bat_helper::getSHA256(octets));

  std::string header_keys[1] = {"digest"};
  std::string header_values[1] = {header_digest};

  std::vector<uint8_t> secret_key = braveledger_bat_helper::getHKDF(
      wallet_info.keyInfoSeed_);
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

  std::string headerSignature = braveledger_bat_helper::sign(header_keys,
                                                             header_values,
                                                             1,
                                                             "primary",
                                                             new_secret_key);

  braveledger_bat_helper::RECONCILE_PAYLOAD_ST reconcile_payload;
  reconcile_payload.requestType_ = "httpSignature";
  reconcile_payload.request_signedtx_headers_digest_ = header_digest;
  reconcile_payload.request_signedtx_headers_signature_ = headerSignature;
  reconcile_payload.request_signedtx_body_ = unsigned_tx;
  reconcile_payload.request_signedtx_octets_ = octets;
  reconcile_payload.request_viewingId_ = reconcile.viewingId_;
  reconcile_payload.request_surveyorId_ = reconcile.surveyorInfo_.surveyorId_;
  std::string payload_stringify =
      braveledger_bat_helper::stringifyReconcilePayloadSt(reconcile_payload);

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
      braveledger_bat_helper::buildURL(path, PREFIX_V2),
      wallet_header,
      payload_stringify,
      "application/json; charset=utf-8",
      ledger::URL_METHOD::PUT,
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
      Complete(ledger::Result::CONTRIBUTION_AMOUNT_TOO_LOW,
               viewing_id,
               reconcile.type_);
    } else {
      contribution_->AddRetry(ledger::ContributionRetry::STEP_PAYLOAD,
                              viewing_id);
    }
    return;
  }

  braveledger_bat_helper::TRANSACTION_ST transaction;
  bool success = braveledger_bat_helper::getJSONTransaction(response,
                                                            &transaction);
  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_PAYLOAD,
                            viewing_id);
    return;
  }

  transaction.viewingId_ = reconcile.viewingId_;
  transaction.surveyorId_ = reconcile.surveyorInfo_.surveyorId_;
  transaction.contribution_rates_ = reconcile.rates_;

  if (ledger::is_testing) {
    transaction.contribution_probi_ = reconcile.amount_ + "000000000000000000";
  }

  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
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
      braveledger_bat_helper::buildURL(
        (std::string)REGISTER_VIEWING, PREFIX_V2),
      std::vector<std::string>(),
      "",
      "",
      ledger::URL_METHOD::GET,
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
                                                      &reconcile.registrarVK_);
  DCHECK(!reconcile.registrarVK_.empty());
  if (!success || reconcile.registrarVK_.empty()) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_REGISTER,
                            viewing_id);
    return;
  }

  reconcile.anonizeViewingId_ = reconcile.viewingId_;
  reconcile.anonizeViewingId_.erase(
      std::remove(reconcile.anonizeViewingId_.begin(),
                  reconcile.anonizeViewingId_.end(),
                  '-'),
      reconcile.anonizeViewingId_.end());
  reconcile.anonizeViewingId_.erase(12, 1);
  reconcile.proof_ = GetAnonizeProof(reconcile.registrarVK_,
                                     reconcile.anonizeViewingId_,
                                     &reconcile.preFlight_);

  success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    Complete(ledger::Result::LEDGER_ERROR,
             viewing_id,
             reconcile.type_);
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
  std::string values[1] = {reconcile.proof_};
  std::string proof_stringified = braveledger_bat_helper::stringify(keys,
                                                                    values,
                                                                    1);

  std::string url = braveledger_bat_helper::buildURL(
      (std::string)REGISTER_VIEWING +
      "/" +
      reconcile.anonizeViewingId_,
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
      ledger::URL_METHOD::POST,
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
      reconcile.anonizeViewingId_.c_str(),
      verification.c_str(),
      reconcile.preFlight_.c_str(),
      reconcile.registrarVK_.c_str());

  if (master_user_token != nullptr) {
    reconcile.masterUserToken_ = master_user_token;
    // should fix in
    // https://github.com/brave-intl/bat-native-anonize/issues/11
    free((void*)master_user_token); // NOLINT
  }

  success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    Complete(ledger::Result::LEDGER_ERROR,
             viewing_id,
             reconcile.type_);
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
  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();

  for (size_t i = 0; i < transactions.size(); i++) {
    if (transactions[i].viewingId_ != reconcile.viewingId_) {
      continue;
    }

    transactions[i].anonizeViewingId_ = reconcile.anonizeViewingId_;
    transactions[i].registrarVK_ = reconcile.registrarVK_;
    transactions[i].masterUserToken_ = reconcile.masterUserToken_;
    transactions[i].surveyorIds_ = surveyors;
    probi = transactions[i].contribution_probi_;
  }

  ledger_->SetTransactions(transactions);
  Complete(ledger::Result::LEDGER_OK,
           reconcile.viewingId_,
           reconcile.type_,
           probi);
}

void PhaseOne::Complete(ledger::Result result,
                        const std::string& viewing_id,
                        const ledger::RewardsType type,
                        const std::string& probi) {
  // Set timer to the next month when AC is complete
  if (type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    contribution_->ResetReconcileStamp();
  }

  ledger_->OnReconcileComplete(result, viewing_id, probi, type);

  if (result != ledger::Result::LEDGER_OK) {
    if (!viewing_id.empty()) {
      ledger_->RemoveReconcileById(viewing_id);
    }
    return;
  }

  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_WINNERS);

  contribution_->StartPhaseTwo(viewing_id);
}

}  // namespace braveledger_contribution
