/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_client.h"
#include "static_values.h"
#include <algorithm>
#include "bat_helper.h"
#include "base/bind.h"
#include "base/guid.h"

#include "anon.h"

namespace bat_client {

BatClient::BatClient(const bool& useProxy):
      useProxy_(useProxy),
      publisherTimestamp_(0) {
  initAnonize();
}

BatClient::~BatClient() {
}

std::string BatClient::buildURL(const std::string& path, const std::string& prefix) {
  std::string url;
  if (ledger::g_isProduction) {
    url = useProxy_ ? LEDGER_PRODUCTION_PROXY_SERVER : LEDGER_PRODUCTION_SERVER;
  } else {
    url = LEDGER_STAGING_SERVER;
  }

  return url + prefix + path;
}

void BatClient::loadStateOrRegisterPersona() {
  BatHelper::loadState(base::Bind(&BatClient::loadStateOrRegisterPersonaCallback,
    base::Unretained(this)));
}

void BatClient::loadStateOrRegisterPersonaCallback(bool result, const CLIENT_STATE_ST& state) {
  if (!result) {
    registerPersona();

    return;
  }
  LOG(ERROR) << "!!!bat address1 == " << state.walletInfo_.addressBAT_;
  state_ = state;
  publisherTimestamp(false);
}

void BatClient::registerPersona() {
  // We should use simple callbacks on iOS
  batClientWebRequest_.run(buildURL(REGISTER_PERSONA, PREFIX_V2),
    base::Bind(&BatClient::requestCredentialsCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", FETCH_CALLBACK_EXTRA_DATA_ST());
}

void BatClient::requestCredentialsCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST&) {
  //LOG(ERROR) << "!!!response == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }
  if (state_.personaId_.empty()) {
    state_.personaId_ = base::GenerateGUID();
  }
  // Anonize2 limit is 31 octets
  state_.userId_ = state_.personaId_;
  state_.userId_.erase(std::remove(state_.userId_.begin(), state_.userId_.end(), '-'), state_.userId_.end());
  state_.userId_.erase(12, 1);

  state_.registrarVK_ = BatHelper::getJSONValue(REGISTRARVK_FIELDNAME, response);
  DCHECK(!state_.registrarVK_.empty());
  const char* cred = makeCred(state_.userId_.c_str());
  if (nullptr != cred) {
    preFlight_ = cred;
    free((void*)cred);
  }
  DCHECK(!preFlight_.empty());
  const char* proofTemp = registerUserMessage(preFlight_.c_str(), state_.registrarVK_.c_str());
  std::string proof;
  if (nullptr != proofTemp) {
    proof = proofTemp;
    free((void*)proofTemp);
  }
  DCHECK(!proof.empty());
  state_.walletInfo_.keyInfoSeed_ = BatHelper::generateSeed();
  std::vector<uint8_t> secretKey = BatHelper::getHKDF(state_.walletInfo_.keyInfoSeed_);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  BatHelper::getPublicKeyFromSeed(secretKey, publicKey, newSecretKey);
  std::string label = base::GenerateGUID();
  std::string publicKeyHex = BatHelper::uint8ToHex(publicKey);
  std::string keys[3] = {"currency", "label", "publicKey"};
  std::string values[3] = {CURRENCY, label, publicKeyHex};
  std::string octets = BatHelper::stringify(keys, values, 3);
  std::string headerDigest = "SHA-256=" + BatHelper::getBase64(BatHelper::getSHA256(octets));
  std::string headerKeys[1] = {"digest"};
  std::string headerValues[1] = {headerDigest};
  std::string headerSignature = BatHelper::sign(headerKeys, headerValues,
    1, "primary", newSecretKey);

  REQUEST_CREDENTIALS_ST requestCredentials;
  requestCredentials.requestType_ = "httpSignature";
  requestCredentials.proof_ = proof;
  requestCredentials.request_body_currency_ = CURRENCY;
  requestCredentials.request_body_label_ = label;
  requestCredentials.request_body_publicKey_ = publicKeyHex;
  requestCredentials.request_headers_digest_ = headerDigest;
  requestCredentials.request_headers_signature_ = headerSignature;
  requestCredentials.request_body_octets_ = octets;
  std::string payloadStringify = BatHelper::stringifyRequestCredentialsSt(requestCredentials);
  //LOG(ERROR) << "!!!payloadStringify == " << payloadStringify1;
  std::vector<std::string> headers;
  headers.push_back("Content-Type: application/json; charset=UTF-8");
  // We should use simple callbacks on iOS
  batClientWebRequest_.run(buildURL((std::string)REGISTER_PERSONA + "/" + state_.userId_, PREFIX_V2),
    base::Bind(&BatClient::registerPersonaCallback,
      base::Unretained(this)), headers, payloadStringify, "application/json; charset=utf-8", FETCH_CALLBACK_EXTRA_DATA_ST());
}

void BatClient::registerPersonaCallback(bool result, const std::string& response,
    const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  if (!result) {
    // TODO errors handling
    return;
  }

  std::string verification = BatHelper::getJSONValue(VERIFICATION_FIELDNAME, response);
  const char* masterUserToken = registerUserFinal(state_.userId_.c_str(), verification.c_str(),
    preFlight_.c_str(), state_.registrarVK_.c_str());
  if (nullptr != masterUserToken) {
    state_.masterUserToken_ = masterUserToken;
    free((void*)masterUserToken);
  }

  BatHelper::getJSONWalletInfo(response, state_.walletInfo_, state_.fee_currency_, state_.fee_amount_, state_.days_);
  state_.bootStamp_ = BatHelper::currentTime() * 1000;
  state_.reconcileStamp_ = state_.bootStamp_ + state_.days_ * 24 * 60 * 60 * 1000;
  publisherTimestamp();
}

void BatClient::publisherTimestamp(const bool& saveState) {
  // We should use simple callbacks on iOS
  FETCH_CALLBACK_EXTRA_DATA_ST extraData;
  extraData.boolean1 = saveState;
  batClientWebRequest_.run(buildURL(PUBLISHER_TIMESTAMP, PREFIX_V3),
    base::Bind(&BatClient::publisherTimestampCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", FETCH_CALLBACK_EXTRA_DATA_ST());
}

void BatClient::publisherTimestampCallback(bool result, const std::string& response,
    const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  if (!result) {
    // TODO errors handling
    return;
  }
  BatHelper::getJSONPublisherTimeStamp(response, publisherTimestamp_);
  if (!extraData.boolean1) {
    return;
  }
  std::lock_guard<std::mutex> guard(state_mutex_);
  BatHelper::saveState(state_);
}

uint64_t BatClient::getPublisherTimestamp() {
  return publisherTimestamp_;
}

void BatClient::publisherInfo(const std::string& publisher, BatHelper::FetchCallback callback,
    const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  batClientWebRequest_.run(buildURL(PUBLISHER_INFO + publisher, PREFIX_V3),
    callback, std::vector<std::string>(), "", "", extraData);
}

void BatClient::setContributionAmount(const double& amount) {
  std::lock_guard<std::mutex> guard(state_mutex_);
  state_.fee_amount_ = amount;
  BatHelper::saveState(state_);
}

std::string BatClient::getBATAddress() {
  return state_.walletInfo_.addressBAT_;
}

std::string BatClient::getBTCAddress() {
  return state_.walletInfo_.addressBTC_;
}

std::string BatClient::getETHAddress() {
  return state_.walletInfo_.addressETH_;
}

std::string BatClient::getLTCAddress() {
  return state_.walletInfo_.addressLTC_;
}

void BatClient::getWalletProperties(BatHelper::FetchCallback callback,
    const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  balance_.getWalletProperties(state_.walletInfo_.paymentId_, callback, extraData);
}

}
