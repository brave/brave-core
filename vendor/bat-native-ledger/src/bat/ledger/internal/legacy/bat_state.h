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
#include "bat/ledger/internal/legacy/client_properties.h"
#include "bat/ledger/internal/legacy/wallet_info_properties.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_bat_state {

class LegacyBatState {
 public:
  explicit LegacyBatState(bat_ledger::LedgerImpl* ledger);
  ~LegacyBatState();

  bool LoadState(const std::string& data);

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

  const ledger::WalletInfoProperties& GetWalletInfo() const;

  void SetWalletInfo(const ledger::WalletInfoProperties& info);

  const ledger::WalletProperties& GetWalletProperties() const;

  void SetWalletProperties(
      ledger::WalletProperties* properties);

  uint64_t GetBootStamp() const;

  void SetBootStamp(uint64_t stamp);

  double GetDefaultContributionAmount();

  void SetInlineTipSetting(const std::string& key, bool enabled);

  bool GetInlineTipSetting(const std::string& key) const;

 private:
  void SaveState();

  void OnSaveState(const ledger::Result result);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<ledger::ClientProperties> state_;
};

}  // namespace braveledger_bat_state

#endif  // BRAVELEDGER_BAT_CLIENT_STATE_H_
