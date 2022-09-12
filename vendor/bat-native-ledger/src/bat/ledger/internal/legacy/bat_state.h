/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_BAT_STATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_BAT_STATE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/legacy/client_properties.h"
#include "bat/ledger/internal/legacy/wallet_info_properties.h"

namespace ledger {
class LedgerImpl;
}

namespace braveledger_bat_state {

class LegacyBatState {
 public:
  explicit LegacyBatState(ledger::LedgerImpl* ledger);
  ~LegacyBatState();

  void Load(ledger::LegacyResultCallback callback);

  bool GetRewardsMainEnabled() const;

  double GetAutoContributionAmount() const;

  bool GetUserChangedContribution() const;

  bool GetAutoContributeEnabled() const;

  const std::string& GetCardIdAddress() const;

  uint64_t GetReconcileStamp() const;

  const std::string& GetPaymentId() const;

  const std::vector<uint8_t>& GetRecoverySeed() const;

  uint64_t GetCreationStamp() const;

  bool GetInlineTipSetting(const std::string& key) const;

 private:
  void OnLoad(ledger::mojom::Result result,
              const std::string& data,
              ledger::LegacyResultCallback callback);

  ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<ledger::ClientProperties> state_;
};

}  // namespace braveledger_bat_state

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_BAT_STATE_H_
