/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_CLIENT_H_
#define BRAVELEDGER_BAT_CLIENT_H_

#include <string>
#include <vector>
#include <mutex>

#include "bat/ledger/ledger_callback_handler.h"
#include "bat/ledger/ledger_url_loader.h"
#include "bat_helper.h"
#include "url_request_handler.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_bat_client {

class BatClient : public ledger::LedgerCallbackHandler {
 public:
  explicit BatClient(bat_ledger::LedgerImpl* ledger);
  ~BatClient() override;

  void loadStateOrRegisterPersonaCallback(bool success, const std::string& data);
  void requestCredentialsCallback(bool result, const std::string& response);
  void registerPersonaCallback(bool result, const std::string& response);
  void setContributionAmount(const double& amount);
  const std::string& getBATAddress() const;
  const std::string& getBTCAddress() const;
  const std::string& getETHAddress() const;
  const std::string& getLTCAddress() const;
  double getContributionAmount() const;
  bool isReadyForReconcile();
  void reconcile(const std::string& viewingId);
  unsigned int ballots(const std::string& viewingId);
  void votePublishers(const std::vector<std::string>& publishers, const std::string& viewingId);
  void prepareBallots();
  std::string getWalletPassphrase();
  void walletPropertiesCallback(bool success, const std::string& response);
  void recoverWallet(const std::string& passPhrase);
  void getPromotion(const std::string& lang, const std::string& forPaymentId);
  void setPromotion(const std::string& promotionId, const std::string& captchaResponse);
  void getPromotionCaptcha();
  void getWalletProperties();

 private:
  void saveState();
  void getPromotionCaptchaCallback(bool result, const std::string& response);
  void getPromotionCallback(bool result, const std::string& response);
  void setPromotionCallback(bool result, const std::string& response);
  void recoverWalletPublicKeyCallback(bool result, const std::string& response);
  void recoverWalletCallback(bool result, const std::string& response);
  void prepareBatch(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction);
  void prepareBatchCallback(bool result, const std::string& response);
  void proofBatch(const std::vector<braveledger_bat_helper::BATCH_PROOF>& batchProof);
  void prepareVoteBatch();
  void voteBatch();
  void voteBatchCallback(const std::string& publisher, bool result, const std::string& response);
  //void prepareBallot(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction);
  //void commitBallot(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction);
  //void prepareBallotCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  //void commitBallotCallback(bool result, const std::string& response);
  void vote(const std::string& publisher, const std::string& viewingId);
  void registerPersona();
  void reconcileCallback(bool result, const std::string& response);
  void currentReconcile();
  void currentReconcileCallback(bool result, const std::string& response);
  void reconcilePayloadCallback(bool result, const std::string& response);
  void updateRulesCallback(bool reconcile, bool result, const std::string& response);
  void updateRulesV2Callback(bool reconcile, bool result, const std::string& response);
  void registerViewing();
  void registerViewingCallback(bool result, const std::string& response);
  void viewingCredentials(const std::string& proofStringified, const std::string& anonizeViewingId);
  void viewingCredentialsCallback(bool result, const std::string& response);
  std::string getAnonizeProof(const std::string& registrarVK, const std::string& id, std::string& preFlight);
  std::string buildURL(const std::string& path, const std::string& prefix, const bool isBalance);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<braveledger_bat_helper::CLIENT_STATE_ST> state_;
  std::unique_ptr<braveledger_bat_helper::CURRENT_RECONCILE> currentReconcile_;

  bat_ledger::URLRequestHandler handler_;
};

}  // namespace braveledger_bat_client

#endif  // BRAVELEDGER_BAT_CLIENT_H_
