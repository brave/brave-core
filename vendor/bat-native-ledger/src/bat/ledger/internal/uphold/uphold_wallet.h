/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_WALLET_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_WALLET_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"
#include "bat/ledger/internal/uphold/uphold_user.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace uphold {

class UpholdWallet {
 public:
  explicit UpholdWallet(LedgerImpl* ledger);

  ~UpholdWallet();

  void Generate(ledger::ResultCallback callback) const;

 private:
  void OnGetUser(const type::Result result,
                 const User& user,
                 ledger::ResultCallback callback) const;

  void OnCreateCard(const type::Result result,
                    const std::string& id,
                    ledger::ResultCallback callback) const;

  void GetAnonFunds(
      endpoint::promotion::GetWalletBalanceCallback callback) const;

  void OnGetAnonFunds(const type::Result result,
                      type::BalancePtr balance,
                      const std::string& id,
                      ledger::ResultCallback callback) const;

  void LinkWallet(const double user_funds,
                  const std::string& id,
                  endpoint::promotion::PostClaimUpholdCallback callback) const;

  void OnLinkWallet(const type::Result result,
                    const std::string& id,
                    ledger::ResultCallback callback) const;

  void OnTransferTokens(const type::Result result,
                        const std::string& drain_id,
                        ledger::ResultCallback callback) const;

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_WALLET_H_
