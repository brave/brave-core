/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_AUTHORIZATION_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_AUTHORIZATION_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_user.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {

class UpholdAuthorization {
 public:
  explicit UpholdAuthorization(bat_ledger::LedgerImpl* ledger, Uphold* uphold);

  ~UpholdAuthorization();

  void Authorize(
    const std::map<std::string, std::string>& args,
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletAuthorizationCallback callback);

 private:
  void OnAuthorize(
    ledger::ExternalWalletAuthorizationCallback callback,
    const ledger::ExternalWallet& wallet,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void OnGetUser(
    const ledger::Result result,
    const User& user,
    ledger::ExternalWalletAuthorizationCallback callback,
    const ledger::ExternalWallet& wallet);

  void OnCardCreate(
    ledger::Result result,
    const std::string& address,
    ledger::ExternalWalletAuthorizationCallback callback,
    const ledger::ExternalWallet& wallet);


  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Uphold* uphold_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_AUTHORIZATION_H_
