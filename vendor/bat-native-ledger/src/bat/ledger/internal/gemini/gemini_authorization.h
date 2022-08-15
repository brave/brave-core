/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_GEMINI_GEMINI_AUTHORIZATION_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_GEMINI_GEMINI_AUTHORIZATION_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {
class GeminiServer;
}

namespace gemini {

class GeminiAuthorization {
 public:
  explicit GeminiAuthorization(LedgerImpl* ledger);

  ~GeminiAuthorization();

  void Authorize(const base::flat_map<std::string, std::string>& args,
                 ledger::ExternalWalletAuthorizationCallback);

 private:
  void OnPostAccount(ledger::ExternalWalletAuthorizationCallback,
                     const std::string& token,
                     const std::string& recipient_id,
                     type::Result,
                     const std::string& linking_info,
                     const std::string& name);

  void OnAuthorize(ledger::ExternalWalletAuthorizationCallback,
                   type::Result,
                   const std::string& token);

  void OnConnectWallet(ledger::ExternalWalletAuthorizationCallback,
                       const std::string& recipient_id,
                       type::Result);

  void OnFetchRecipientId(ledger::ExternalWalletAuthorizationCallback,
                          const std::string& token,
                          type::Result,
                          const std::string& recipient_id);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::GeminiServer> gemini_server_;
};

}  // namespace gemini
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_GEMINI_GEMINI_AUTHORIZATION_H_
