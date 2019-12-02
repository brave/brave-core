/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_CLIENT_STATE_H_
#define BRAVELEDGER_BAT_CLIENT_STATE_H_

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/properties/ballot_properties.h"
#include "bat/ledger/internal/properties/publisher_votes_properties.h"
#include "bat/ledger/internal/properties/client_properties.h"
#include "bat/ledger/internal/properties/current_reconcile_properties.h"
#include "bat/ledger/internal/properties/transaction_properties.h"
#include "bat/ledger/internal/properties/wallet_info_properties.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_bat_state {

class BatState {
 public:
  explicit BatState(bat_ledger::LedgerImpl* ledger);
  ~BatState();

  bool LoadState(const std::string& data);

  void AddReconcile(
      const std::string& viewing_id,
      const ledger::CurrentReconcileProperties& reconcile);

  bool UpdateReconcile(
      const ledger::CurrentReconcileProperties& reconcile);

  ledger::CurrentReconcileProperties GetReconcileById(
      const std::string& viewingId) const;

  void RemoveReconcileById(const std::string& viewingId);

  bool ReconcileExists(const std::string& viewingId) const;

  void SetRewardsMainEnabled(bool enabled);

  bool GetRewardsMainEnabled() const;

  void SetContributionAmount(double amount);

  double GetContributionAmount() const;

  void SetUserChangedContribution();

  bool GetUserChangedContribution() const;

  void SetAutoContribute(bool enabled);

  bool GetAutoContribute() const;

  const std::string& GetCardIdAddress() const;

  uint64_t GetReconcileStamp() const;

  void ResetReconcileStamp();

  bool IsWalletCreated() const;

  const std::string& GetPaymentId() const;

  const std::string& GetPersonaId() const;

  void SetPersonaId(const std::string& persona_id);

  const std::string& GetUserId() const;

  void SetUserId(const std::string& user_id);

  const std::string& GetRegistrarVK() const;

  void SetRegistrarVK(const std::string& registrar_vk);

  const std::string& GetPreFlight() const;

  void SetPreFlight(const std::string& pre_flight);

  const ledger::WalletInfoProperties& GetWalletInfo() const;

  void SetWalletInfo(const ledger::WalletInfoProperties& info);

  const ledger::WalletProperties& GetWalletProperties() const;

  void SetWalletProperties(
      ledger::WalletProperties* properties);

  unsigned int GetDays() const;

  void SetDays(unsigned int days);

  const ledger::Transactions& GetTransactions() const;

  void SetTransactions(
      const ledger::Transactions& transactions);

  const ledger::Ballots& GetBallots() const;

  void SetBallots(const ledger::Ballots& ballots);

  const ledger::PublisherVotes& GetPublisherVotes() const;

  void SetPublisherVotes(
      const ledger::PublisherVotes& publisher_votes);

  const std::string& GetCurrency() const;

  void SetCurrency(const std::string& currency);

  uint64_t GetBootStamp() const;

  void SetBootStamp(uint64_t stamp);

  const std::string& GetMasterUserToken() const;

  void SetMasterUserToken(const std::string& token);

  bool AddReconcileStep(const std::string& viewing_id,
                        ledger::ContributionRetry step,
                        int level);

  const ledger::CurrentReconciles& GetCurrentReconciles() const;

  double GetDefaultContributionAmount();

  void SetInlineTipSetting(const std::string& key, bool enabled);

  bool GetInlineTipSetting(const std::string& key) const;

 private:
  void SaveState();

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<ledger::ClientProperties> state_;
};

}  // namespace braveledger_bat_state

#endif  // BRAVELEDGER_BAT_CLIENT_STATE_H_
