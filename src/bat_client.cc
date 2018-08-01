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
      state_(new braveledger_bat_helper::CLIENT_STATE_ST()),
      currentReconcile_(new braveledger_bat_helper::CURRENT_RECONCILE) {
  // Enable emscripten calls
  //braveledger_bat_helper::readEmscripten();
  initAnonize();
}

BatClient::~BatClient() {
}

std::string BatClient::buildURL(const std::string& path, const std::string& prefix, const bool isBalance = false) {
  std::string url;
  if (isBalance) {
    if (braveledger_ledger::g_isProduction) {
      url = BALANCE_PRODUCTION_SERVER;
    } else {
      url = BALANCE_STAGING_SERVER;
    }
  } else {
    if (braveledger_ledger::g_isProduction) {
      url = LEDGER_PRODUCTION_SERVER;
    } else {
      url = LEDGER_STAGING_SERVER;
    }
  }

  return url + prefix + path;
}

void BatClient::loadStateOrRegisterPersonaCallback(bool success, const std::string& data) {
  if (!success) {
    LOG(ERROR) << "!!!here1";
    registerPersona();
    return;
  }

  braveledger_bat_helper::CLIENT_STATE_ST state;
  braveledger_bat_helper::loadFromJson(state, data.c_str());

  LOG(ERROR) << "!!!bat address == " << state.walletInfo_.addressBAT_;
  LOG(ERROR) << "!!!card address == " << state.walletInfo_.addressCARD_ID_;

  state_.reset(new braveledger_bat_helper::CLIENT_STATE_ST(state));
  ledger_->OnWalletCreated(ledger::Result::OK);
}

void BatClient::registerPersona() {
  auto request_id = ledger_->LoadURL(buildURL(REGISTER_PERSONA, PREFIX_V2),
      std::vector<std::string>(), "", "",
      ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::requestCredentialsCallback,
                                       this,
                                       _1,
                                       _2));
}

void BatClient::requestCredentialsCallback(bool result, const std::string& response) {
  //LOG(ERROR) << "!!!response == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }
  if (state_->personaId_.empty()) {
    state_->personaId_ = ledger_->GenerateGUID();
  }
  // Anonize2 limit is 31 octets
  state_->userId_ = state_->personaId_;
  state_->userId_.erase(std::remove(state_->userId_.begin(), state_->userId_.end(), '-'), state_->userId_.end());
  state_->userId_.erase(12, 1);

  braveledger_bat_helper::getJSONValue(REGISTRARVK_FIELDNAME, response, state_->registrarVK_);
  DCHECK(!state_->registrarVK_.empty());
  std::string proof = getAnonizeProof(state_->registrarVK_, state_->userId_, state_->preFlight_);

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
  //LOG(ERROR) << "!!!payloadStringify == " << payloadStringify;
  std::vector<std::string> headers;
  headers.push_back("Content-Type: application/json; charset=UTF-8");

  // We should use simple callbacks on iOS
  auto request_id = ledger_->LoadURL(
    buildURL((std::string)REGISTER_PERSONA + "/" + state_->userId_, PREFIX_V2),
    headers, payloadStringify, "application/json; charset=utf-8",
    ledger::URL_METHOD::POST, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::registerPersonaCallback,
                                       this,
                                       _1,
                                       _2));
}

std::string BatClient::getAnonizeProof(const std::string& registrarVK, const std::string& id, std::string& preFlight) {
  const char* cred = makeCred(id.c_str());
  if (nullptr != cred) {
    preFlight = cred;
    free((void*)cred);
  }
  DCHECK(!preFlight.empty());
  const char* proofTemp = registerUserMessage(preFlight.c_str(), registrarVK.c_str());
  std::string proof;
  if (nullptr != proofTemp) {
    proof = proofTemp;
    free((void*)proofTemp);
  }
  DCHECK(!proof.empty());

  return proof;
}

void BatClient::registerPersonaCallback(bool result,
                                       const std::string& response) {
  if (!result) {
    // TODO error handling
    ledger_->OnWalletCreated(ledger::Result::ERROR);
    return;
  }

  std::string verification;
  braveledger_bat_helper::getJSONValue(VERIFICATION_FIELDNAME, response, verification);
  const char* masterUserToken = registerUserFinal(state_->userId_.c_str(), verification.c_str(),
    state_->preFlight_.c_str(), state_->registrarVK_.c_str());
  if (nullptr != masterUserToken) {
    state_->masterUserToken_ = masterUserToken;
    free((void*)masterUserToken);
  }

  LOG(ERROR) << "!!!registerPersonaCallback response == " << response;
  braveledger_bat_helper::getJSONWalletInfo(response, state_->walletInfo_, state_->fee_currency_, state_->fee_amount_, state_->days_);
  state_->bootStamp_ = braveledger_bat_helper::currentTime() * 1000;
  state_->reconcileStamp_ = state_->bootStamp_ + state_->days_ * 24 * 60 * 60 * 1000;

  // TODO debug
  //getPromotionCaptcha();
  //
  saveState();
  ledger_->OnWalletCreated(ledger::Result::OK);
}

void BatClient::setContributionAmount(const double& amount) {
  state_->fee_amount_ = amount;
  saveState();
}

double BatClient::getContributionAmount() const {
  return state_->fee_amount_;
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

void BatClient::getWalletProperties() {
  std::string path = (std::string)WALLET_PROPERTIES + state_->walletInfo_.paymentId_ + WALLET_PROPERTIES_END;
   auto request_id = ledger_->LoadURL(
       buildURL(path, PREFIX_V2, true),
       std::vector<std::string>(),
       "",
       "",
       ledger::URL_METHOD::GET,
       &handler_);

   handler_.AddRequestHandler(std::move(request_id),
                              std::bind(&BatClient::walletPropertiesCallback,
                                        this,
                                        _1,
                                        _2));
 }

 void BatClient::walletPropertiesCallback(bool success,
                                           const std::string& response) {
   if (!success) {
     LOG(ERROR) << "walletPropertiesCallback error";
     return;
   }

   braveledger_bat_helper::WALLET_PROPERTIES_ST properties;
   braveledger_bat_helper::loadFromJson(properties, response);
   state_->walletProperties_ = properties;
   ledger_->OnWalletProperties(properties);
 }

bool BatClient::isReadyForReconcile() {
  // TODO real check of reconcile timestamp
  return true;
}

void BatClient::reconcile(const std::string& viewingId) {
  currentReconcile_->viewingId_ = viewingId;

  auto request_id = ledger_->LoadURL(buildURL((std::string)RECONCILE_CONTRIBUTION + state_->userId_, PREFIX_V2),
      std::vector<std::string>(), "", "",
      ledger::URL_METHOD::GET,
      &handler_);

  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::reconcileCallback,
                                       this,
                                       _1,
                                       _2));
}

void BatClient::reconcileCallback(bool result, const std::string& response) {
  LOG(ERROR) << "!!!reconcileCallback response == " + response;
  if (!result) {
    // TODO errors handling
    return;
  }
  //currentReconcile_->viewingId_ = extraData.string1;
  braveledger_bat_helper::getJSONValue(SURVEYOR_ID, response, currentReconcile_->surveyorInfo_.surveyorId_);
  currentReconcile();
}

void BatClient::currentReconcile() {
  std::ostringstream amount;
  amount << state_->fee_amount_;
  std::string path = (std::string)WALLET_PROPERTIES + state_->walletInfo_.paymentId_ + "?refresh=true&amount=" + amount.str() + "&altcurrency=" + state_->fee_currency_;

  auto request_id = ledger_->LoadURL(buildURL(path, ""),
      std::vector<std::string>(), "", "",
      ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::currentReconcileCallback,
                                       this,
                                       _1,
                                       _2));
}

void BatClient::currentReconcileCallback(bool result, const std::string& response) {
  //LOG(ERROR) << "!!!currentReconcileCallback response == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }

  braveledger_bat_helper::getJSONRates(response, currentReconcile_->rates_);
  //LOG(ERROR) << "!!!rates == " << currentReconcile_->rates_.size();
  braveledger_bat_helper::UNSIGNED_TX unsignedTx;
  braveledger_bat_helper::getJSONUnsignedTx(response, unsignedTx);
  if (unsignedTx.amount_.empty() && unsignedTx.currency_.empty() && unsignedTx.destination_.empty()) {
    // We don't have any unsigned transactions
    return;
  }
  currentReconcile_->amount_ = unsignedTx.amount_;
  currentReconcile_->currency_ = unsignedTx.currency_;

  //std::string keysDenomination[2] = {"amount", "currency"};
  //std::string valuesDenomination[2] = {unsignedTx.amount_, unsignedTx.currency_};
  //std::string denomination = stringify(keysDenomination, valuesDenomination, 2);
  //std::string keys[2] = {"denomination", "destination"};
  //std::string values[2] = {denomination, unsignedTx.destination_};
  std::string octets = braveledger_bat_helper::stringifyUnsignedTx(unsignedTx);//stringify(keys, values, 2);
  //LOG(ERROR) << "!!!octets == " << octets;
  std::string headerDigest = "SHA-256=" + braveledger_bat_helper::getBase64(braveledger_bat_helper::getSHA256(octets));
  std::string headerKeys[1] = {"digest"};
  std::string headerValues[1] = {headerDigest};
  std::vector<uint8_t> secretKey = braveledger_bat_helper::getHKDF(state_->walletInfo_.keyInfoSeed_);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  braveledger_bat_helper::getPublicKeyFromSeed(secretKey, publicKey, newSecretKey);
  //LOG(ERROR) << "!!!state_->walletInfo_.keyInfoSeed_.size == " << state_->walletInfo_.keyInfoSeed_.size();
  //LOG(ERROR) << "!!!secretKey.size == " << secretKey.size();
  //LOG(ERROR) << "!!!newSecretKey.size == " << newSecretKey.size();
  std::string headerSignature = braveledger_bat_helper::sign(headerKeys, headerValues, 1, "primary", newSecretKey);
  //LOG(ERROR) << "!!!headerSignature == " << headerSignature;

  braveledger_bat_helper::RECONCILE_PAYLOAD_ST reconcilePayload;
  reconcilePayload.requestType_ = "httpSignature";
  reconcilePayload.request_signedtx_headers_digest_ = headerDigest;
  reconcilePayload.request_signedtx_headers_signature_ = headerSignature;
  reconcilePayload.request_signedtx_body_ = unsignedTx;
  reconcilePayload.request_signedtx_octets_ = octets;
  reconcilePayload.request_viewingId_ = currentReconcile_->viewingId_;
  reconcilePayload.request_surveyorId_ = currentReconcile_->surveyorInfo_.surveyorId_;
  std::string payloadStringify = braveledger_bat_helper::stringifyReconcilePayloadSt(reconcilePayload);
  //LOG(ERROR) << "!!!payloadStringify == " << payloadStringify;

  std::vector<std::string> headers;
  headers.push_back("Content-Type: application/json; charset=UTF-8");
  std::string path = (std::string)WALLET_PROPERTIES + state_->walletInfo_.paymentId_;

  auto request_id = ledger_->LoadURL(buildURL(path, ""),
    headers, payloadStringify, "application/json; charset=utf-8",
    ledger::URL_METHOD::PUT,
    &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::reconcilePayloadCallback,
                                       this,
                                       _1,
                                       _2));
}

void BatClient::reconcilePayloadCallback(bool result, const std::string& response) {
  //LOG(ERROR) << "!!!response reconcilePayloadCallback == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }
  braveledger_bat_helper::TRANSACTION_ST transaction;
  braveledger_bat_helper::getJSONTransaction(response, transaction);
  transaction.viewingId_ = currentReconcile_->viewingId_;
  transaction.surveyorId_ = currentReconcile_->surveyorInfo_.surveyorId_;
  transaction.contribution_rates_ = currentReconcile_->rates_;
  transaction.contribution_fiat_amount_ = currentReconcile_->amount_;
  transaction.contribution_fiat_currency_ = currentReconcile_->currency_;

  state_->transactions_.push_back(transaction);
  saveState();
  // TODO set a new timestamp for the next reconcile
  // TODO self.state.updateStamp var in old lib
  // TODO do we need to call update rules v1 at all?
  registerViewing();
  //
  /*
  auto request_id = ledger_->LoadURL(buildURL(UPDATE_RULES_V1, ""),
    std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::updateRulesCallback,
                                       this,
                                       true,
                                       _1,
                                       _2,
                                       stExtraData));*/
}

void BatClient::updateRulesCallback(bool reconcile, bool result, const std::string& response) {
  LOG(ERROR) << "!!!response updateRulesCallback == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }
  state_->ruleset_ = response;

  auto request_id = ledger_->LoadURL(buildURL(UPDATE_RULES_V2, ""),
    std::vector<std::string>(), "", "",
    ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::updateRulesV2Callback,
                                       this,
                                       reconcile,
                                       _1,
                                       _2));
}

void BatClient::updateRulesV2Callback(bool reconcile, bool result, const std::string& response) {
  LOG(ERROR) << "!!!response updateRulesV2Callback == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }
  // TODO parse the return rulesetV2
  state_->rulesetV2_ = response;
  // We are doing a reconcile if it is true
  if (reconcile) {
    // Register viewingId
    registerViewing();
  }
}

void BatClient::registerViewing() {
  auto request_id = ledger_->LoadURL(buildURL((std::string)REGISTER_VIEWING, PREFIX_V2),
    std::vector<std::string>(), "", "",
    ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::registerViewingCallback,
                                       this,
                                       _1,
                                       _2));
}

void BatClient::registerViewingCallback(bool result, const std::string& response) {
  LOG(ERROR) << "!!!response registerViewingCallback == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }

  braveledger_bat_helper::getJSONValue(REGISTRARVK_FIELDNAME, response, currentReconcile_->registrarVK_);
  DCHECK(!currentReconcile_->registrarVK_.empty());
  currentReconcile_->anonizeViewingId_ = currentReconcile_->viewingId_;
  currentReconcile_->anonizeViewingId_.erase(std::remove(currentReconcile_->anonizeViewingId_.begin(), currentReconcile_->anonizeViewingId_.end(), '-'), currentReconcile_->anonizeViewingId_.end());
  currentReconcile_->anonizeViewingId_.erase(12, 1);
  std::string proof = getAnonizeProof(currentReconcile_->registrarVK_, currentReconcile_->anonizeViewingId_, currentReconcile_->preFlight_);
  //LOG(ERROR) << "!!!proof1 == " << proof;

  std::string keys[1] = {"proof"};
  std::string values[1] = {proof};
  std::string proofStringified = braveledger_bat_helper::stringify(keys, values, 1);
  viewingCredentials(proofStringified, currentReconcile_->anonizeViewingId_);
}

void BatClient::viewingCredentials(const std::string& proofStringified, const std::string& anonizeViewingId) {
  auto request_id = ledger_->LoadURL(buildURL((std::string)REGISTER_VIEWING + "/" + anonizeViewingId, PREFIX_V2),
    std::vector<std::string>(), proofStringified, "application/json; charset=utf-8",
    ledger::URL_METHOD::POST, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::viewingCredentialsCallback,
                                       this,
                                       _1,
                                       _2));
}

void BatClient::viewingCredentialsCallback(bool result, const std::string& response) {
  //LOG(ERROR) << "!!!response viewingCredentialsCallback == " << response;
  if (!result) {
    ledger_->OnReconcileComplete(ledger::Result::ERROR, currentReconcile_->viewingId_);
    // TODO errors handling
    return;
  }

  std::string verification;
  braveledger_bat_helper::getJSONValue(VERIFICATION_FIELDNAME, response, verification);
  //LOG(ERROR) << "!!!response verification == " << verification;
  const char* masterUserToken = registerUserFinal(currentReconcile_->anonizeViewingId_.c_str(), verification.c_str(),
    currentReconcile_->preFlight_.c_str(), currentReconcile_->registrarVK_.c_str());

  if (nullptr != masterUserToken) {
    currentReconcile_->masterUserToken_ = masterUserToken;
    free((void*)masterUserToken);
  }

  std::vector<std::string> surveyors;
  braveledger_bat_helper::getJSONList(SURVEYOR_IDS, response, surveyors);
  // Save the rest values to transactions
  for (size_t i = 0; i < state_->transactions_.size(); i++) {
    if (state_->transactions_[i].viewingId_ != currentReconcile_->viewingId_) {
      continue;
    }
    state_->transactions_[i].anonizeViewingId_ = currentReconcile_->anonizeViewingId_;
    state_->transactions_[i].registrarVK_ = currentReconcile_->registrarVK_;
    state_->transactions_[i].masterUserToken_ = currentReconcile_->masterUserToken_;
    state_->transactions_[i].surveyorIds_ = surveyors;
  }

  saveState();
  ledger_->OnReconcileComplete(ledger::Result::OK, currentReconcile_->viewingId_);
  //LOG(ERROR) << "!!!response masterUserToken == " << currentReconcile_.masterUserToken_;
}

unsigned int BatClient::ballots(const std::string& viewingId) {
  unsigned int count = 0;
  for (size_t i = 0; i < state_->transactions_.size(); i++) {
    if (state_->transactions_[i].votes_ < state_->transactions_[i].surveyorIds_.size()
        && (state_->transactions_[i].viewingId_ == viewingId || 0 == viewingId.length())) {
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
  //LOG(ERROR) << "!!!prepared ballout " << publisher << ", votes == " << state_->transactions_[i].votes_;
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
          //prepareBallot(state_->ballots_[i], state_->transactions_[j]);
          breakTheLoop = true;
          break;
        }
        //BATCH_PROOF batchProofEl;
        //batchProofEl.transaction_ = state_->transactions_[j];
        //batchProofEl.ballot_ = state_->ballots_[i];
        //batchProof.push_back(batchProofEl);
      }
    }
    if (breakTheLoop) {
      break;
    }
  }

  //proofBatch(batchProof);
  //LOG(ERROR) << "!!! 1 batchProof.size() == " << batchProof.size();
}

void BatClient::prepareBatch(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction) {
  auto request_id = ledger_->LoadURL(buildURL((std::string)SURVEYOR_BATCH_VOTING + "/" + transaction.anonizeViewingId_, PREFIX_V2),
    std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::prepareBatchCallback,
                                       this,
                                       _1,
                                       _2));
}

void BatClient::prepareBatchCallback(bool result, const std::string& response) {
  //LOG(ERROR) << "!!!!prepareBatchCallback response == " << response;
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
  proofBatch(batchProof);
  prepareVoteBatch();
  voteBatch();
}

void BatClient::proofBatch(const std::vector<braveledger_bat_helper::BATCH_PROOF>& batchProof) {
  for (size_t i = 0; i < batchProof.size(); i++) {
    braveledger_bat_helper::SURVEYOR_ST surveyor;
    braveledger_bat_helper::loadFromJson(surveyor, batchProof[i].ballot_.prepareBallot_);

    std::string surveyorIdEncoded;
    braveledger_bat_helper::encodeURIComponent(surveyor.surveyorId_, surveyorIdEncoded);

    std::string signatureToSend;
    size_t delimeterPos = surveyor.signature_.find(',');
    if (std::string::npos != delimeterPos && delimeterPos + 1 <= surveyor.signature_.length()) {
      signatureToSend = surveyor.signature_.substr(delimeterPos + 1);
      if (signatureToSend.length() > 1 && signatureToSend[0] == ' ') {
        signatureToSend.erase(0, 1);
      }
    }

    //LOG(ERROR) << "!!!result signature == " << signatureToSend;
    std::string keysMsg[1] = {"publisher"};
    std::string valuesMsg[1] = {batchProof[i].ballot_.publisher_};
    std::string msg = braveledger_bat_helper::stringify(keysMsg, valuesMsg, 1);

    const char* proof = submitMessage(msg.c_str(), batchProof[i].transaction_.masterUserToken_.c_str(),
      batchProof[i].transaction_.registrarVK_.c_str(), signatureToSend.c_str(), surveyor.surveyorId_.c_str(), surveyor.surveyVK_.c_str());
    //LOG(ERROR) << "!!!proof == " << proof;
    std::string anonProof;
    if (nullptr != proof) {
      anonProof = proof;
      free((void*)proof);
    }

    for (size_t j = 0; j < state_->ballots_.size(); j++) {
      if (state_->ballots_[j].surveyorId_ == batchProof[i].ballot_.surveyorId_) {
        state_->ballots_[j].proofBallot_ = anonProof;
      }
    }
  }
  // TODO make wait and call prepareVoteBatch after that only
  // const delayTime = random.randomInt({ min: 10 * msecs.second, max: 1 * msecs.minute })
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

  // TODO make wait and call voteBatch after that only
  // const delayTime = random.randomInt({ min: 10 * msecs.second, max: 1 * msecs.minute })
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
  //std::string keysMsg[1] = {"payload"};
  //std::string valuesMsg[1] = {braveledger_bat_helper::stringifyBatch(voteBatch)};
  std::string payload = braveledger_bat_helper::stringifyBatch(voteBatch);//braveledger_bat_helper::stringify(keysMsg, valuesMsg, 1);

  auto request_id = ledger_->LoadURL(
    buildURL((std::string)SURVEYOR_BATCH_VOTING , PREFIX_V2),
    std::vector<std::string>(), payload, "application/json; charset=utf-8", ledger::URL_METHOD::POST, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::voteBatchCallback,
                                       this,
                                       batchVotes.publisher_,
                                       _1,
                                       _2));
  // TODO make wait and call voteBatch again after that only
  // const delayTime = random.randomInt({ min: 10 * msecs.second, max: 1 * msecs.minute })
}

void BatClient::voteBatchCallback(const std::string& publisher, bool result, const std::string& response) {
  //LOG(ERROR) << "!!!voteBatchCallback response == " << response;
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
}

// void BatClient::prepareBallot(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction) {
//   std::string surveyorIdEncoded;
//   braveledger_bat_helper::encodeURIComponent(ballot.surveyorId_, surveyorIdEncoded);

//   braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST extraData;
//   extraData.string1 = ballot.viewingId_;
//   extraData.string2 = ballot.surveyorId_;

//   auto request_id = ledger_->LoadURL(buildURL((std::string)SURVEYOR_VOTING + surveyorIdEncoded + "/" + transaction.anonizeViewingId_, PREFIX_V2),
//     std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
//   handler_.AddRequestHandler(std::move(request_id),
//                              std::bind(&BatClient::prepareBallotCallback,
//                                        this,
//                                        _1,
//                                        _2,
//                                        extraData));
// }

// void BatClient::prepareBallotCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
//   LOG(ERROR) << "!!!!prepareBallotCallback response == " << response;
//   for (int i = state_->ballots_.size() - 1; i >= 0; i--) {
//     if (state_->ballots_[i].viewingId_ == extraData.string1
//         && state_->ballots_[i].surveyorId_ == extraData.string2) {
//       state_->ballots_[i].prepareBallot_ = response;
//       // TODO make random from 1 second to 3 hours.
//       state_->ballots_[i].delayStamp_ = braveledger_bat_helper::currentTime() * 1000;
//       // TODO debug, just calling commitBallot here for testing purposes
//       for (size_t j = 0; j < state_->transactions_.size(); j++) {
//         if (state_->transactions_[j].viewingId_ == state_->ballots_[i].viewingId_) {
//           commitBallot(state_->ballots_[i], state_->transactions_[j]);
//         }
//       }
//       break;
//     }
//   }
//   saveState();
// }

// void BatClient::commitBallot(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction) {
//   braveledger_bat_helper::SURVEYOR_ST surveyor;
//   braveledger_bat_helper::loadFromJson(surveyor, ballot.prepareBallot_);

//   std::string surveyorIdEncoded;
//   braveledger_bat_helper::encodeURIComponent(surveyor.surveyorId_, surveyorIdEncoded);

//   std::string signatureToSend;
//   //LOG(ERROR) << "!!!full signature == " << surveyor.signature_;
//   size_t delimeterPos = surveyor.signature_.find(',');
//   if (std::string::npos != delimeterPos && delimeterPos + 1 <= surveyor.signature_.length()) {
//     signatureToSend = surveyor.signature_.substr(delimeterPos + 1);
//     //LOG(ERROR) << "!!!signatureToSend == " << signatureToSend;
//     if (signatureToSend.length() > 1 && signatureToSend[0] == ' ') {
//       signatureToSend.erase(0, 1);
//     }
//   }
//   //LOG(ERROR) << "!!!result signature == " << signatureToSend;
//   std::string keysMsg[1] = {"publisher"};
//   std::string valuesMsg[1] = {ballot.publisher_};
//   std::string msg = braveledger_bat_helper::stringify(keysMsg, valuesMsg, 1);

//   const char* proof = submitMessage(msg.c_str(), transaction.masterUserToken_.c_str(),
//     transaction.registrarVK_.c_str(), signatureToSend.c_str(), surveyor.surveyorId_.c_str(), surveyor.surveyVK_.c_str());
//   //LOG(ERROR) << "!!!proof == " << proof;
//   std::string anonProof;
//   if (nullptr != proof) {
//     anonProof = proof;
//     free((void*)proof);
//   }

//   std::string keys[1] = {"proof"};
//   std::string values[1] = {anonProof};
//   std::string payload = braveledger_bat_helper::stringify(keys, values, 1);

//   auto request_id = ledger_->LoadURL(buildURL((std::string)SURVEYOR_VOTING + surveyorIdEncoded, PREFIX_V2),
//     std::vector<std::string>(), payload, "", ledger::URL_METHOD::PUT, &handler_);
//   handler_.AddRequestHandler(std::move(request_id),
//                              std::bind(&BatClient::commitBallotCallback,
//                                        this,
//                                        _1,
//                                        _2));
// }

// void BatClient::commitBallotCallback(bool result, const std::string& response) {
//   LOG(ERROR) << "!!!!commitBallotCallback response == " << response;

//   // TODO add ballots to transaction, saveState, remove ballots from vector of ballots
// }

std::string BatClient::getWalletPassphrase() {
  DCHECK(state_->walletInfo_.keyInfoSeed_.size());
  std::string passPhrase;
  if (0 == state_->walletInfo_.keyInfoSeed_.size()) {
    return passPhrase;
  }
    char* words = nullptr;
  int result = bip39_mnemonic_from_bytes(nullptr, &state_->walletInfo_.keyInfoSeed_.front(),
    state_->walletInfo_.keyInfoSeed_.size(), &words);
  LOG(ERROR) << "!!!getWalletPassphrase result == " << result << ", !!!words == " << words;
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
  ledger_->SaveLedgerState(data, this);
}

void BatClient::recoverWallet(const std::string& passPhrase) {
  std::vector<unsigned char> newSeed;
  newSeed.resize(32);
  size_t written = 0;
  int result = bip39_mnemonic_to_bytes(nullptr, passPhrase.c_str(), &newSeed.front(), newSeed.size(), &written);
  LOG(ERROR) << "!!!recoverWallet result == " << result << "!!!result size == " << written;
  if (0 != result || 0 == written) {
    DCHECK(false);

    return;
  }
  state_->walletInfo_.keyInfoSeed_ = newSeed;

  std::vector<uint8_t> secretKey = braveledger_bat_helper::getHKDF(state_->walletInfo_.keyInfoSeed_);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  braveledger_bat_helper::getPublicKeyFromSeed(secretKey, publicKey, newSecretKey);
  std::string publicKeyHex = braveledger_bat_helper::uint8ToHex(publicKey);

  auto request_id = ledger_->LoadURL(buildURL((std::string)RECOVER_WALLET_PUBLIC_KEY + publicKeyHex, ""),
    std::vector<std::string>(), "", "",
    ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::recoverWalletPublicKeyCallback,
                                       this,
                                       _1,
                                       _2));

}

void BatClient::recoverWalletPublicKeyCallback(bool result, const std::string& response) {
  //LOG(ERROR) << "!!!recoverWalletPublicKeyCallback == " << response;
  std::string recoveryId;
  braveledger_bat_helper::getJSONValue("paymentId", response, recoveryId);

  auto request_id = ledger_->LoadURL(buildURL((std::string)RECOVER_WALLET + recoveryId, ""),
    std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::recoverWalletCallback,
                                       this,
                                       _1,
                                       _2));
}

void BatClient::recoverWalletCallback(bool result, const std::string& response) {
  //LOG(ERROR) << "!!!recoverWalletCallback == " << response;
  braveledger_bat_helper::getJSONWalletInfo(response, state_->walletInfo_, state_->fee_currency_, state_->fee_amount_, state_->days_);
  saveState();
}

void BatClient::getPromotion(const std::string& lang, const std::string& forPaymentId) {
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

  auto request_id = ledger_->LoadURL(buildURL((std::string)GET_SET_PROMOTION + arguments, ""),
    std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::getPromotionCallback,
                                       this,
                                       _1,
                                       _2));
}

void BatClient::getPromotionCallback(bool result, const std::string& response) {
  LOG(ERROR) << "!!!getPromotionCallback == " << response;
}

void BatClient::setPromotion(const std::string& promotionId, const std::string& captchaResponse) {
  std::string keys[2] = {"promotionId", "captchaResponse"};
  std::string values[2] = {promotionId, captchaResponse};
  std::string payload = braveledger_bat_helper::stringify(keys, values, 2);

  auto request_id = ledger_->LoadURL(buildURL((std::string)GET_SET_PROMOTION + "/" + state_->walletInfo_.paymentId_, ""),
    std::vector<std::string>(), payload, "", ledger::URL_METHOD::PUT, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::setPromotionCallback,
                                       this,
                                       _1,
                                       _2));
}

void BatClient::setPromotionCallback(bool result, const std::string& response) {
  LOG(ERROR) << "!!!setPromotionCallback == " << response;
}

void BatClient::getPromotionCaptcha() {
  auto request_id = ledger_->LoadURL(buildURL((std::string)GET_PROMOTION_CAPTCHA + state_->walletInfo_.paymentId_, ""),
    std::vector<std::string>(), "", "",
      ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatClient::getPromotionCaptchaCallback,
                                       this,
                                       _1,
                                       _2));
}

void BatClient::getPromotionCaptchaCallback(bool result, const std::string& response) {
  LOG(ERROR) << "!!!getPromotionCaptchaCallback == " << response;
}

}  // namespace braveledger_bat_client
