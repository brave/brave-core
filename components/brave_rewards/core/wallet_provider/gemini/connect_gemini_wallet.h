/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_GEMINI_CONNECT_GEMINI_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_GEMINI_CONNECT_GEMINI_WALLET_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "brave/components/brave_rewards/core/endpoints/gemini/get_recipient_id/get_recipient_id_gemini.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"

namespace brave_rewards::core {
class LedgerImpl;

namespace endpoint {
class GeminiServer;
}

namespace gemini {

class ConnectGeminiWallet : public wallet_provider::ConnectExternalWallet {
 public:
  explicit ConnectGeminiWallet(LedgerImpl*);

  ~ConnectGeminiWallet() override;

 private:
  const char* WalletType() const override;

  void Authorize(OAuthInfo&&, ConnectExternalWalletCallback) const override;

  void OnAuthorize(ConnectExternalWalletCallback,
                   mojom::Result,
                   std::string&& token) const;

  void OnGetRecipientID(ConnectExternalWalletCallback,
                        std::string&& token,
                        endpoints::GetRecipientIDGemini::Result&&) const;

  void OnPostRecipientID(ConnectExternalWalletCallback,
                         std::string&& token,
                         mojom::Result,
                         std::string&& recipient_id) const;

  void OnPostAccount(ConnectExternalWalletCallback,
                     std::string&& token,
                     std::string&& recipient_id,
                     mojom::Result,
                     std::string&& linking_info,
                     std::string&& user_name) const;

  std::unique_ptr<endpoint::GeminiServer> gemini_server_;
};

}  // namespace gemini
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_GEMINI_CONNECT_GEMINI_WALLET_H_
