/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_AUTHORIZATION_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_AUTHORIZATION_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_server.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace uphold {

class UpholdAuthorization {
 public:
  explicit UpholdAuthorization(LedgerImpl* ledger);

  ~UpholdAuthorization();

  void Authorize(const base::flat_map<std::string, std::string>& args,
                 ledger::ExternalWalletAuthorizationCallback callback) const;

 private:
  void OnAuthorize(const type::Result result,
                   const std::string& token,
                   ledger::ExternalWalletAuthorizationCallback callback) const;

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::UpholdServer> uphold_server_;
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_AUTHORIZATION_H_
