/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_GEMINI_CONNECT_GEMINI_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_GEMINI_CONNECT_GEMINI_WALLET_H_

#include <string>

#include "base/containers/flat_map.h"
#include "brave/components/brave_rewards/core/endpoint/gemini/gemini_server.h"
#include "brave/components/brave_rewards/core/endpoints/gemini/get_recipient_id/get_recipient_id_gemini.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"

namespace ledger {
class LedgerImpl;

namespace gemini {

class ConnectGeminiWallet : public wallet_provider::ConnectExternalWallet {
 public:
  explicit ConnectGeminiWallet(LedgerImpl& ledger);

  ~ConnectGeminiWallet() override;

 private:
  const char* WalletType() const override;

  void Authorize(OAuthInfo&&, ledger::ConnectExternalWalletCallback) override;

  void OnAuthorize(ledger::ConnectExternalWalletCallback,
                   mojom::Result,
                   std::string&& token);

  void OnGetRecipientID(ledger::ConnectExternalWalletCallback,
                        std::string&& token,
                        endpoints::GetRecipientIDGemini::Result&&);

  void OnPostRecipientID(ledger::ConnectExternalWalletCallback,
                         std::string&& token,
                         mojom::Result,
                         std::string&& recipient_id);

  void OnPostAccount(ledger::ConnectExternalWalletCallback,
                     std::string&& token,
                     std::string&& recipient_id,
                     mojom::Result,
                     std::string&& linking_info,
                     std::string&& user_name);

  endpoint::GeminiServer gemini_server_;
};

}  // namespace gemini
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_GEMINI_CONNECT_GEMINI_WALLET_H_
