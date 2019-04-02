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
#include "bat/ledger/internal/bat_helper.h"

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
      const braveledger_bat_helper::CURRENT_RECONCILE& reconcile);

  bool UpdateReconcile(
      const braveledger_bat_helper::CURRENT_RECONCILE& reconcile);

  braveledger_bat_helper::CURRENT_RECONCILE GetReconcileById(
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

  const std::string& GetBATAddress() const;

  const std::string& GetBTCAddress() const;

  const std::string& GetETHAddress() const;

  const std::string& GetLTCAddress() const;

  uint64_t GetReconcileStamp() const;

  void ResetReconcileStamp();

  uint64_t GetLastGrantLoadTimestamp() const;

  void SetLastGrantLoadTimestamp(uint64_t stamp);

  bool IsWalletCreated() const;

  double GetBalance() const;

  const std::string& GetPaymentId() const;

  void SetPaymentId(const std::string& payment_id);

  const braveledger_bat_helper::Grants& GetGrants() const;

  void SetGrants(braveledger_bat_helper::Grants grants);

  const std::string& GetPersonaId() const;

  void SetPersonaId(const std::string& persona_id);

  const std::string& GetUserId() const;

  void SetUserId(const std::string& user_id);

  const std::string& GetRegistrarVK() const;

  void SetRegistrarVK(const std::string& registrar_vk);

  const std::string& GetPreFlight() const;

  void SetPreFlight(const std::string& pre_flight);

  const braveledger_bat_helper::WALLET_INFO_ST& GetWalletInfo() const;

  void SetWalletInfo(const braveledger_bat_helper::WALLET_INFO_ST& info);

  const braveledger_bat_helper::WALLET_PROPERTIES_ST&
  GetWalletProperties() const;

  void SetWalletProperties(
      braveledger_bat_helper::WALLET_PROPERTIES_ST* properties);

  unsigned int GetDays() const;

  void SetDays(unsigned int days);

  const braveledger_bat_helper::Transactions& GetTransactions() const;

  void SetTransactions(
      const braveledger_bat_helper::Transactions& transactions);

  const braveledger_bat_helper::Ballots& GetBallots() const;

  void SetBallots(const braveledger_bat_helper::Ballots& ballots);

  const braveledger_bat_helper::BatchVotes& GetBatch() const;

  void SetBatch(const braveledger_bat_helper::BatchVotes& votes);

  const std::string& GetCurrency() const;

  void SetCurrency(const std::string& currency);

  uint64_t GetBootStamp() const;

  void SetBootStamp(uint64_t stamp);

  const std::string& GetMasterUserToken() const;

  void SetMasterUserToken(const std::string& token);

  bool AddReconcileStep(const std::string& viewing_id,
                        ledger::ContributionRetry step,
                        int level);

  const braveledger_bat_helper::CurrentReconciles& GetCurrentReconciles() const;

  double GetDefaultContributionAmount();

  void SetAddress(std::map<std::string, std::string> addresses);

  void ResetState();

 private:
  void SaveState();

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<braveledger_bat_helper::CLIENT_STATE_ST> state_;
};

}  // namespace braveledger_bat_state

#endif  // BRAVELEDGER_BAT_CLIENT_STATE_H_
