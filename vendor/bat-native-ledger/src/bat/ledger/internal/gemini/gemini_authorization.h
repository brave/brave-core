/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_GEMINI_GEMINI_AUTHORIZATION_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_GEMINI_GEMINI_AUTHORIZATION_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "bat/ledger/internal/endpoints/post_connect/post_connect.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {
class GeminiServer;
}

namespace gemini {

class GeminiAuthorization {
 public:
  explicit GeminiAuthorization(LedgerImpl*);

  ~GeminiAuthorization();

  void Authorize(const base::flat_map<std::string, std::string>& args,
                 ledger::ExternalWalletAuthorizationCallback);

 private:
  void OnPostAccount(ledger::ExternalWalletAuthorizationCallback,
                     std::string&& token,
                     std::string&& recipient_id,
                     mojom::Result,
                     std::string&& linking_info,
                     std::string&& user_name);

  void OnAuthorize(ledger::ExternalWalletAuthorizationCallback,
                   mojom::Result,
                   std::string&& token);

  void OnConnectWallet(ledger::ExternalWalletAuthorizationCallback,
                       std::string&& token,
                       std::string&& address,
                       endpoints::PostConnect::Result&&);

  void OnFetchRecipientId(ledger::ExternalWalletAuthorizationCallback,
                          std::string&& token,
                          mojom::Result,
                          std::string&& recipient_id);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::GeminiServer> gemini_server_;
};

}  // namespace gemini
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_GEMINI_GEMINI_AUTHORIZATION_H_
