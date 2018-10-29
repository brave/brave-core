/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_CLIENT_H_
#define BRAVELEDGER_BAT_CLIENT_H_

#include <string>
#include <vector>
#include <mutex>

#include "bat/ledger/ledger_task_runner.h"
#include "bat/ledger/ledger_url_loader.h"
#include "bat/ledger/publisher_info.h"
#include "bat_helper.h"
#include "url_request_handler.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_bat_client {

class BatClient {
 public:
  explicit BatClient(bat_ledger::LedgerImpl* ledger);
  ~BatClient();

  void setRewardsMainEnabled(const bool& enabled);
  bool getRewardsMainEnabled() const;
  bool loadState(const std::string& data);
  void registerPersona();
  void requestCredentialsCallback(bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void registerPersonaCallback(bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void setContributionAmount(const double& amount);
  void setUserChangedContribution();
  void setAutoContribute(const bool& enabled);
  const std::string& getBATAddress() const;
  const std::string& getBTCAddress() const;
  const std::string& getETHAddress() const;
  const std::string& getLTCAddress() const;
  void resetReconcileStamp();
  uint64_t getReconcileStamp() const;
  bool didUserChangeContributionAmount() const;
  double getContributionAmount() const;
  bool getAutoContribute() const;
  bool isReadyForReconcile();
  braveledger_bat_helper::CURRENT_RECONCILE GetReconcileById(const std::string& viewingId);
  bool SetReconcile(const braveledger_bat_helper::CURRENT_RECONCILE& reconcile);
  void removeReconcileById(const std::string& viewingId);
  void reconcilePublisherList(const ledger::PUBLISHER_CATEGORY category,
                              const ledger::PublisherInfoList& list);
  void reconcile(const std::string& viewingId,
      const ledger::PUBLISHER_CATEGORY category,
      const std::vector<braveledger_bat_helper::PUBLISHER_ST>& list,
      const std::vector<braveledger_bat_helper::RECONCILE_DIRECTION>& directions = {});
  unsigned int ballots(const std::string& viewingId);
  void votePublishers(const std::vector<std::string>& publishers, const std::string& viewingId);
  void prepareBallots();
  std::string getWalletPassphrase() const;
  void walletPropertiesCallback(bool success, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void recoverWallet(const std::string& passPhrase);
  void getGrant(const std::string& lang, const std::string& forPaymentId);
  void setGrant(const std::string& captchaResponse, const std::string& promotionId);
  void getGrantCaptcha();
  void getWalletProperties();
  bool isWalletCreated() const;
  void prepareVoteBatch();
  void voteBatch();

  void continueRecover(int result, size_t *written, std::vector<uint8_t>& newSeed);

  void OnNicewareListLoaded(const std::string& pass_phrase,
                                ledger::Result result,
                                const std::string& data);
  double getBalance() const;
  uint64_t getLastGrantLoadTimestamp() const;

 private:
  void saveState();
  void getGrantCaptchaCallback(bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void getGrantCallback(bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void setGrantCallback(bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void recoverWalletPublicKeyCallback(bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void recoverWalletCallback(bool result, const std::string& response,
      const std::map<std::string, std::string>& headers, const std::string& paymentId);
  void prepareBatch(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction);
  void prepareBatchCallback(bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void proofBatch(
      const std::vector<braveledger_bat_helper::BATCH_PROOF>& batchProof,
      ledger::LedgerTaskRunner::CallerThreadCallback callback);
  void proofBatchCallback(
      const std::vector<braveledger_bat_helper::BATCH_PROOF>& batchProof,
      const std::vector<std::string>& proofs);
  void voteBatchCallback(const std::string& publisher, bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  //void prepareBallot(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction);
  //void commitBallot(const braveledger_bat_helper::BALLOT_ST& ballot, const braveledger_bat_helper::TRANSACTION_ST& transaction);
  //void prepareBallotCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
  //void commitBallotCallback(bool result, const std::string& response);
  void vote(const std::string& publisher, const std::string& viewingId);
  void reconcileCallback(const std::string& viewingId, bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void currentReconcile(const std::string& viewingId);
  void currentReconcileCallback(const std::string& viewingId, bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void reconcilePayloadCallback(const std::string& viewingId, bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void registerViewing(const std::string& viewingId);
  void registerViewingCallback(const std::string& viewingId, bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void viewingCredentials(const std::string& viewingId, const std::string& proofStringified, const std::string& anonizeViewingId);
  void viewingCredentialsCallback(const std::string& viewingId, bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  std::string getAnonizeProof(const std::string& registrarVK, const std::string& id, std::string& preFlight);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<braveledger_bat_helper::CLIENT_STATE_ST> state_;

  bat_ledger::URLRequestHandler handler_;
};

}  // namespace braveledger_bat_client

#endif  // BRAVELEDGER_BAT_CLIENT_H_
