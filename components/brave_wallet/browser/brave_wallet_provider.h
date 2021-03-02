/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

class BraveWalletService;

namespace brave_wallet {

class BraveWalletProvider final
    : public brave_wallet::mojom::BraveWalletProvider {
 public:
  BraveWalletProvider(const BraveWalletProvider&) = delete;
  BraveWalletProvider& operator=(const BraveWalletProvider&) = delete;
  explicit BraveWalletProvider(BraveWalletService* wallet_service);
  ~BraveWalletProvider() override;

  void Request(const std::string& json_payload,
               RequestCallback callback) override;
  void OnResponse(RequestCallback callback,
                  const int status,
                  const std::string& response,
                  const std::map<std::string, std::string>& headers);
 private:
  BraveWalletService* wallet_service_;

  base::WeakPtrFactory<BraveWalletProvider> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_H_
