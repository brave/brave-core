/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_client.h"

#include <algorithm>
#include <sstream>

#include "ledger_impl.h"
#include "bat_helper.h"
#include "rapidjson_bat_helper.h"
#include "static_values.h"

#include "anon/anon.h"
#include "wally_bip39.h"

using namespace std::placeholders;

namespace braveledger_bat_client {

BatClient::BatClient(bat_ledger::LedgerImpl* ledger) :
      ledger_(ledger),
      state_(new braveledger_bat_helper::CLIENT_STATE_ST()) {
  // Enable emscripten calls
  //braveledger_bat_helper::readEmscripten();
  initAnonize();
}

BatClient::~BatClient() {
}

bool BatClient::loadState(const std::string& data) {
  braveledger_bat_helper::CLIENT_STATE_ST state;
  if (!braveledger_bat_helper::loadFromJson(state, data.c_str())) {
    ledger_->Log(__func__, ledger::LogLevel::ERROR, {"Failed to load client state: ", data});
    return false;
  }

  state_.reset(new braveledger_bat_helper::CLIENT_STATE_ST(state));

  bool stateChanged = false;

  // clear old reconciles
  if (state_->batch_.size() == 0) {
    state_->current_reconciles_ = {};
    stateChanged = true;
  }

  // fix timestamp ms to s conversion
  if (std::to_string(state_->reconcileStamp_).length() > 10) {
    state_->reconcileStamp_ = state_->reconcileStamp_ / 1000;
    stateChanged = true;
  }

  // fix timestamp ms to s conversion
  if (std::to_string(state_->bootStamp_).length() > 10) {
    state_->bootStamp_ = state_->bootStamp_ / 1000;
    stateChanged = true;
  }

  if (stateChanged) {
    saveState();
  }

  return true;
}

void BatClient::registerPersona() {
  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL(REGISTER_PERSONA, PREFIX_V2),
      std::vector<std::string>(), "", "",
      ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::requestCredentialsCallback,
                                       this,
                                       _1,
                                       _2,
                                       _3));
}

void BatClient::requestCredentialsCallback(bool result, const std::string& response, const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  if (!result) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  if (state_->personaId_.empty()) {
    state_->personaId_ = ledger_->GenerateGUID();
  }
  // Anonize2 limit is 31 octets
  state_->userId_ = state_->personaId_;
  state_->userId_.erase(std::remove(state_->userId_.begin(), state_->userId_.end(), '-'), state_->userId_.end());
  state_->userId_.erase(12, 1);

  if (!braveledger_bat_helper::getJSONValue(REGISTRARVK_FIELDNAME, response, state_->registrarVK_)) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }
  DCHECK(!state_->registrarVK_.empty());
  std::string proof = getAnonizeProof(state_->registrarVK_, state_->userId_, state_->preFlight_);

  if (proof.empty()) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }
  state_->walletInfo_.keyInfoSeed_ = braveledger_bat_helper::generateSeed();
  std::vector<uint8_t> secretKey = braveledger_bat_helper::getHKDF(state_->walletInfo_.keyInfoSeed_);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  braveledger_bat_helper::getPublicKeyFromSeed(secretKey, publicKey, newSecretKey);
  std::string label = ledger_->GenerateGUID();
  std::string publicKeyHex = braveledger_bat_helper::uint8ToHex(publicKey);
  std::string keys[3] = {"currency", "label", "publicKey"};
  std::string values[3] = {CURRENCY, label, publicKeyHex};
  std::string octets = braveledger_bat_helper::stringify(keys, values, 3);
  std::string headerDigest = "SHA-256=" + braveledger_bat_helper::getBase64(braveledger_bat_helper::getSHA256(octets));
  std::string headerKeys[1] = {"digest"};
  std::string headerValues[1] = {headerDigest};
  std::string headerSignature = braveledger_bat_helper::sign(headerKeys, headerValues, 1, "primary", newSecretKey);

  braveledger_bat_helper::REQUEST_CREDENTIALS_ST requestCredentials;
  requestCredentials.requestType_ = "httpSignature";
  requestCredentials.proof_ = proof;
  requestCredentials.request_body_currency_ = CURRENCY;
  requestCredentials.request_body_label_ = label;
  requestCredentials.request_body_publicKey_ = publicKeyHex;
  requestCredentials.request_headers_digest_ = headerDigest;
  requestCredentials.request_headers_signature_ = headerSignature;
  requestCredentials.request_body_octets_ = octets;
  std::string payloadStringify = braveledger_bat_helper::stringifyRequestCredentialsSt(requestCredentials);
  std::vector<std::string> registerHeaders;
  registerHeaders.push_back("Content-Type: application/json; charset=UTF-8");

  // We should use simple callbacks on iOS
  auto request_id = ledger_->LoadURL(
    braveledger_bat_helper::buildURL((std::string)REGISTER_PERSONA + "/" + state_->userId_, PREFIX_V2),
    registerHeaders, payloadStringify, "application/json; charset=utf-8",
    ledger::URL_METHOD::POST, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::registerPersonaCallback,
                                       this,
                                       _1,
                                       _2,
                                       _3));
}

std::string BatClient::getAnonizeProof(const std::string& registrarVK, const std::string& id, std::string& preFlight) {
  const char* cred = makeCred(id.c_str());
  if (nullptr != cred) {
    preFlight = cred;
    free((void*)cred);
  } else {
    return "";
  }
  const char* proofTemp = registerUserMessage(preFlight.c_str(), registrarVK.c_str());
  std::string proof;
  if (nullptr != proofTemp) {
    proof = proofTemp;
    free((void*)proofTemp);
  } else {
    return "";
  }

  return proof;
}

void BatClient::registerPersonaCallback(bool result,
                                       const std::string& response,
                                       const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  if (!result) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  std::string verification;
  if (!braveledger_bat_helper::getJSONValue(VERIFICATION_FIELDNAME, response, verification)) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }
  const char* masterUserToken = registerUserFinal(state_->userId_.c_str(), verification.c_str(),
    state_->preFlight_.c_str(), state_->registrarVK_.c_str());
  if (nullptr != masterUserToken) {
    state_->masterUserToken_ = masterUserToken;
    free((void*)masterUserToken);
  } else if (!braveledger_bat_helper::ignore_for_testing()) {
    ledger_->OnWalletInitialized(ledger::Result::REGISTRATION_VERIFICATION_FAILED);
    return;
  }

  if (!braveledger_bat_helper::getJSONWalletInfo(response, state_->walletInfo_, state_->fee_currency_, state_->fee_amount_, state_->days_)) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }
  // In seconds
  // Do we need bootStamp_ at all?
  state_->bootStamp_ = braveledger_bat_helper::currentTime();
  // Should we use _reconcile_default_interval or state_->days_?
  // In seconds

  resetReconcileStamp();

  ledger_->OnWalletInitialized(ledger::Result::WALLET_CREATED);
}

void BatClient::resetReconcileStamp() {
  if (ledger::reconcile_time > 0) {
    state_->reconcileStamp_ = braveledger_bat_helper::currentTime() + ledger::reconcile_time * 60;
  } else {
    state_->reconcileStamp_ = braveledger_bat_helper::currentTime() + braveledger_ledger::_reconcile_default_interval;
  }
  saveState();
}

bool BatClient::didUserChangeContributionAmount() const {
  return state_->user_changed_fee_;
}

void BatClient::setContributionAmount(const double& amount) {
  state_->fee_amount_ = amount;
  saveState();
}

void BatClient::setUserChangedContribution() {
    state_->user_changed_fee_ = true;
    saveState();
  }

double BatClient::getContributionAmount() const {
  return state_->fee_amount_;
}

void BatClient::setRewardsMainEnabled(const bool& enabled) {
  state_->rewards_enabled_ = enabled;
  saveState();
}

bool BatClient::getRewardsMainEnabled() const {
  return state_->rewards_enabled_;
}

void BatClient::setAutoContribute(const bool& enabled) {
  state_->auto_contribute_ = enabled;
  saveState();
}

bool BatClient::getAutoContribute() const {
  return state_->auto_contribute_;
}

const std::string& BatClient::getBATAddress() const {
  return state_->walletInfo_.addressBAT_;
}

const std::string& BatClient::getBTCAddress() const {
  return state_->walletInfo_.addressBTC_;
}

const std::string& BatClient::getETHAddress() const {
  return state_->walletInfo_.addressETH_;
}

const std::string& BatClient::getLTCAddress() const {
  return state_->walletInfo_.addressLTC_;
}

uint64_t BatClient::getReconcileStamp() const {
  return state_->reconcileStamp_;
}

void BatClient::getWalletProperties() {
  std::string path = (std::string)WALLET_PROPERTIES + state_->walletInfo_.paymentId_ + WALLET_PROPERTIES_END;
   auto request_id = ledger_->LoadURL(
       braveledger_bat_helper::buildURL(path, PREFIX_V2, braveledger_bat_helper::SERVER_TYPES::BALANCE),
       std::vector<std::string>(),
       "",
       "",
       ledger::URL_METHOD::GET,
       &handler_);

   handler_.AddRequestHandler(std::move(request_id),
                              std::bind(&BatClient::walletPropertiesCallback,
                                        this,
                                        _1,
                                        _2,
                                        _3));
 }

 void BatClient::walletPropertiesCallback(bool success,
                                          const std::string& response,
                                          const std::map<std::string, std::string>& headers) {
   braveledger_bat_helper::WALLET_PROPERTIES_ST properties;
   ledger_->LogResponse(__func__, success, response, headers);
   if (!success) {
     ledger_->OnWalletProperties(ledger::Result::LEDGER_ERROR, properties);
     return;
   }

   bool ok = braveledger_bat_helper::loadFromJson(properties, response);
   if (!ok) {
     ledger_->Log(__func__, ledger::LogLevel::ERROR, {"Failed to load wallet properties state."});
     ledger_->OnWalletProperties(ledger::Result::LEDGER_ERROR, properties);
     return;
   }
   state_->walletProperties_ = properties;
   ledger_->OnWalletProperties(ledger::Result::LEDGER_OK, properties);
}

bool BatClient::isReadyForReconcile() {
  // TODO real check of reconcile timestamp
  return true;
}

void BatClient::reconcilePublisherList(const ledger::PUBLISHER_CATEGORY category,
                                       const ledger::PublisherInfoList& list) {

  std::vector<braveledger_bat_helper::PUBLISHER_ST> newList;

  for(const auto& publisher: list) {
    braveledger_bat_helper::PUBLISHER_ST new_publisher;
    new_publisher.id_ = publisher.id;
    new_publisher.percent_ = publisher.percent;
    new_publisher.weight_ = publisher.weight;
    new_publisher.duration_ = publisher.duration;
    new_publisher.score_ = publisher.score;
    new_publisher.visits_ = publisher.visits;

    newList.push_back(new_publisher);
  }

  reconcile(ledger_->GenerateGUID(), category, newList);
}

void BatClient::reconcile(const std::string& viewingId,
    const ledger::PUBLISHER_CATEGORY category,
    const std::vector<braveledger_bat_helper::PUBLISHER_ST>& list,
    const std::vector<braveledger_bat_helper::RECONCILE_DIRECTION>& directions) {
  if (state_->current_reconciles_.count(viewingId) > 0) {
    ledger_->Log(__func__, ledger::LogLevel::ERROR, {"unable to reconcile with the same viewing id"});
    // TODO add error callback
    return;
  }

  auto reconcile = braveledger_bat_helper::CURRENT_RECONCILE();

  double fee = .0;

  double balance = getBalance();

  if (category == ledger::PUBLISHER_CATEGORY ::AUTO_CONTRIBUTE) {
    double ac_amount = getContributionAmount();

    if (list.size() == 0 || ac_amount > balance) {
      if (list.size() == 0) {
        ledger_->Log(__func__, ledger::LogLevel::INFO, {"AC table is empty"});
      }

      if (ac_amount > balance) {
        ledger_->Log(__func__, ledger::LogLevel::INFO, {"You don't have enough funds for AC contribution"});
      }

      resetReconcileStamp();
      // TODO add error callback
      return;
    }

    reconcile.list_ = list;
  }

  if (category == ledger::PUBLISHER_CATEGORY::RECURRING_DONATION) {
    double ac_amount = getContributionAmount();
    if (list.size() == 0) {
      ledger_->Log(__func__, ledger::LogLevel::INFO, {"recurring donation list is empty"});
      ledger_->StartAutoContribute();
      // TODO add error callback
      return;
    }

    for (const auto& publisher : list) {
      if (publisher.id_.empty()) {
        ledger_->Log(__func__, ledger::LogLevel::ERROR, {"recurring donation is missing publisher"});
        ledger_->StartAutoContribute();
        // TODO add error callback
        return;
      }

      fee += publisher.weight_;
    }

    if (fee + ac_amount > balance) {
        ledger_->Log(__func__, ledger::LogLevel::ERROR, {"You don't have enough funds to do recurring and AC contribution"});
      // TODO add error callback
      return;
    }

    reconcile.list_ = list;
  }

  if (category == ledger::PUBLISHER_CATEGORY::DIRECT_DONATION) {
    for (const auto& direction : directions) {
      if (direction.publisher_key_.empty()) {
        ledger_->Log(__func__, ledger::LogLevel::ERROR, {"reconcile direction missing publisher"});
        // TODO add error callback
        return;
      }

      if (direction.currency_ != CURRENCY) {
        ledger_->Log(__func__, ledger::LogLevel::ERROR, {"reconcile direction currency invalid for ", direction.publisher_key_});
        // TODO add error callback
        return;
      }

      fee += direction.amount_;
    }

    if (fee > balance) {
      ledger_->Log(__func__, ledger::LogLevel::ERROR, {"You don't have enough funds to do a tip"});
      // TODO add error callback
      return;
    }

  }

  reconcile.viewingId_ = viewingId;
  reconcile.fee_ = fee;
  reconcile.directions_ = directions;
  reconcile.category_ = category;

  state_->current_reconciles_.insert(std::make_pair(viewingId, reconcile));
  saveState();

  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL((std::string)RECONCILE_CONTRIBUTION + state_->userId_, PREFIX_V2),
      std::vector<std::string>(), "", "",
      ledger::URL_METHOD::GET,
      &handler_);

  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::reconcileCallback,
                                       this,
                                       viewingId,
                                       _1,
                                       _2,
                                       _3));
}

void BatClient::reconcileCallback(const std::string& viewingId,
                                  bool result,
                                  const std::string& response,
                                  const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  auto reconcile = GetReconcileById(viewingId);

  if (!result || reconcile.viewingId_.empty()) {
    // TODO errors handling
    return;
  }

  braveledger_bat_helper::getJSONValue(SURVEYOR_ID, response, reconcile.surveyorInfo_.surveyorId_);
  bool success = SetReconcile(reconcile);
  if (!success) {
    // TODO error handling
    return;
  }
  currentReconcile(viewingId);
}

void BatClient::currentReconcile(const std::string& viewingId) {
  std::ostringstream amount;
  auto reconcile = GetReconcileById(viewingId);

  if (reconcile.category_ == ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE) {
    amount << state_->fee_amount_;
  } else {
    amount << reconcile.fee_;
  }

  std::string path = (std::string)WALLET_PROPERTIES + state_->walletInfo_.paymentId_ + "?refresh=true" + "&amount=" + amount.str() + "&altcurrency=" + state_->fee_currency_;

  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL(path, PREFIX_V2),
      std::vector<std::string>(), "", "",
      ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::currentReconcileCallback,
                                       this,
                                       viewingId,
                                       _1,
                                       _2,
                                       _3));
}

braveledger_bat_helper::CURRENT_RECONCILE BatClient::GetReconcileById(const std::string& viewingId) {
  if (state_->current_reconciles_.count(viewingId) == 0) {
    ledger_->Log(__func__, ledger::LogLevel::ERROR, {"Could not find any reconcile tasks with the id ", viewingId});
    // For safety we don't crash, perhaps in a dev build we want to throw an exception anyways
    return braveledger_bat_helper::CURRENT_RECONCILE();
  }
  return state_->current_reconciles_[viewingId];
}

bool BatClient::SetReconcile(const braveledger_bat_helper::CURRENT_RECONCILE& reconcile) {
  if (state_->current_reconciles_.count(reconcile.viewingId_) == 0) {
    return false;
  }

  state_->current_reconciles_[reconcile.viewingId_] = reconcile;
  saveState();
  return true;
}

void BatClient::removeReconcileById(const std::string& viewingId) {
  state_->current_reconciles_.erase(state_->current_reconciles_.find(viewingId));
  saveState();
}

void BatClient::currentReconcileCallback(const std::string& viewingId,
                                         bool result,
                                         const std::string& response,
                                         const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  if (!result) {
    ledger_->OnReconcileComplete(ledger::Result::LEDGER_ERROR, viewingId);
    // TODO errors handling
    return;
  }

  auto reconcile = GetReconcileById(viewingId);

  braveledger_bat_helper::getJSONRates(response, reconcile.rates_);
  braveledger_bat_helper::UNSIGNED_TX unsignedTx;
  braveledger_bat_helper::getJSONUnsignedTx(response, unsignedTx);
  if (unsignedTx.amount_.empty() && unsignedTx.currency_.empty() && unsignedTx.destination_.empty()) {
    ledger_->OnReconcileComplete(ledger::Result::LEDGER_ERROR, reconcile.viewingId_);
    // We don't have any unsigned transactions
    return;
  }
  reconcile.amount_ = unsignedTx.amount_;
  reconcile.currency_ = unsignedTx.currency_;
  bool success = SetReconcile(reconcile);
  if (!success) {
    // TODO error handling
    return;
  }

  std::string octets = braveledger_bat_helper::stringifyUnsignedTx(unsignedTx);
  std::string headerDigest = "SHA-256=" + braveledger_bat_helper::getBase64(braveledger_bat_helper::getSHA256(octets));
  std::string headerKeys[1] = {"digest"};
  std::string headerValues[1] = {headerDigest};
  std::vector<uint8_t> secretKey = braveledger_bat_helper::getHKDF(state_->walletInfo_.keyInfoSeed_);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  braveledger_bat_helper::getPublicKeyFromSeed(secretKey, publicKey, newSecretKey);
  std::string headerSignature = braveledger_bat_helper::sign(headerKeys, headerValues, 1, "primary", newSecretKey);

  braveledger_bat_helper::RECONCILE_PAYLOAD_ST reconcilePayload;
  reconcilePayload.requestType_ = "httpSignature";
  reconcilePayload.request_signedtx_headers_digest_ = headerDigest;
  reconcilePayload.request_signedtx_headers_signature_ = headerSignature;
  reconcilePayload.request_signedtx_body_ = unsignedTx;
  reconcilePayload.request_signedtx_octets_ = octets;
  reconcilePayload.request_viewingId_ = reconcile.viewingId_;
  reconcilePayload.request_surveyorId_ = reconcile.surveyorInfo_.surveyorId_;
  std::string payloadStringify = braveledger_bat_helper::stringifyReconcilePayloadSt(reconcilePayload);

  std::vector<std::string> walletHeader;
  walletHeader.push_back("Content-Type: application/json; charset=UTF-8");
  std::string path = (std::string)WALLET_PROPERTIES + state_->walletInfo_.paymentId_;

  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL(path, PREFIX_V2),
    walletHeader, payloadStringify, "application/json; charset=utf-8",
    ledger::URL_METHOD::PUT,
    &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::reconcilePayloadCallback,
                                       this,
                                       viewingId,
                                       _1,
                                       _2,
                                       _3));
}

void BatClient::reconcilePayloadCallback(const std::string& viewingId,
                                         bool result,
                                         const std::string& response,
                                         const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  if (!result) {
    ledger_->OnReconcileComplete(ledger::Result::LEDGER_ERROR, viewingId);
    // TODO errors handling
    return;
  }

  const auto reconcile = GetReconcileById(viewingId);

  braveledger_bat_helper::TRANSACTION_ST transaction;
  braveledger_bat_helper::getJSONTransaction(response, transaction);
  transaction.viewingId_ = reconcile.viewingId_;
  transaction.surveyorId_ = reconcile.surveyorInfo_.surveyorId_;
  transaction.contribution_rates_ = reconcile.rates_;
  transaction.contribution_fiat_amount_ = reconcile.amount_;
  transaction.contribution_fiat_currency_ = reconcile.currency_;

  state_->transactions_.push_back(transaction);
  saveState();
  registerViewing(viewingId);
}

void BatClient::registerViewing(const std::string& viewingId) {
  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL((std::string)REGISTER_VIEWING, PREFIX_V2),
    std::vector<std::string>(), "", "",
    ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::registerViewingCallback,
                                       this,
                                       viewingId,
                                       _1,
                                       _2,
                                       _3));
}

void BatClient::registerViewingCallback(const std::string& viewingId,
                                        bool result,
                                        const std::string& response,
                                        const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  if (!result) {
    ledger_->OnReconcileComplete(ledger::Result::LEDGER_ERROR, viewingId);
    // TODO errors handling
    return;
  }

  auto reconcile = GetReconcileById(viewingId);

  braveledger_bat_helper::getJSONValue(REGISTRARVK_FIELDNAME, response, reconcile.registrarVK_);
  DCHECK(!reconcile.registrarVK_.empty());
  reconcile.anonizeViewingId_ = reconcile.viewingId_;
  reconcile.anonizeViewingId_.erase(std::remove(reconcile.anonizeViewingId_.begin(), reconcile.anonizeViewingId_.end(), '-'), reconcile.anonizeViewingId_.end());
  reconcile.anonizeViewingId_.erase(12, 1);
  std::string proof = getAnonizeProof(reconcile.registrarVK_, reconcile.anonizeViewingId_, reconcile.preFlight_);

  bool success = SetReconcile(reconcile);
  if (!success) {
    // TODO error handling
    return;
  }

  std::string keys[1] = {"proof"};
  std::string values[1] = {proof};
  std::string proofStringified = braveledger_bat_helper::stringify(keys, values, 1);
  viewingCredentials(viewingId, proofStringified, reconcile.anonizeViewingId_);
}

void BatClient::viewingCredentials(const std::string& viewingId, const std::string& proofStringified, const std::string& anonizeViewingId) {
  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL((std::string)REGISTER_VIEWING + "/" + anonizeViewingId, PREFIX_V2),
    std::vector<std::string>(), proofStringified, "application/json; charset=utf-8",
    ledger::URL_METHOD::POST, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::viewingCredentialsCallback,
                                       this,
                                       viewingId,
                                       _1,
                                       _2,
                                       _3));
}

void BatClient::viewingCredentialsCallback(const std::string& viewingId,
                                           bool result,
                                           const std::string& response,
                                           const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  if (!result) {
    ledger_->OnReconcileComplete(ledger::Result::LEDGER_ERROR, viewingId);
    // TODO errors handling
    return;
  }

  auto reconcile = GetReconcileById(viewingId);

  std::string verification;
  braveledger_bat_helper::getJSONValue(VERIFICATION_FIELDNAME, response, verification);
  const char* masterUserToken = registerUserFinal(reconcile.anonizeViewingId_.c_str(), verification.c_str(),
    reconcile.preFlight_.c_str(), reconcile.registrarVK_.c_str());

  if (nullptr != masterUserToken) {
    reconcile.masterUserToken_ = masterUserToken;
    free((void*)masterUserToken);
  }

  bool success = SetReconcile(reconcile);
  if (!success) {
    // TODO error handling
    return;
  }

  std::vector<std::string> surveyors;
  braveledger_bat_helper::getJSONList(SURVEYOR_IDS, response, surveyors);
  std::string probi = "0";
  // Save the rest values to transactions
  for (size_t i = 0; i < state_->transactions_.size(); i++) {
    if (state_->transactions_[i].viewingId_ != reconcile.viewingId_) {
      continue;
    }
    state_->transactions_[i].anonizeViewingId_ = reconcile.anonizeViewingId_;
    state_->transactions_[i].registrarVK_ = reconcile.registrarVK_;
    state_->transactions_[i].masterUserToken_ = reconcile.masterUserToken_;
    state_->transactions_[i].surveyorIds_ = surveyors;
    probi = state_->transactions_[i].contribution_probi_;
  }

  saveState();
  ledger_->OnReconcileComplete(ledger::Result::LEDGER_OK, reconcile.viewingId_, probi);
}

unsigned int BatClient::ballots(const std::string& viewingId) {
  unsigned int count = 0;
  for (size_t i = 0; i < state_->transactions_.size(); i++) {
    if (state_->transactions_[i].votes_ < state_->transactions_[i].surveyorIds_.size()
        && state_->transactions_[i].viewingId_ == viewingId) {
      count += state_->transactions_[i].surveyorIds_.size() - state_->transactions_[i].votes_;
    }
  }

  return count;
}

void BatClient::votePublishers(const std::vector<std::string>& publishers, const std::string& viewingId) {
  for (size_t i = 0; i < publishers.size(); i++) {
    vote(publishers[i], viewingId);
  }
  saveState();
}

void BatClient::vote(const std::string& publisher, const std::string& viewingId) {
  DCHECK(!publisher.empty());
  if (publisher.empty()) {
    return;
  }
  braveledger_bat_helper::BALLOT_ST ballot;
  int i = 0;
  for (i = state_->transactions_.size() - 1; i >=0; i--) {
    if (state_->transactions_[i].votes_ >= state_->transactions_[i].surveyorIds_.size()) {
      continue;
    }
    if (state_->transactions_[i].viewingId_ == viewingId || viewingId.empty()) {
      break;
    }
  }
  if (i < 0) {
    return;
  }
  ballot.viewingId_ = state_->transactions_[i].viewingId_;
  ballot.surveyorId_ = state_->transactions_[i].surveyorIds_[state_->transactions_[i].votes_];
  ballot.publisher_ = publisher;
  ballot.offset_ = state_->transactions_[i].votes_;
  state_->transactions_[i].votes_++;
  state_->ballots_.push_back(ballot);
}

void BatClient::prepareBallots() {
  //uint64_t currentTime = braveledger_bat_helper::currentTime() * 1000;
  //std::vector<BATCH_PROOF> batchProof;
  for (int i = state_->ballots_.size() - 1; i >= 0; i--) {
    bool breakTheLoop = false;
    for (size_t j = 0; j < state_->transactions_.size(); j++) {
      // TODO check on valid credentials for transaction
      if (state_->transactions_[j].viewingId_ == state_->ballots_[i].viewingId_
          /*&& (state_->ballots_[i].prepareBallot_.empty() || 0 == state_->ballots_[i].delayStamp_
            || state_->ballots_[i].delayStamp_ <= currentTime)*/) {
        // TODO check on ballot.prepareBallot and call commitBallot if it exist
        if (state_->ballots_[i].prepareBallot_.empty()) {
          prepareBatch(state_->ballots_[i], state_->transactions_[j]);
          breakTheLoop = true;
          break;
        }
      }
    }
    if (breakTheLoop) {
      break;
    }
  }
}

void BatClient::prepareBatch(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction) {
  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL((std::string)SURVEYOR_BATCH_VOTING + "/" + transaction.anonizeViewingId_, PREFIX_V2),
    std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::prepareBatchCallback,
                                       this,
                                       _1,
                                       _2,
                                       _3));
}

void BatClient::prepareBatchCallback(bool result,
                                     const std::string& response,
                                     const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  std::vector<std::string> surveyors;
  braveledger_bat_helper::getJSONBatchSurveyors(response, surveyors);
  std::vector<braveledger_bat_helper::BATCH_PROOF> batchProof;
  for (size_t j = 0; j < surveyors.size(); j++) {
    std::string error;
    braveledger_bat_helper::getJSONValue("error", surveyors[j], error);
    if (!error.empty()) {
      continue;
    }
    for (int i = state_->ballots_.size() - 1; i >= 0; i--) {
      std::string survId;
      braveledger_bat_helper::getJSONValue("surveyorId", surveyors[j], survId);
      if (state_->ballots_[i].surveyorId_ == survId) {
        for (size_t k = 0; k < state_->transactions_.size(); k++) {
          if (state_->transactions_[k].viewingId_ == state_->ballots_[i].viewingId_) {
            state_->ballots_[i].prepareBallot_ = surveyors[j];
            braveledger_bat_helper::BATCH_PROOF batchProofEl;
            batchProofEl.transaction_ = state_->transactions_[k];
            batchProofEl.ballot_ = state_->ballots_[i];
            batchProof.push_back(batchProofEl);
          }
        }
      }
    }
  }

  saveState();
  ledger_->RunIOTask(std::bind(&BatClient::proofBatch, this, batchProof, _1));
}

void BatClient::proofBatch(
    const std::vector<braveledger_bat_helper::BATCH_PROOF>& batchProof,
    ledger::LedgerTaskRunner::CallerThreadCallback callback) {
  std::vector<std::string> proofs;

  for (size_t i = 0; i < batchProof.size(); i++) {
    braveledger_bat_helper::SURVEYOR_ST surveyor;
    if (!braveledger_bat_helper::loadFromJson(surveyor, batchProof[i].ballot_.prepareBallot_)) {
      ledger_->Log(__func__, ledger::LogLevel::ERROR, {"Failed to load surveyor state: ", batchProof[i].ballot_.prepareBallot_});
    }

    std::string signatureToSend;
    size_t delimeterPos = surveyor.signature_.find(',');
    if (std::string::npos != delimeterPos && delimeterPos + 1 <= surveyor.signature_.length()) {
      signatureToSend = surveyor.signature_.substr(delimeterPos + 1);
      if (signatureToSend.length() > 1 && signatureToSend[0] == ' ') {
        signatureToSend.erase(0, 1);
      }
    }

    std::string keysMsg[1] = {"publisher"};
    std::string valuesMsg[1] = {batchProof[i].ballot_.publisher_};
    std::string msg = braveledger_bat_helper::stringify(keysMsg, valuesMsg, 1);

    const char* proof = submitMessage(msg.c_str(), batchProof[i].transaction_.masterUserToken_.c_str(),
      batchProof[i].transaction_.registrarVK_.c_str(), signatureToSend.c_str(), surveyor.surveyorId_.c_str(), surveyor.surveyVK_.c_str());

    std::string anonProof;
    if (nullptr != proof) {
      anonProof = proof;
      free((void*)proof);
    }

    proofs.push_back(anonProof);
  }

  callback(std::bind(&BatClient::proofBatchCallback, this, batchProof, proofs));
}

void BatClient::proofBatchCallback(
    const std::vector<braveledger_bat_helper::BATCH_PROOF>& batchProof,
    const std::vector<std::string>& proofs) {
  for (size_t i = 0; i < batchProof.size(); i++) {
    for (size_t j = 0; j < state_->ballots_.size(); j++) {
      if (state_->ballots_[j].surveyorId_ == batchProof[i].ballot_.surveyorId_) {
        state_->ballots_[j].proofBallot_ = proofs[i];
      }
    }
  }
  ledger_->PrepareVoteBatchTimer();
}

void BatClient::prepareVoteBatch() {
  for (int i = state_->ballots_.size() - 1; i >= 0; i--) {
    if (state_->ballots_[i].prepareBallot_.empty() || state_->ballots_[i].proofBallot_.empty()) {
      // TODO error handling
      continue;
    }
    bool transactionExist = false;
    for (size_t k = 0; k < state_->transactions_.size(); k++) {
      if (state_->transactions_[k].viewingId_ == state_->ballots_[i].viewingId_) {
        bool existBallot = false;
        for (size_t j = 0; j < state_->transactions_[k].ballots_.size(); j++) {
          if (state_->transactions_[k].ballots_[j].publisher_ == state_->ballots_[i].publisher_) {
            state_->transactions_[k].ballots_[j].offset_++;
            existBallot = true;
            break;
          }
        }
        if (!existBallot) {
          braveledger_bat_helper::TRANSACTION_BALLOT_ST transactionBallot;
          transactionBallot.publisher_ = state_->ballots_[i].publisher_;
          transactionBallot.offset_++;
          state_->transactions_[k].ballots_.push_back(transactionBallot);
        }
        transactionExist = true;
        break;
      }
    }
    if (!transactionExist) {
      continue;
    }
    bool existBatch = false;
    braveledger_bat_helper::BATCH_VOTES_INFO_ST batchVotesInfoSt;
    batchVotesInfoSt.surveyorId_ = state_->ballots_[i].surveyorId_;
    batchVotesInfoSt.proof_ = state_->ballots_[i].proofBallot_;
    for (size_t k = 0; k < state_->batch_.size(); k++) {
      if (state_->batch_[k].publisher_ == state_->ballots_[i].publisher_) {
        existBatch = true;
        state_->batch_[k].batchVotesInfo_.push_back(batchVotesInfoSt);
      }
    }
    if (!existBatch) {
      braveledger_bat_helper::BATCH_VOTES_ST batchVotesSt;
      batchVotesSt.publisher_ = state_->ballots_[i].publisher_;
      batchVotesSt.batchVotesInfo_.push_back(batchVotesInfoSt);
      state_->batch_.push_back(batchVotesSt);
    }
    state_->ballots_.erase(state_->ballots_.begin() + i);
  }
  saveState();

  ledger_->VoteBatchTimer();
}

void BatClient::voteBatch() {
  if (0 == state_->batch_.size()) {
    return;
  }
  braveledger_bat_helper::BATCH_VOTES_ST batchVotes = state_->batch_[0];
  std::vector<braveledger_bat_helper::BATCH_VOTES_INFO_ST> voteBatch;

  if (batchVotes.batchVotesInfo_.size() > VOTE_BATCH_SIZE) {
    voteBatch.assign(batchVotes.batchVotesInfo_.begin(), batchVotes.batchVotesInfo_.begin() + VOTE_BATCH_SIZE);
  } else {
    voteBatch = batchVotes.batchVotesInfo_;
  }

  std::string payload = braveledger_bat_helper::stringifyBatch(voteBatch);

  auto request_id = ledger_->LoadURL(
    braveledger_bat_helper::buildURL((std::string)SURVEYOR_BATCH_VOTING , PREFIX_V2),
    std::vector<std::string>(), payload, "application/json; charset=utf-8", ledger::URL_METHOD::POST, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::voteBatchCallback,
                                       this,
                                       batchVotes.publisher_,
                                       _1,
                                       _2,
                                       _3));
}

void BatClient::voteBatchCallback(const std::string& publisher,
                                  bool result,
                                  const std::string& response,
                                  const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  std::vector<std::string> surveyors;
  braveledger_bat_helper::getJSONBatchSurveyors(response, surveyors);
  for (size_t i = 0; i < state_->batch_.size(); i++) {
    if (state_->batch_[i].publisher_ == publisher) {
      size_t sizeToCheck = VOTE_BATCH_SIZE;
      if (state_->batch_[i].batchVotesInfo_.size() < VOTE_BATCH_SIZE) {
        sizeToCheck = state_->batch_[i].batchVotesInfo_.size();
      }
      for (int j = sizeToCheck - 1; j >= 0; j--) {
        for (size_t k = 0; k < surveyors.size(); k++) {
          std::string surveyorId;
          braveledger_bat_helper::getJSONValue("surveyorId", surveyors[k], surveyorId);
          if (surveyorId == state_->batch_[i].batchVotesInfo_[j].surveyorId_) {
            state_->batch_[i].batchVotesInfo_.erase(state_->batch_[i].batchVotesInfo_.begin() + j);
            break;
          }
        }
      }
      if (0 == state_->batch_[i].batchVotesInfo_.size()) {
        state_->batch_.erase(state_->batch_.begin() + i);
      }
      break;
    }
  }
  saveState();
  ledger_->VoteBatchTimer();
}

std::string BatClient::getWalletPassphrase() const {
  DCHECK(state_->walletInfo_.keyInfoSeed_.size());
  std::string passPhrase;
  if (0 == state_->walletInfo_.keyInfoSeed_.size()) {
    return passPhrase;
  }
    char* words = nullptr;
  int result = bip39_mnemonic_from_bytes(nullptr, &state_->walletInfo_.keyInfoSeed_.front(),
    state_->walletInfo_.keyInfoSeed_.size(), &words);
  if (0 != result) {
    DCHECK(false);

    return passPhrase;
  }
  passPhrase = words;
  wally_free_string(words);

  return passPhrase;
}

void BatClient::saveState() {
  std::string data;
  braveledger_bat_helper::saveToJsonString(*state_, data);
  ledger_->SaveLedgerState(data);
}

void BatClient::recoverWallet(const std::string& pass_phrase) {
  size_t written = 0;
  std::vector<unsigned char> newSeed;
  if (braveledger_bat_helper::split(pass_phrase,
    WALLET_PASSPHRASE_DELIM).size() == 16) {
    // use niceware for legacy wallet passphrases
    ledger_->LoadNicewareList(
      std::bind(&BatClient::OnNicewareListLoaded, this, pass_phrase, _1, _2));
  } else {
    std::vector<unsigned char> newSeed;
    newSeed.resize(32);
    int result = bip39_mnemonic_to_bytes
      (nullptr, pass_phrase.c_str(), &newSeed.front(),
      newSeed.size(), &written);
    continueRecover(result, &written, newSeed);
  }
}

void BatClient::OnNicewareListLoaded(const std::string& pass_phrase,
                                        ledger::Result result,
                                        const std::string& data) {
  if (result == ledger::Result::LEDGER_OK &&
    braveledger_bat_helper::split(pass_phrase,
    WALLET_PASSPHRASE_DELIM).size() == 16) {
    std::vector<uint8_t> seed;
    seed.resize(32);
    size_t written = 0;
    uint8_t nwResult = braveledger_bat_helper::niceware_mnemonic_to_bytes(
      pass_phrase, seed, &written, braveledger_bat_helper::split(
      data, DICTIONARY_DELIMITER));
    continueRecover(nwResult, &written, seed);
  } else {
    std::vector<braveledger_bat_helper::GRANT> empty;
    ledger_->OnRecoverWallet(result, 0, empty);
    return;
  }
}

void BatClient::continueRecover(int result, size_t *written, std::vector<uint8_t>& newSeed) {
  if (0 != result || 0 == *written) {
    ledger_->Log(__func__, ledger::LogLevel::ERROR, {"Result: ", std::to_string(result), " Size: ", std::to_string(*written)});
    std::vector<braveledger_bat_helper::GRANT> empty;
    ledger_->OnRecoverWallet(ledger::Result::LEDGER_ERROR, 0, empty);
    return;
  }
  state_->walletInfo_.keyInfoSeed_ = newSeed;

  std::vector<uint8_t> secretKey = braveledger_bat_helper::getHKDF(state_->walletInfo_.keyInfoSeed_);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  braveledger_bat_helper::getPublicKeyFromSeed(secretKey, publicKey, newSecretKey);
  std::string publicKeyHex = braveledger_bat_helper::uint8ToHex(publicKey);

  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL((std::string)RECOVER_WALLET_PUBLIC_KEY + publicKeyHex, PREFIX_V2),
    std::vector<std::string>(), "", "",
    ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::recoverWalletPublicKeyCallback,
                                       this,
                                       _1,
                                       _2,
                                       _3));

}

void BatClient::recoverWalletPublicKeyCallback(bool result,
                                               const std::string& response,
                                               const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  if (!result) {
    std::vector<braveledger_bat_helper::GRANT> empty;
    ledger_->OnRecoverWallet(ledger::Result::LEDGER_ERROR, 0, empty);
    return;
  }
  std::string recoveryId;
  braveledger_bat_helper::getJSONValue("paymentId", response, recoveryId);

  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL((std::string)WALLET_PROPERTIES + recoveryId, PREFIX_V2),
    std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::recoverWalletCallback,
                                       this,
                                       _1,
                                       _2,
                                       _3,
                                       recoveryId));
}

void BatClient::recoverWalletCallback(bool result,
                                      const std::string& response,
                                      const std::map<std::string, std::string>& headers,
                                      const std::string& recoveryId) {
  ledger_->LogResponse(__func__, result, response, headers);
  if (!result) {
    std::vector<braveledger_bat_helper::GRANT> empty;
    ledger_->OnRecoverWallet(ledger::Result::LEDGER_ERROR, 0, empty);
    return;
  }

  braveledger_bat_helper::getJSONWalletInfo(response, state_->walletInfo_, state_->fee_currency_, state_->fee_amount_, state_->days_);
  braveledger_bat_helper::getJSONRecoverWallet(response, state_->walletProperties_.balance_, state_->walletProperties_.probi_, state_->walletProperties_.grants_);
  state_->walletInfo_.paymentId_ = recoveryId;
  saveState();
  ledger_->OnRecoverWallet(ledger::Result::LEDGER_OK, state_->walletProperties_.balance_, state_->walletProperties_.grants_);
}

void BatClient::getGrant(const std::string& lang, const std::string& forPaymentId) {
  std::string paymentId = forPaymentId;
  if (paymentId.empty()) {
    paymentId = state_->walletInfo_.paymentId_;
  }
  std::string arguments;
  if (!paymentId.empty() || !lang.empty()) {
    arguments = "?";
    if (!paymentId.empty()) {
      arguments += "paymentId=" + paymentId;
    }
    if (!lang.empty()) {
      if (arguments.length() > 1) {
        arguments += "&";
      }
      arguments += "lang=" + lang;
    }
  }

  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL((std::string)GET_SET_PROMOTION + arguments, PREFIX_V2),
    std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::getGrantCallback,
                                       this,
                                       _1,
                                       _2,
                                       _3));
}

void BatClient::getGrantCallback(bool success,
                                 const std::string& response,
                                 const std::map<std::string, std::string>& headers) {
  braveledger_bat_helper::GRANT properties;

  ledger_->LogResponse(__func__, success, response, headers);

  if (!success) {
    ledger_->OnGrant(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  bool ok = braveledger_bat_helper::loadFromJson(properties, response);

  if (!ok) {
    ledger_->OnGrant(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  state_->last_grant_fetch_stamp_ = time(0);

  state_->grant_ = properties;
  saveState();
  ledger_->OnGrant(ledger::Result::LEDGER_OK, properties);
}

uint64_t BatClient::getLastGrantLoadTimestamp() const {
  return state_->last_grant_fetch_stamp_;
}

void BatClient::setGrant(const std::string& captchaResponse, const std::string& promotionId) {
  if (promotionId.empty() && state_->grant_.promotionId.empty()) {
    braveledger_bat_helper::GRANT properties;
    ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  std::string promoId = state_->grant_.promotionId;
  if (!promotionId.empty()) {
    promoId = promotionId;
  }

  std::string keys[2] = {"promotionId", "captchaResponse"};
  std::string values[2] = {promoId, captchaResponse};
  std::string payload = braveledger_bat_helper::stringify(keys, values, 2);

  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL((std::string)GET_SET_PROMOTION + "/" + state_->walletInfo_.paymentId_, PREFIX_V2),
       std::vector<std::string>(), payload, "application/json; charset=utf-8", ledger::URL_METHOD::PUT, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::setGrantCallback,
                                       this,
                                       _1,
                                       _2,
                                       _3));
}

void BatClient::setGrantCallback(bool success,
                                 const std::string& response,
                                 const std::map<std::string, std::string>& headers) {
  std::string error;
  unsigned int statusCode;
  braveledger_bat_helper::GRANT grant;
  braveledger_bat_helper::getJSONResponse(response, statusCode, error);

  ledger_->LogResponse(__func__, success, response, headers);

  if (!success) {
    if (statusCode == 403) {
      ledger_->OnGrantFinish(ledger::Result::CAPTCHA_FAILED, grant);
    } else if (statusCode == 404 || statusCode == 410) {
      ledger_->OnGrantFinish(ledger::Result::GRANT_NOT_FOUND, grant);
    } else {
      ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, grant);
    }
    return;
  }

  bool ok = braveledger_bat_helper::loadFromJson(grant, response);
  if (!ok) {
    ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, grant);
    return;
  }

  grant.promotionId = state_->grant_.promotionId;
  state_->grant_ = grant;

  ledger_->OnGrantFinish(ledger::Result::LEDGER_OK, grant);
}

void BatClient::getGrantCaptcha() {
  std::vector<std::string> headers;
  headers.push_back("brave-product:brave-core");
  auto request_id = ledger_->LoadURL(braveledger_bat_helper::buildURL((std::string)GET_PROMOTION_CAPTCHA + state_->walletInfo_.paymentId_, PREFIX_V2),
   headers, "", "",
      ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::getGrantCaptchaCallback,
                                       this,
                                       _1,
                                       _2,
                                       _3));
}

void BatClient::getGrantCaptchaCallback(bool success,
                                        const std::string& response,
                                        const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, success, response, headers);

  auto it = headers.find("captcha-hint");
  if (!success || it == headers.end()) {
    // TODO NZ Add error handler
    return;
  }

  ledger_->OnGrantCaptcha(response, it->second);
}

bool BatClient::isWalletCreated() const {
  return state_->bootStamp_ != 0u;
}

double BatClient::getBalance() const {
  return state_->walletProperties_.balance_;
}

}  // namespace braveledger_bat_client
