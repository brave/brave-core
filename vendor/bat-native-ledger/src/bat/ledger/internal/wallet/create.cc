/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/create.h"

#include <utility>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "net/http/http_status_code.h"

#include "anon/anon.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_wallet {

Create::Create(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  initAnonize();
}

Create::~Create() {
}

void Create::Start(ledger::CreateWalletCallback callback) {
  auto on_req = std::bind(&Create::RequestCredentialsCallback,
                            this,
                            _1,
                            _2,
                            _3,
                            std::move(callback));
  ledger_->LoadURL(
      braveledger_bat_helper::buildURL(REGISTER_PERSONA, PREFIX_V2),
      std::vector<std::string>(), "", "",
      ledger::URL_METHOD::GET, on_req);
}

std::string Create::GetAnonizeProof(const std::string& registrarVK,
                                    const std::string& id,
                                    std::string* preFlight) {
  const char* cred = makeCred(id.c_str());
  if (cred != nullptr) {
    *preFlight = cred;
    // should fix in
    // https://github.com/brave-intl/bat-native-anonize/issues/11
    free((void*)cred); // NOLINT
  } else {
    return "";
  }
  const char* proofTemp = registerUserMessage(preFlight->c_str(),
                                              registrarVK.c_str());
  std::string proof;
  if (proofTemp != nullptr) {
    proof = proofTemp;
    // should fix in
    // https://github.com/brave-intl/bat-native-anonize/issues/11
    free((void*)proofTemp); // NOLINT
  } else {
    return "";
  }

  return proof;
}

void Create::RequestCredentialsCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      ledger::CreateWalletCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  std::string persona_id = ledger_->GetPersonaId();

  if (persona_id.empty()) {
    persona_id = ledger_->GenerateGUID();
    ledger_->SetPersonaId(persona_id);
  }
  // Anonize2 limit is 31 octets
  std::string user_id = persona_id;
  user_id.erase(
      std::remove(user_id.begin(), user_id.end(), '-'), user_id.end());
  user_id.erase(12, 1);

  ledger_->SetUserId(user_id);

  std::string registrar_vk;
  if (!braveledger_bat_helper::getJSONValue(REGISTRARVK_FIELDNAME,
                                            response,
                                            &registrar_vk)) {
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }
  DCHECK(!registrar_vk.empty());
  ledger_->SetRegistrarVK(registrar_vk);
  std::string pre_flight;
  std::string proof = GetAnonizeProof(registrar_vk, user_id, &pre_flight);
  ledger_->SetPreFlight(pre_flight);

  if (proof.empty()) {
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  braveledger_bat_helper::WALLET_INFO_ST wallet_info;
  std::vector<uint8_t> key_info_seed = braveledger_bat_helper::generateSeed();

  wallet_info.keyInfoSeed_ = key_info_seed;
  ledger_->SetWalletInfo(wallet_info);
  std::vector<uint8_t> secretKey =
      braveledger_bat_helper::getHKDF(key_info_seed);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  braveledger_bat_helper::getPublicKeyFromSeed(secretKey,
                                               &publicKey,
                                               &newSecretKey);
  std::string label = ledger_->GenerateGUID();
  std::string publicKeyHex = braveledger_bat_helper::uint8ToHex(publicKey);
  std::string keys[3] = {"currency", "label", "publicKey"};
  std::string values[3] = {LEDGER_CURRENCY, label, publicKeyHex};
  std::string octets = braveledger_bat_helper::stringify(keys, values, 3);
  std::string headerDigest = "SHA-256=" +
      braveledger_bat_helper::getBase64(
          braveledger_bat_helper::getSHA256(octets));
  std::string headerKeys[1] = {"digest"};
  std::string headerValues[1] = {headerDigest};
  std::string headerSignature = braveledger_bat_helper::sign(headerKeys,
                                                             headerValues,
                                                             1,
                                                             "primary",
                                                             newSecretKey);

  braveledger_bat_helper::REQUEST_CREDENTIALS_ST requestCredentials;
  requestCredentials.requestType_ = "httpSignature";
  requestCredentials.proof_ = proof;
  requestCredentials.request_body_currency_ = LEDGER_CURRENCY;
  requestCredentials.request_body_label_ = label;
  requestCredentials.request_body_publicKey_ = publicKeyHex;
  requestCredentials.request_headers_digest_ = headerDigest;
  requestCredentials.request_headers_signature_ = headerSignature;
  requestCredentials.request_body_octets_ = octets;
  std::string payloadStringify =
      braveledger_bat_helper::stringifyRequestCredentialsSt(requestCredentials);
  std::vector<std::string> registerHeaders;
  registerHeaders.push_back("Content-Type: application/json; charset=UTF-8");

  // We should use simple callbacks on iOS
  const std::string url = braveledger_bat_helper::buildURL(
      (std::string)REGISTER_PERSONA + "/" + ledger_->GetUserId(), PREFIX_V2);
  auto on_register = std::bind(&Create::RegisterPersonaCallback,
                            this,
                            _1,
                            _2,
                            _3,
                            std::move(callback));
  ledger_->LoadURL(
    url,
    registerHeaders, payloadStringify, "application/json; charset=utf-8",
    ledger::URL_METHOD::POST, on_register);
}

void Create::RegisterPersonaCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      ledger::CreateWalletCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  std::string verification;
  if (!braveledger_bat_helper::getJSONValue(VERIFICATION_FIELDNAME,
                                            response,
                                            &verification)) {
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }
  const char* masterUserToken = registerUserFinal(
      ledger_->GetUserId().c_str(),
      verification.c_str(),
      ledger_->GetPreFlight().c_str(),
      ledger_->GetRegistrarVK().c_str());

  if (masterUserToken != nullptr) {
    ledger_->SetMasterUserToken(masterUserToken);
    // should fix in
    // https://github.com/brave-intl/bat-native-anonize/issues/11
    free((void*)masterUserToken); // NOLINT
  } else if (!braveledger_bat_helper::ignore_for_testing()) {
    callback(
        ledger::Result::REGISTRATION_VERIFICATION_FAILED);
    return;
  }

  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();
  unsigned int days;
  double fee_amount = .0;
  std::string currency;
  if (!braveledger_bat_helper::getJSONWalletInfo(response,
                                                 &wallet_info,
                                                 &currency,
                                                 &fee_amount,
                                                 &days)) {
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  ledger_->SetWalletInfo(wallet_info);
  ledger_->SetCurrency(currency);
  ledger_->SetContributionAmount(fee_amount);
  ledger_->SetDays(days);
  ledger_->SetBootStamp(braveledger_bat_helper::currentTime());
  ledger_->ResetReconcileStamp();
  callback(ledger::Result::WALLET_CREATED);
}

}  // namespace braveledger_wallet
