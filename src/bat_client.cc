/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_client.h"
#include "static_values.h"
#include <algorithm>
#include "bat_helper.h"
#include "base/bind.h"
#include "base/guid.h"
#include "url_util.h"
#include "url_canon_stdstring.h"

#include "anon.h"
#include "wally_bip39.h"

namespace bat_client {

BatClient::BatClient(const bool& useProxy):
      useProxy_(useProxy),
      publisherTimestamp_(0) {
  // Enable emscripten calls
  //BatHelper::readEmscripten();
  //
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
    LOG(ERROR) << "!!!here1";
    registerPersona();

    return;
  }
  LOG(ERROR) << "!!!bat address == " << state.walletInfo_.addressBAT_;
  LOG(ERROR) << "!!!card address == " << state.walletInfo_.addressCARD_ID_;
  state_ = state;
  publisherTimestamp(false);
}

void BatClient::registerPersona() {
  // We should use simple callbacks on iOS
  batClientWebRequest_.run(buildURL(REGISTER_PERSONA, PREFIX_V2),
    base::Bind(&BatClient::requestCredentialsCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", FETCH_CALLBACK_EXTRA_DATA_ST(),
      URL_METHOD::GET);
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
  std::string proof = getAnonizeProof(state_.registrarVK_, state_.userId_, state_.preFlight_);

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
  //LOG(ERROR) << "!!!payloadStringify == " << payloadStringify;
  std::vector<std::string> headers;
  headers.push_back("Content-Type: application/json; charset=UTF-8");
  // We should use simple callbacks on iOS
  batClientWebRequest_.run(buildURL((std::string)REGISTER_PERSONA + "/" + state_.userId_, PREFIX_V2),
    base::Bind(&BatClient::registerPersonaCallback,
      base::Unretained(this)), headers, payloadStringify, "application/json; charset=utf-8",
      FETCH_CALLBACK_EXTRA_DATA_ST(), URL_METHOD::POST);
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

void BatClient::registerPersonaCallback(bool result, const std::string& response,
    const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  if (!result) {
    // TODO errors handling
    return;
  }

  std::string verification = BatHelper::getJSONValue(VERIFICATION_FIELDNAME, response);
  const char* masterUserToken = registerUserFinal(state_.userId_.c_str(), verification.c_str(),
    state_.preFlight_.c_str(), state_.registrarVK_.c_str());
  if (nullptr != masterUserToken) {
    state_.masterUserToken_ = masterUserToken;
    free((void*)masterUserToken);
  }

  LOG(ERROR) << "!!!registerPersonaCallback response == " << response;
  BatHelper::getJSONWalletInfo(response, state_.walletInfo_, state_.fee_currency_, state_.fee_amount_, state_.days_);
  state_.bootStamp_ = BatHelper::currentTime() * 1000;
  state_.reconcileStamp_ = state_.bootStamp_ + state_.days_ * 24 * 60 * 60 * 1000;
  publisherTimestamp();

  // TODO debug
  //getPromotionCaptcha();
  //
}

void BatClient::publisherTimestamp(const bool& saveState) {
  // We should use simple callbacks on iOS
  FETCH_CALLBACK_EXTRA_DATA_ST extraData;
  extraData.boolean1 = saveState;
  batClientWebRequest_.run(buildURL(PUBLISHER_TIMESTAMP, PREFIX_V3),
    base::Bind(&BatClient::publisherTimestampCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", extraData,
      URL_METHOD::GET);
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
    callback, std::vector<std::string>(), "", "", extraData, URL_METHOD::GET);
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

bool BatClient::isReadyForReconcile() {
  // TODO real check of reconcile timestamp
  return true;
}

void BatClient::reconcile(const std::string& viewingId, BatHelper::SimpleCallback callback) {
  //FETCH_CALLBACK_EXTRA_DATA_ST extraData;
  currentReconcile_.viewingId_ = viewingId;
  currentReconcile_.ledgerCallback_ = callback;
  batClientWebRequest_.run(buildURL((std::string)RECONCILE_CONTRIBUTION + state_.userId_, PREFIX_V2),
    base::Bind(&BatClient::reconcileCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", FETCH_CALLBACK_EXTRA_DATA_ST(),
      URL_METHOD::GET);
}

void BatClient::reconcileCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  LOG(ERROR) << "!!!reconcileCallback response == " + response;
  if (!result) {
    // TODO errors handling
    return;
  }
  //currentReconcile_.viewingId_ = extraData.string1;
  currentReconcile_.surveyorInfo_.surveyorId_ = BatHelper::getJSONValue(SURVEYOR_ID, response);

  currentReconcile();
}

void BatClient::currentReconcile() {
  std::ostringstream amount;
  amount << state_.fee_amount_;
  std::string path = (std::string)WALLET_PROPERTIES + state_.walletInfo_.paymentId_ + "?refresh=true&amount=" + amount.str() + "&altcurrency=" + state_.fee_currency_;
  //FETCH_CALLBACK_EXTRA_DATA_ST extraData;
  batClientWebRequest_.run(buildURL(path, ""),
    base::Bind(&BatClient::currentReconcileCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", FETCH_CALLBACK_EXTRA_DATA_ST(),
      URL_METHOD::GET);
}

void BatClient::currentReconcileCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  //LOG(ERROR) << "!!!currentReconcileCallback response == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }

  BatHelper::getJSONRates(response, currentReconcile_.rates_);
  //LOG(ERROR) << "!!!rates == " << currentReconcile_.rates_.size();
  UNSIGNED_TX unsignedTx;
  BatHelper::getJSONUnsignedTx(response, unsignedTx);
  if (unsignedTx.amount_.empty() && unsignedTx.currency_.empty() && unsignedTx.destination_.empty()) {
    // We don't have any unsigned transactions
    return;
  }
  currentReconcile_.amount_ = unsignedTx.amount_;
  currentReconcile_.currency_ = unsignedTx.currency_;

  //std::string keysDenomination[2] = {"amount", "currency"};
  //std::string valuesDenomination[2] = {unsignedTx.amount_, unsignedTx.currency_};
  //std::string denomination = BatHelper::stringify(keysDenomination, valuesDenomination, 2);
  //std::string keys[2] = {"denomination", "destination"};
  //std::string values[2] = {denomination, unsignedTx.destination_};
  std::string octets = BatHelper::stringifyUnsignedTx(unsignedTx);//BatHelper::stringify(keys, values, 2);
  //LOG(ERROR) << "!!!octets == " << octets;
  std::string headerDigest = "SHA-256=" + BatHelper::getBase64(BatHelper::getSHA256(octets));
  std::string headerKeys[1] = {"digest"};
  std::string headerValues[1] = {headerDigest};
  std::vector<uint8_t> secretKey = BatHelper::getHKDF(state_.walletInfo_.keyInfoSeed_);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  BatHelper::getPublicKeyFromSeed(secretKey, publicKey, newSecretKey);
  //LOG(ERROR) << "!!!state_.walletInfo_.keyInfoSeed_.size == " << state_.walletInfo_.keyInfoSeed_.size();
  //LOG(ERROR) << "!!!secretKey.size == " << secretKey.size();
  //LOG(ERROR) << "!!!newSecretKey.size == " << newSecretKey.size();
  std::string headerSignature = BatHelper::sign(headerKeys, headerValues,
    1, "primary", newSecretKey);
  //LOG(ERROR) << "!!!headerSignature == " << headerSignature;

  RECONCILE_PAYLOAD_ST reconcilePayload;
  reconcilePayload.requestType_ = "httpSignature";
  reconcilePayload.request_signedtx_headers_digest_ = headerDigest;
  reconcilePayload.request_signedtx_headers_signature_ = headerSignature;
  reconcilePayload.request_signedtx_body_ = unsignedTx;
  reconcilePayload.request_signedtx_octets_ = octets;
  reconcilePayload.request_viewingId_ = currentReconcile_.viewingId_;
  reconcilePayload.request_surveyorId_ = currentReconcile_.surveyorInfo_.surveyorId_;
  std::string payloadStringify = BatHelper::stringifyReconcilePayloadSt(reconcilePayload);
  //LOG(ERROR) << "!!!payloadStringify == " << payloadStringify;

  std::vector<std::string> headers;
  headers.push_back("Content-Type: application/json; charset=UTF-8");
  std::string path = (std::string)WALLET_PROPERTIES + state_.walletInfo_.paymentId_;
  batClientWebRequest_.run(buildURL(path, ""),
    base::Bind(&BatClient::reconcilePayloadCallback,
      base::Unretained(this)), headers, payloadStringify, "application/json; charset=utf-8",
      FETCH_CALLBACK_EXTRA_DATA_ST(), URL_METHOD::PUT);
}

void BatClient::reconcilePayloadCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  //LOG(ERROR) << "!!!response == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }
  TRANSACTION_ST transaction;
  BatHelper::getJSONTransaction(response, transaction);
  transaction.viewingId_ = currentReconcile_.viewingId_;
  transaction.surveyorId_ = currentReconcile_.surveyorInfo_.surveyorId_;
  transaction.contribution_rates_ = currentReconcile_.rates_;
  transaction.contribution_fiat_amount_ = currentReconcile_.amount_;
  transaction.contribution_fiat_currency_ = currentReconcile_.currency_;

  {
    std::lock_guard<std::mutex> guard(transactions_access_mutex_);
    state_.transactions_.push_back(transaction);
  }
  BatHelper::saveState(state_);
  // TODO set a new timestamp for the next reconcile
  // TODO self.state.updateStamp var in old lib
  FETCH_CALLBACK_EXTRA_DATA_ST stExtraData;
  stExtraData.boolean1 = true;
  batClientWebRequest_.run(buildURL(UPDATE_RULES_V1, ""),
    base::Bind(&BatClient::updateRulesCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", stExtraData,
      URL_METHOD::GET);
}

void BatClient::updateRulesCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  //LOG(ERROR) << "!!!response updateRulesCallback == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }
  state_.ruleset_ = response;

  batClientWebRequest_.run(buildURL(UPDATE_RULES_V2, ""),
    base::Bind(&BatClient::updateRulesV2Callback,
      base::Unretained(this)), std::vector<std::string>(), "", "", extraData,
      URL_METHOD::GET);
}

void BatClient::updateRulesV2Callback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  //LOG(ERROR) << "!!!response updateRulesV2Callback == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }
  // TODO parse the return rulesetV2
  state_.rulesetV2_ = response;
  // We are doing a reconcile if the boolean is true
  if (extraData.boolean1) {
    // Register viewingId
    registerViewing();
  }
}

void BatClient::registerViewing() {
  batClientWebRequest_.run(buildURL((std::string)REGISTER_VIEWING, PREFIX_V2),
    base::Bind(&BatClient::registerViewingCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", FETCH_CALLBACK_EXTRA_DATA_ST(),
      URL_METHOD::GET);
}

void BatClient::registerViewingCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  LOG(ERROR) << "!!!response registerViewingCallback == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }

  currentReconcile_.registrarVK_ = BatHelper::getJSONValue(REGISTRARVK_FIELDNAME, response);
  DCHECK(!currentReconcile_.registrarVK_.empty());
  currentReconcile_.anonizeViewingId_ = currentReconcile_.viewingId_;
  currentReconcile_.anonizeViewingId_.erase(std::remove(currentReconcile_.anonizeViewingId_.begin(), currentReconcile_.anonizeViewingId_.end(), '-'), currentReconcile_.anonizeViewingId_.end());
  currentReconcile_.anonizeViewingId_.erase(12, 1);
  std::string proof = getAnonizeProof(currentReconcile_.registrarVK_, currentReconcile_.anonizeViewingId_, currentReconcile_.preFlight_);
  //LOG(ERROR) << "!!!proof1 == " << proof;

  std::string keys[1] = {"proof"};
  std::string values[1] = {proof};
  std::string proofStringified = BatHelper::stringify(keys, values, 1);
  viewingCredentials(proofStringified, currentReconcile_.anonizeViewingId_);
}

void BatClient::viewingCredentials(const std::string& proofStringified, const std::string& anonizeViewingId) {
  batClientWebRequest_.run(buildURL((std::string)REGISTER_VIEWING + "/" + anonizeViewingId, PREFIX_V2),
    base::Bind(&BatClient::viewingCredentialsCallback,
      base::Unretained(this)), std::vector<std::string>(), proofStringified, "application/json; charset=utf-8", FETCH_CALLBACK_EXTRA_DATA_ST(),
      URL_METHOD::POST);
}

void BatClient::viewingCredentialsCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  //LOG(ERROR) << "!!!response viewingCredentialsCallback == " << response;
  if (!result) {
    // TODO errors handling
    return;
  }

  std::string verification = BatHelper::getJSONValue(VERIFICATION_FIELDNAME, response);
  //LOG(ERROR) << "!!!response verification == " << verification;
  const char* masterUserToken = registerUserFinal(currentReconcile_.anonizeViewingId_.c_str(), verification.c_str(),
    currentReconcile_.preFlight_.c_str(), currentReconcile_.registrarVK_.c_str());
  if (nullptr != masterUserToken) {
    currentReconcile_.masterUserToken_ = masterUserToken;
    free((void*)masterUserToken);
  }
  std::vector<std::string> surveyors = BatHelper::getJSONList(SURVEYOR_IDS, response);
  // Save the rest values to transactions
  {
    std::lock_guard<std::mutex> guard(transactions_access_mutex_);
    for (size_t i = 0; i < state_.transactions_.size(); i++) {
      if (state_.transactions_[i].viewingId_ != currentReconcile_.viewingId_) {
        continue;
      }
      state_.transactions_[i].anonizeViewingId_ = currentReconcile_.anonizeViewingId_;
      state_.transactions_[i].registrarVK_ = currentReconcile_.registrarVK_;
      state_.transactions_[i].masterUserToken_ = currentReconcile_.masterUserToken_;
      state_.transactions_[i].surveyorIds_ = surveyors;
    }
  }
  BatHelper::saveState(state_);
  currentReconcile_.ledgerCallback_.Run(currentReconcile_.viewingId_);
  //LOG(ERROR) << "!!!response masterUserToken == " << currentReconcile_.masterUserToken_;

}

unsigned int BatClient::ballots(const std::string& viewingId) {
  std::lock_guard<std::mutex> guard(transactions_access_mutex_);
  unsigned int count = 0;
  for (size_t i = 0; i < state_.transactions_.size(); i++) {
    if (state_.transactions_[i].votes_ < state_.transactions_[i].surveyorIds_.size()
        && (state_.transactions_[i].viewingId_ == viewingId || 0 == viewingId.length())) {
      count += state_.transactions_[i].surveyorIds_.size() - state_.transactions_[i].votes_;
    }
  }

  return count;
}

void BatClient::votePublishers(const std::vector<std::string>& publishers, const std::string& viewingId) {
  for (size_t i = 0; i < publishers.size(); i++) {
    vote(publishers[i], viewingId);
  }
  BatHelper::saveState(state_);
}

void BatClient::vote(const std::string& publisher, const std::string& viewingId) {
  DCHECK(!publisher.empty());
  if (publisher.empty()) {
    return;
  }
  BALLOT_ST ballot;
  {
    std::lock_guard<std::mutex> guard(transactions_access_mutex_);
    int i = 0;
    for (i = state_.transactions_.size() - 1; i >=0; i--) {
      if (state_.transactions_[i].votes_ >= state_.transactions_[i].surveyorIds_.size()) {
        continue;
      }
      if (state_.transactions_[i].viewingId_ == viewingId || viewingId.empty()) {
        break;
      }
    }
    if (i < 0) {
      return;
    }
    ballot.viewingId_ = state_.transactions_[i].viewingId_;
    ballot.surveyorId_ = state_.transactions_[i].surveyorIds_[state_.transactions_[i].votes_];
    ballot.publisher_ = publisher;
    ballot.offset_ = state_.transactions_[i].votes_;
    state_.transactions_[i].votes_++;
    //LOG(ERROR) << "!!!prepared ballout " << publisher << ", votes == " << state_.transactions_[i].votes_;
  }
  std::lock_guard<std::mutex> guard(ballots_access_mutex_);
  state_.ballots_.push_back(ballot);
}

void BatClient::prepareBallots() {
  //uint64_t currentTime = BatHelper::currentTime() * 1000;
  //std::vector<BATCH_PROOF> batchProof;
  {
    std::lock_guard<std::mutex> guard(ballots_access_mutex_);
    for (int i = state_.ballots_.size() - 1; i >= 0; i--) {
      bool breakTheLoop = false;
      std::lock_guard<std::mutex> guard(transactions_access_mutex_);
      for (size_t j = 0; j < state_.transactions_.size(); j++) {
        // TODO check on valid credentials for transaction
        if (state_.transactions_[j].viewingId_ == state_.ballots_[i].viewingId_
            /*&& (state_.ballots_[i].prepareBallot_.empty() || 0 == state_.ballots_[i].delayStamp_
              || state_.ballots_[i].delayStamp_ <= currentTime)*/) {
          // TODO check on ballot.prepareBallot and call commitBallot if it exist
          if (state_.ballots_[i].prepareBallot_.empty()) {
            prepareBatch(state_.ballots_[i], state_.transactions_[j]);
            //prepareBallot(state_.ballots_[i], state_.transactions_[j]);
            breakTheLoop = true;
            break;
          }
          //BATCH_PROOF batchProofEl;
          //batchProofEl.transaction_ = state_.transactions_[j];
          //batchProofEl.ballot_ = state_.ballots_[i];
          //batchProof.push_back(batchProofEl);
        }
      }
      if (breakTheLoop) {
        break;
      }
    }
  }

  //proofBatch(batchProof);
  //LOG(ERROR) << "!!! 1 batchProof.size() == " << batchProof.size();
}

void BatClient::prepareBatch(const BALLOT_ST& ballot, const TRANSACTION_ST& transaction) {
  FETCH_CALLBACK_EXTRA_DATA_ST extraData;
  extraData.string1 = ballot.viewingId_;
  extraData.string2 = ballot.surveyorId_;
  batClientWebRequest_.run(buildURL((std::string)SURVEYOR_BATCH_VOTING + transaction.anonizeViewingId_, PREFIX_V2),
    base::Bind(&BatClient::prepareBatchCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", extraData,
      URL_METHOD::GET);
}

void BatClient::prepareBatchCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  //LOG(ERROR) << "!!!!prepareBatchCallback response == " << response;
  std::vector<std::string> surveyors;
  BatHelper::getJSONBatchSurveyors(response, surveyors);
  std::vector<BATCH_PROOF> batchProof;
  for (size_t j = 0; j < surveyors.size(); j++) {
    std::string error = BatHelper::getJSONValue("error", surveyors[j]);
    if (!error.empty()) {
      continue;
    }
    std::lock_guard<std::mutex> guard(ballots_access_mutex_);
    for (int i = state_.ballots_.size() - 1; i >= 0; i--) {
      if (state_.ballots_[i].surveyorId_ == BatHelper::getJSONValue("surveyorId", surveyors[j])) {
        std::lock_guard<std::mutex> guard(transactions_access_mutex_);
        for (size_t k = 0; k < state_.transactions_.size(); k++) {
          if (state_.transactions_[k].viewingId_ == state_.ballots_[i].viewingId_) {
            state_.ballots_[i].prepareBallot_ = surveyors[j];
            BATCH_PROOF batchProofEl;
            batchProofEl.transaction_ = state_.transactions_[k];
            batchProofEl.ballot_ = state_.ballots_[i];
            batchProof.push_back(batchProofEl);
          }
        }
      }
    }
  }

  BatHelper::saveState(state_);
  proofBatch(batchProof);
  LOG(ERROR) << "!!! 2 batchProof.size() == " << batchProof.size();
}

void BatClient::proofBatch(const std::vector<BATCH_PROOF>& batchProof) {
  for (size_t i = 0; i < batchProof.size(); i++) {
    SURVEYOR_ST surveyor;
    BatHelper::getJSONSurveyor(batchProof[i].ballot_.prepareBallot_, surveyor);
    std::string surveyorIdEncoded;
    url::StdStringCanonOutput surveyorIdCanon(&surveyorIdEncoded);
    url::EncodeURIComponent(surveyor.surveyorId_.c_str(), surveyor.surveyorId_.length(), &surveyorIdCanon);
    surveyorIdCanon.Complete();

    std::string signatureToSend;
    LOG(ERROR) << "!!!full signature == " << surveyor.signature_;
    size_t delimeterPos = surveyor.signature_.find(',');
    if (std::string::npos != delimeterPos && delimeterPos + 1 <= surveyor.signature_.length()) {
      signatureToSend = surveyor.signature_.substr(delimeterPos + 1);
      LOG(ERROR) << "!!!signatureToSend == " << signatureToSend;
      if (signatureToSend.length() > 1 && signatureToSend[0] == ' ') {
        signatureToSend.erase(0, 1);
      }
    }

    //LOG(ERROR) << "!!!result signature == " << signatureToSend;
    std::string keysMsg[1] = {"publisher"};
    std::string valuesMsg[1] = {batchProof[i].ballot_.publisher_};
    std::string msg = BatHelper::stringify(keysMsg, valuesMsg, 1);

    const char* proof = submitMessage(msg.c_str(), batchProof[i].transaction_.masterUserToken_.c_str(),
      batchProof[i].transaction_.registrarVK_.c_str(), signatureToSend.c_str(), surveyor.surveyorId_.c_str(), surveyor.surveyVK_.c_str());
    //LOG(ERROR) << "!!!proof == " << proof;
    std::string anonProof;
    if (nullptr != proof) {
      anonProof = proof;
      free((void*)proof);
    }

    std::lock_guard<std::mutex> guard(ballots_access_mutex_);
    for (size_t j = 0; j < state_.ballots_.size(); j++) {
      if (state_.ballots_[j].surveyorId_ == batchProof[i].ballot_.surveyorId_) {
        state_.ballots_[j].proofBallot_ = anonProof;
      }
    }
  }
  // TODO make wait
  // const delayTime = random.randomInt({ min: 10 * msecs.second, max: 1 * msecs.minute })
}


void BatClient::prepareBallot(const BALLOT_ST& ballot, const TRANSACTION_ST& transaction) {
  std::string surveyorIdEncoded;
  url::StdStringCanonOutput surveyorIdCanon(&surveyorIdEncoded);
  url::EncodeURIComponent(ballot.surveyorId_.c_str(), ballot.surveyorId_.length(), &surveyorIdCanon);
  surveyorIdCanon.Complete();
  FETCH_CALLBACK_EXTRA_DATA_ST extraData;
  extraData.string1 = ballot.viewingId_;
  extraData.string2 = ballot.surveyorId_;
  batClientWebRequest_.run(buildURL((std::string)SURVEYOR_VOTING + surveyorIdEncoded + "/" + transaction.anonizeViewingId_, PREFIX_V2),
    base::Bind(&BatClient::prepareBallotCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", extraData,
      URL_METHOD::GET);
}

void BatClient::prepareBallotCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  LOG(ERROR) << "!!!!prepareBallotCallback response == " << response;
  {
    std::lock_guard<std::mutex> guard(ballots_access_mutex_);
    for (int i = state_.ballots_.size() - 1; i >= 0; i--) {
      if (state_.ballots_[i].viewingId_ == extraData.string1
          && state_.ballots_[i].surveyorId_ == extraData.string2) {
        state_.ballots_[i].prepareBallot_ = response;
        // TODO make random from 1 second to 3 hours.
        state_.ballots_[i].delayStamp_ = BatHelper::currentTime() * 1000;
        // TODO debug, just calling commitBallot here for testing purposes
        {
          std::lock_guard<std::mutex> guard(transactions_access_mutex_);
          for (size_t j = 0; j < state_.transactions_.size(); j++) {
            if (state_.transactions_[j].viewingId_ == state_.ballots_[i].viewingId_) {
              commitBallot(state_.ballots_[i], state_.transactions_[j]);
            }
          }
        }
        break;
      }
    }
  }
  BatHelper::saveState(state_);
}

void BatClient::commitBallot(const BALLOT_ST& ballot, const TRANSACTION_ST& transaction) {
  SURVEYOR_ST surveyor;
  BatHelper::getJSONSurveyor(ballot.prepareBallot_, surveyor);
  std::string surveyorIdEncoded;
  url::StdStringCanonOutput surveyorIdCanon(&surveyorIdEncoded);
  url::EncodeURIComponent(surveyor.surveyorId_.c_str(), surveyor.surveyorId_.length(), &surveyorIdCanon);
  surveyorIdCanon.Complete();

  std::string signatureToSend;
  //LOG(ERROR) << "!!!full signature == " << surveyor.signature_;
  size_t delimeterPos = surveyor.signature_.find(',');
  if (std::string::npos != delimeterPos && delimeterPos + 1 <= surveyor.signature_.length()) {
    signatureToSend = surveyor.signature_.substr(delimeterPos + 1);
    //LOG(ERROR) << "!!!signatureToSend == " << signatureToSend;
    if (signatureToSend.length() > 1 && signatureToSend[0] == ' ') {
      signatureToSend.erase(0, 1);
    }
  }
  //LOG(ERROR) << "!!!result signature == " << signatureToSend;
  std::string keysMsg[1] = {"publisher"};
  std::string valuesMsg[1] = {ballot.publisher_};
  std::string msg = BatHelper::stringify(keysMsg, valuesMsg, 1);

  const char* proof = submitMessage(msg.c_str(), transaction.masterUserToken_.c_str(),
    transaction.registrarVK_.c_str(), signatureToSend.c_str(), surveyor.surveyorId_.c_str(), surveyor.surveyVK_.c_str());
  //LOG(ERROR) << "!!!proof == " << proof;
  std::string anonProof;
  if (nullptr != proof) {
    anonProof = proof;
    free((void*)proof);
  }

  std::string keys[1] = {"proof"};
  std::string values[1] = {anonProof};
  std::string payload = BatHelper::stringify(keys, values, 1);

  FETCH_CALLBACK_EXTRA_DATA_ST extraData;
  batClientWebRequest_.run(buildURL((std::string)SURVEYOR_VOTING + surveyorIdEncoded, PREFIX_V2),
    base::Bind(&BatClient::commitBallotCallback,
      base::Unretained(this)), std::vector<std::string>(), payload, "", extraData,
      URL_METHOD::PUT);
}

void BatClient::commitBallotCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  LOG(ERROR) << "!!!!commitBallotCallback response == " << response;

  // TODO add ballots to transaction, saveState, remove ballots from vector of ballots
}

std::string BatClient::getWalletPassphrase() {
  DCHECK(state_.walletInfo_.keyInfoSeed_.size());
  std::string passPhrase;
  if (0 == state_.walletInfo_.keyInfoSeed_.size()) {
    return passPhrase;
  }
    char* words = nullptr;
  int result = bip39_mnemonic_from_bytes(nullptr, &state_.walletInfo_.keyInfoSeed_.front(),
    state_.walletInfo_.keyInfoSeed_.size(), &words);
  LOG(ERROR) << "!!!getWalletPassphrase result == " << result << ", !!!words == " << words;
  if (0 != result) {
    DCHECK(false);

    return passPhrase;
  }
  passPhrase = words;
  wally_free_string(words);

  return passPhrase;
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
  state_.walletInfo_.keyInfoSeed_ = newSeed;

  std::vector<uint8_t> secretKey = BatHelper::getHKDF(state_.walletInfo_.keyInfoSeed_);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  BatHelper::getPublicKeyFromSeed(secretKey, publicKey, newSecretKey);
  std::string publicKeyHex = BatHelper::uint8ToHex(publicKey);

  batClientWebRequest_.run(buildURL((std::string)RECOVER_WALLET_PUBLIC_KEY + publicKeyHex, ""),
    base::Bind(&BatClient::recoverWalletPublicKeyCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", FETCH_CALLBACK_EXTRA_DATA_ST(),
      URL_METHOD::GET);
}

void BatClient::recoverWalletPublicKeyCallback(bool result, const std::string& response,
    const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  //LOG(ERROR) << "!!!recoverWalletPublicKeyCallback == " << response;
  std::string recoveryId = BatHelper::getJSONValue("paymentId", response);

  batClientWebRequest_.run(buildURL((std::string)RECOVER_WALLET + recoveryId, ""),
    base::Bind(&BatClient::recoverWalletCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", FETCH_CALLBACK_EXTRA_DATA_ST(),
      URL_METHOD::GET);
}

void BatClient::recoverWalletCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  //LOG(ERROR) << "!!!recoverWalletCallback == " << response;
  BatHelper::getJSONWalletInfo(response, state_.walletInfo_, state_.fee_currency_, state_.fee_amount_, state_.days_);
  BatHelper::saveState(state_);
}

void BatClient::getPromotion(const std::string& lang, const std::string& forPaymentId) {
  std::string paymentId = forPaymentId;
  if (paymentId.empty()) {
    paymentId = state_.walletInfo_.paymentId_;
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

  batClientWebRequest_.run(buildURL((std::string)GET_SET_PROMOTION + arguments, ""),
    base::Bind(&BatClient::getPromotionCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", FETCH_CALLBACK_EXTRA_DATA_ST(),
      URL_METHOD::GET);
}

void BatClient::getPromotionCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  LOG(ERROR) << "!!!getPromotionCallback == " << response;
}

void BatClient::setPromotion(const std::string& promotionId, const std::string& captchaResponse) {
  std::string keys[2] = {"promotionId", "captchaResponse"};
  std::string values[2] = {promotionId, captchaResponse};
  std::string payload = BatHelper::stringify(keys, values, 2);

  batClientWebRequest_.run(buildURL((std::string)GET_SET_PROMOTION + "/" + state_.walletInfo_.paymentId_, ""),
    base::Bind(&BatClient::setPromotionCallback,
      base::Unretained(this)), std::vector<std::string>(), payload, "",
      FETCH_CALLBACK_EXTRA_DATA_ST(), URL_METHOD::PUT);
}

void BatClient::setPromotionCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  LOG(ERROR) << "!!!setPromotionCallback == " << response;
}

void BatClient::getPromotionCaptcha() {
  batClientWebRequest_.run(buildURL((std::string)GET_PROMOTION_CAPTCHA + state_.walletInfo_.paymentId_, ""),
    base::Bind(&BatClient::getPromotionCaptchaCallback,
      base::Unretained(this)), std::vector<std::string>(), "", "", FETCH_CALLBACK_EXTRA_DATA_ST(),
      URL_METHOD::GET);
}

void BatClient::getPromotionCaptchaCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  LOG(ERROR) << "!!!getPromotionCaptchaCallback == " << response;
}

}
