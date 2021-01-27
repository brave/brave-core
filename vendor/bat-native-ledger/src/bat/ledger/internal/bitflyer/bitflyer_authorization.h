/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_AUTHORIZATION_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_AUTHORIZATION_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_server.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace bitflyer {

class BitflyerAuthorization {
 public:
  explicit BitflyerAuthorization(LedgerImpl* ledger);

  ~BitflyerAuthorization();

  void Authorize(const base::flat_map<std::string, std::string>& args,
                 ledger::ExternalWalletAuthorizationCallback callback);

 private:
  void OnAuthorize(const type::Result result,
                   const std::string& token,
                   const std::string& address,
                   const std::string& linking_info,
                   ledger::ExternalWalletAuthorizationCallback callback);

  void OnClaimWallet(const type::Result result,
                     const std::string& token,
                     const std::string& address,
                     const std::string& linking_info,
                     ledger::ExternalWalletAuthorizationCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::BitflyerServer> bitflyer_server_;
};

}  // namespace bitflyer
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_AUTHORIZATION_H_
