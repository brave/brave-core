/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/create.h"

#include <utility>

#include "base/guid.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/wallet_info_properties.h"
#include "bat/ledger/internal/request/request_util.h"
#include "net/http/http_status_code.h"

#include "anon/anon.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

std::string GetAnonizeProof(
    const std::string& registrarVK,
    const std::string& id,
    std::string* preFlight) {
  DCHECK(preFlight);

  const char* cred = makeCred(id.c_str());
  if (!cred) {
    return "";
  }

  *preFlight = cred;
  // should fix in
  // https://github.com/brave-intl/bat-native-anonize/issues/11
  free((void*)cred); // NOLINT

  const char* proof_temp = registerUserMessage(
      preFlight->c_str(),
      registrarVK.c_str());

  if (!proof_temp) {
    return "";
  }

  std::string proof = proof_temp;
  // should fix in
  // https://github.com/brave-intl/bat-native-anonize/issues/11
  free((void*)proof_temp); // NOLINT

  return proof;
}

std::string StringifyRequestCredentials(
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

}  // namespace

namespace braveledger_wallet {

Create::Create(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  initAnonize();
}

Create::~Create() = default;

void Create::Start(ledger::ResultCallback callback) {
  auto req_callback = std::bind(&Create::RequestCredentialsCallback,
      this,
      _1,
      std::move(callback));

  const std::string url =
      braveledger_request_util::BuildUrl(REGISTER_PERSONA, PREFIX_V2);

  ledger_->LoadURL(url, {}, "", "", ledger::UrlMethod::GET, req_callback);
}

void Create::RequestCredentialsCallback(
      const ledger::UrlResponse& response,
      ledger::ResultCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code != net::HTTP_OK) {
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  // Anonize2 limit is 31 octets
  std::string user_id = base::GenerateGUID();
  user_id.erase(
      std::remove(user_id.begin(), user_id.end(), '-'), user_id.end());
  user_id.erase(12, 1);

  std::string registrar_vk;
  if (!braveledger_bat_helper::getJSONValue(REGISTRARVK_FIELDNAME,
                                            response.body,
                                            &registrar_vk)) {
    BLOG(0, "Bad registration response");
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }
  DCHECK(!registrar_vk.empty());
  std::string pre_flight;
  std::string proof = GetAnonizeProof(registrar_vk, user_id, &pre_flight);

  if (proof.empty()) {
    BLOG(0, "Proof is empty");
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  ledger::WalletInfoProperties wallet_info;
  std::vector<uint8_t> key_info_seed = braveledger_bat_helper::generateSeed();

  wallet_info.key_info_seed = key_info_seed;
  ledger_->SetWalletInfo(wallet_info);
  std::vector<uint8_t> secretKey =
      braveledger_bat_helper::getHKDF(key_info_seed);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  braveledger_bat_helper::getPublicKeyFromSeed(secretKey,
                                               &publicKey,
                                               &newSecretKey);
  std::string label = base::GenerateGUID();
  std::string public_key_hex = braveledger_bat_helper::uint8ToHex(publicKey);
  std::string keys[3] = {"currency", "label", "publicKey"};
  std::string values[3] = {LEDGER_CURRENCY, label, public_key_hex};
  std::string octets = braveledger_bat_helper::stringify(keys, values, 3);
  std::string digest = "SHA-256=" +
      braveledger_bat_helper::getBase64(
          braveledger_bat_helper::getSHA256(octets));

  std::vector<std::string> header_keys;
  header_keys.push_back("digest");
  std::vector<std::string> header_values;
  header_values.push_back(digest);

  std::string signature = braveledger_bat_helper::sign(
      header_keys,
      header_values,
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
  const std::string url = braveledger_request_util::BuildUrl(
      (std::string)REGISTER_PERSONA + "/" + user_id, PREFIX_V2);
  auto on_register = std::bind(&Create::RegisterPersonaCallback,
      this,
      _1,
      user_id,
      pre_flight,
      registrar_vk,
      std::move(callback));

  ledger_->LoadURL(
      url,
      registerHeaders,
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      on_register);
}

void Create::RegisterPersonaCallback(
      const ledger::UrlResponse& response,
      const std::string& user_id,
      const std::string& pre_flight,
      const std::string& registrar_vk,
      ledger::ResultCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code != net::HTTP_OK) {
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  std::string verification;
  if (!braveledger_bat_helper::getJSONValue(VERIFICATION_FIELDNAME,
                                            response.body,
                                            &verification)) {
    BLOG(0, "Verification is missing");
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  const char* masterUserToken = registerUserFinal(
      user_id.c_str(),
      verification.c_str(),
      pre_flight.c_str(),
      registrar_vk.c_str());

  if (masterUserToken != nullptr) {
    // should fix in
    // https://github.com/brave-intl/bat-native-anonize/issues/11
    free((void*)masterUserToken); // NOLINT
  } else if (!ledger::is_testing) {
    BLOG(0, "Master token error");
    callback(ledger::Result::REGISTRATION_VERIFICATION_FAILED);
    return;
  }

  ledger::WalletInfoProperties wallet_info = ledger_->GetWalletInfo();
  if (!braveledger_bat_helper::getJSONWalletInfo(
      response.body,
      &wallet_info)) {
    BLOG(0, "Can't get wallet info");
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  ledger_->SetRewardsMainEnabled(true);
  ledger_->SetAutoContributeEnabled(true);
  ledger_->SetWalletInfo(wallet_info);
  ledger_->SetCreationStamp(braveledger_time_util::GetCurrentTimeStamp());
  ledger_->ResetReconcileStamp();
  callback(ledger::Result::WALLET_CREATED);
}

}  // namespace braveledger_wallet
