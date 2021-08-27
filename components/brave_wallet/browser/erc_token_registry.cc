/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/erc_token_registry.h"

#include <algorithm>
#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"

namespace brave_wallet {

ERCTokenRegistry::ERCTokenRegistry() = default;

ERCTokenRegistry::~ERCTokenRegistry() {}

ERCTokenRegistry* ERCTokenRegistry::GetInstance() {
  return base::Singleton<ERCTokenRegistry>::get();
}

mojo::PendingRemote<mojom::ERCTokenRegistry> ERCTokenRegistry::MakeRemote() {
  mojo::PendingRemote<mojom::ERCTokenRegistry> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void ERCTokenRegistry::Bind(
    mojo::PendingReceiver<mojom::ERCTokenRegistry> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void ERCTokenRegistry::UpdateTokenList(
    std::vector<mojom::ERCTokenPtr> erc_tokens) {
  erc_tokens_ = std::move(erc_tokens);
}

void ERCTokenRegistry::GetTokenByContract(const std::string& contract,
                                          GetTokenByContractCallback callback) {
  auto token_it =
      std::find_if(erc_tokens_.begin(), erc_tokens_.end(),
                   [&](const mojom::ERCTokenPtr& current_token) {
                     return current_token->contract_address == contract;
                   });
  if (token_it == erc_tokens_.end()) {
    std::move(callback).Run(nullptr);
    return;
  }
  std::move(callback).Run(token_it->Clone());
}

void ERCTokenRegistry::GetTokenBySymbol(const std::string& symbol,
                                        GetTokenBySymbolCallback callback) {
  auto token_it = std::find_if(erc_tokens_.begin(), erc_tokens_.end(),
                               [&](const mojom::ERCTokenPtr& current_token) {
                                 return current_token->symbol == symbol;
                               });

  if (token_it == erc_tokens_.end()) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(token_it->Clone());
}

void ERCTokenRegistry::GetAllTokens(GetAllTokensCallback callback) {
  std::vector<brave_wallet::mojom::ERCTokenPtr> erc_tokens_copy(
      erc_tokens_.size());
  std::transform(erc_tokens_.begin(), erc_tokens_.end(),
                 erc_tokens_copy.begin(),
                 [](const brave_wallet::mojom::ERCTokenPtr& current_token)
                     -> brave_wallet::mojom::ERCTokenPtr {
                   return current_token.Clone();
                 });
  std::move(callback).Run(std::move(erc_tokens_copy));
}

void ERCTokenRegistry::GetBuyTokens(GetBuyTokensCallback callback) {
  std::vector<brave_wallet::mojom::ERCTokenPtr> erc_buy_tokens;
  for (auto token : *kBuyTokens) {
    auto erc_token = brave_wallet::mojom::ERCToken::New();
    *erc_token = token;
    erc_buy_tokens.push_back(std::move(erc_token));
  }
  std::move(callback).Run(std::move(erc_buy_tokens));
}

}  // namespace brave_wallet
