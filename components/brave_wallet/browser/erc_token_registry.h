/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ERC_TOKEN_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ERC_TOKEN_REGISTRY_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "build/build_config.h"

namespace brave_wallet {

class ERCTokenRegistry {
 public:
  ERCTokenRegistry(const ERCTokenRegistry&) = delete;
  ERCTokenRegistry& operator=(const ERCTokenRegistry&) = delete;

  static ERCTokenRegistry* GetInstance();
  bool ParseERCTokens(const std::string& token_list);
  mojom::ERCTokenPtr GetTokenByContract(const std::string& contract);
  mojom::ERCTokenPtr GetTokenBySymbol(const std::string& symbol);
  std::vector<mojom::ERCTokenPtr> GetAllTokens();

 protected:
  std::vector<mojom::ERCTokenPtr> erc_tokens_;
  friend struct base::DefaultSingletonTraits<ERCTokenRegistry>;

  ERCTokenRegistry();
  virtual ~ERCTokenRegistry();
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ERC_TOKEN_REGISTRY_H_
