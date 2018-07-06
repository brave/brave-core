/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_CLIENT_H_
#define BRAVELEDGER_BAT_CLIENT_H_

#include <string>
#include <vector>
#include <mutex>

#include "bat_balance.h"
#include "bat_client_webrequest.h"
#include "bat_helper.h"

namespace braveledger_bat_client {

class BatClient {
 public:

  explicit BatClient( bool useProxy = true);
  ~BatClient();

  void loadStateOrRegisterPersona();
  void requestCredentialsCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void registerPersonaCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void publisherTimestampCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  uint64_t getPublisherTimestamp();
  void publisherInfo(const std::string& publisher, braveledger_bat_client_webrequest::FetchCallback callback,
      const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void setContributionAmount(const double& amount);
  std::string getBATAddress();
  std::string getBTCAddress();
  std::string getETHAddress();
  std::string getLTCAddress();
  void getWalletProperties(braveledger_bat_client_webrequest::FetchCallback callback,
      const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  bool isReadyForReconcile();
  void reconcile(const std::string& viewingId, braveledger_bat_helper::SimpleCallback callback);
  unsigned int ballots(const std::string& viewingId);
  void votePublishers(const std::vector<std::string>& publishers, const std::string& viewingId);
  void prepareBallots();
  std::string getWalletPassphrase();
  void recoverWallet(const std::string& passPhrase);
  void getPromotion(const std::string& lang, const std::string& forPaymentId);
  void setPromotion(const std::string& promotionId, const std::string& captchaResponse);
  void getPromotionCaptcha();

 private:
  void getPromotionCaptchaCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void getPromotionCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void setPromotionCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void recoverWalletPublicKeyCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void recoverWalletCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void prepareBatch(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction);
  void prepareBatchCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void proofBatch(const std::vector<braveledger_bat_helper::BATCH_PROOF>& batchProof);
  void prepareVoteBatch(const std::vector<braveledger_bat_helper::BATCH_PROOF>& batchProof);
  void prepareBallot(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction);
  void commitBallot(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction);
  void prepareBallotCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void commitBallotCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void vote(const std::string& publisher, const std::string& viewingId);
  void loadStateOrRegisterPersonaCallback(bool result, const braveledger_bat_helper::CLIENT_STATE_ST& state);
  void registerPersona();
  void publisherTimestamp(bool saveState = true);
  void reconcileCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void currentReconcile();
  void currentReconcileCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void reconcilePayloadCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void updateRulesCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void updateRulesV2Callback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void registerViewing();
  void registerViewingCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  void viewingCredentials(const std::string& proofStringified, const std::string& anonizeViewingId);
  void viewingCredentialsCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  std::string getAnonizeProof(const std::string& registrarVK, const std::string& id, std::string& preFlight);
  std::string buildURL(const std::string& path, const std::string& prefix);

  bool useProxy_;
  std::unique_ptr<braveledger_bat_helper::CLIENT_STATE_ST> state_;
  uint64_t publisherTimestamp_;
  std::mutex state_mutex_;
  std::mutex transactions_access_mutex_;
  std::mutex ballots_access_mutex_;
  braveledger_bat_balance::BatBalance balance_;
  std::unique_ptr<braveledger_bat_helper::CURRENT_RECONCILE> currentReconcile_;
};

}  // namespace braveledger_bat_client

#endif  // BRAVELEDGER_BAT_CLIENT_H_
