/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TEST_UTILS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_blockfrost_api.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"
#include "services/network/test/test_url_loader_factory.h"

namespace brave_wallet {

class CardanoWalletService;

class CardanoTestRpcServer {
 public:
  CardanoTestRpcServer();
  explicit CardanoTestRpcServer(CardanoWalletService& cardano_wallet_service);
  ~CardanoTestRpcServer();

  void SetUpCardanoRpc(const std::optional<std::string>& mnemonic,
                       std::optional<uint32_t> account_index);

  void AddUtxo(const std::string& address, uint32_t amount);

 private:
  void RequestInterceptor(const network::ResourceRequest& request);
  std::string ExtractApiRequestPath(const GURL& request_url);
  std::optional<std::string> IsAddressUtxoRequest(
      const network::ResourceRequest& request);

  std::array<uint8_t, 32> CreateNewTxHash();

  std::map<std::string, std::vector<cardano_rpc::blockfrost_api::UnspentOutput>>
      utxos_map_;
  std::array<uint8_t, 32> next_tx_hash_ = {};

  network::TestURLLoaderFactory url_loader_factory_;
  raw_ref<CardanoWalletService> cardano_wallet_service_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<CardanoHDKeyring> keyring_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TEST_UTILS_H_
