/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_AUTHORIZATION_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_AUTHORIZATION_H_

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/internal/endpoint/uphold/uphold_server.h"
#include "bat/ledger/internal/uphold/uphold_user.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace uphold {

class UpholdAuthorization {
 public:
  explicit UpholdAuthorization(LedgerImpl* ledger);

  ~UpholdAuthorization();

  void Authorize(
      const std::map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback);

 private:
  void OnAuthorize(
      const type::Result result,
      const std::string& token,
      ledger::ExternalWalletAuthorizationCallback callback);

  void OnGetUser(
      const type::Result result,
      const User& user,
      ledger::ExternalWalletAuthorizationCallback callback);

  void OnCardCreate(
      const type::Result result,
      const std::string& address,
      ledger::ExternalWalletAuthorizationCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::UpholdServer> uphold_server_;
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_AUTHORIZATION_H_
