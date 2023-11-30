/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMPLE_HASH_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMPLE_HASH_CLIENT_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/solana_address.h"

namespace brave_wallet {

class SimpleHashClient {
 public:
  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;

  SimpleHashClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  SimpleHashClient(const SimpleHashClient&) = delete;
  SimpleHashClient& operator=(SimpleHashClient&) = delete;
  ~SimpleHashClient();

  // For discovering NFTs on Solana and Ethereum
  using FetchNFTsFromSimpleHashCallback =
      base::OnceCallback<void(std::vector<mojom::BlockchainTokenPtr> nfts,
                              const std::optional<std::string>& cursor)>;

  using FetchAllNFTsFromSimpleHashCallback =
      base::OnceCallback<void(std::vector<mojom::BlockchainTokenPtr> nfts)>;

  void FetchNFTsFromSimpleHash(const std::string& account_address,
                               const std::vector<std::string>& chain_ids,
                               mojom::CoinType coin,
                               const std::optional<std::string>& cursor,
                               bool skip_spam,
                               bool only_spam,
                               FetchNFTsFromSimpleHashCallback callback);

  void FetchAllNFTsFromSimpleHash(const std::string& account_address,
                                  const std::vector<std::string>& chain_ids,
                                  mojom::CoinType coin,
                                  FetchAllNFTsFromSimpleHashCallback callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest, DecodeMintAddress);
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest,
                           GetSimpleHashNftsByWalletUrl);
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest, ParseNFTsFromSimpleHash);

  void OnFetchNFTsFromSimpleHash(mojom::CoinType coin,
                                 bool skip_spam,
                                 bool only_spam,
                                 FetchNFTsFromSimpleHashCallback callback,
                                 APIRequestResult api_request_result);

  void OnFetchAllNFTsFromSimpleHash(
      std::vector<mojom::BlockchainTokenPtr> nfts_so_far,
      const std::string& account_address,
      const std::vector<std::string>& chain_ids,
      mojom::CoinType coin,
      FetchAllNFTsFromSimpleHashCallback callback,
      std::vector<mojom::BlockchainTokenPtr> nfts,
      const std::optional<std::string>& next_cursor);

  std::optional<std::pair<std::optional<std::string>,
                          std::vector<mojom::BlockchainTokenPtr>>>
  ParseNFTsFromSimpleHash(const base::Value& json_value,
                          mojom::CoinType coin,
                          bool skip_spam,
                          bool only_spam);

  static GURL GetSimpleHashNftsByWalletUrl(
      const std::string& account_address,
      const std::vector<std::string>& chain_ids,
      const std::optional<std::string>& cursor);

  std::unique_ptr<APIRequestHelper> api_request_helper_;
  base::WeakPtrFactory<SimpleHashClient> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMPLE_HASH_CLIENT_H_
