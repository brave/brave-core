/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/create.h"

#include <utility>

#include "base/json/json_writer.h"
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

void Create::Start(const std::string& safetynet_token,
      ledger::CreateWalletCallback callback) {
  if (!safetynet_token.empty()) {
    std::string safetynet_prefix = PREFIX_V5;
#if defined (OS_ANDROID) && defined(ARCH_CPU_X86_FAMILY)\
    && defined(OFFICIAL_BUILD)
    safetynet_prefix = PREFIX_V3;
#endif
    std::vector<std::string> headers;
    headers.push_back("safetynet-token:" + safetynet_token);
    auto safetynet_callback = std::bind(&Create::StartSafetyNetCallback,
                                    this,
                                    _1,
                                    _2,
                                    _3,
                                    std::move(callback));
    ledger_->LoadURL(braveledger_bat_helper::buildURL(
          (std::string)GET_SET_PROMOTION, safetynet_prefix),
        headers, "", "", ledger::URL_METHOD::GET, safetynet_callback);
    return;
  }
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

void Create::StartSafetyNetCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ledger::CreateWalletCallback callback) {

  ledger_->LogResponse(__func__, response_status_code, response, headers);

  unsigned int statusCode;
  std::string error;
  bool hasResponseError = braveledger_bat_helper::getJSONResponse(
    response, &statusCode, &error);
  std::string message;
  if (statusCode == SAFETYNET_ERROR_CODE || (hasResponseError &&
      statusCode == net::HTTP_NOT_FOUND &&
      braveledger_bat_helper::getJSONMessage(response, &message) &&
      message == SAFETYNET_ERROR_MESSAGE)) {
    callback(ledger::Result::SAFETYNET_ATTESTATION_FAILED);
    return;
  }
  // We passed safetynet check, so just make regular call
  Start("", callback);
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

std::string Create::StringifyRequestCredentials(
    const std::string& proof,
    const std::string& label,
    const std::string& public_key,
    const std::string& digest,
    const std::string& signature,
    const std::string& octets) {
  base::Value headers(base::Value::Type::DICTIONARY);
  headers.SetStringKey("digest", digest);
  headers.SetStringKey("signature", signature);

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("currency", LEDGER_CURRENCY);
  body.SetStringKey("label", label);
  body.SetStringKey("publicKey", public_key);

  base::Value request(base::Value::Type::DICTIONARY);
  request.SetKey("headers", std::move(headers));
  request.SetKey("body", std::move(body));
  request.SetStringKey("octets", octets);

  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("requestType", "httpSignature");
  payload.SetStringKey("proof", proof);
  payload.SetKey("request", std::move(request));

  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
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
  std::string public_key_hex = braveledger_bat_helper::uint8ToHex(publicKey);
  std::string keys[3] = {"currency", "label", "publicKey"};
  std::string values[3] = {LEDGER_CURRENCY, label, public_key_hex};
  std::string octets = braveledger_bat_helper::stringify(keys, values, 3);
  std::string digest = "SHA-256=" +
      braveledger_bat_helper::getBase64(
          braveledger_bat_helper::getSHA256(octets));
  std::string headerKeys[1] = {"digest"};
  std::string headerValues[1] = {digest};
  std::string signature = braveledger_bat_helper::sign(
      headerKeys,
      headerValues,
      1,
      "primary",
      newSecretKey);

  std::string payload = StringifyRequestCredentials(
      proof,
      label,
      public_key_hex,
      digest,
      signature,
      octets);
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
      registerHeaders,
      payload,
      "application/json; charset=utf-8",
      ledger::URL_METHOD::POST,
      on_register);
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
