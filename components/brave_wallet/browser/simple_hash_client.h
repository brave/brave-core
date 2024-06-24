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

struct SolCompressedNftProofData {
  std::string root;
  std::string data_hash;
  std::string creator_hash;
  std::string owner;
  std::vector<std::string> proof;
  std::string merkle_tree;
  std::string delegate;
  uint32_t leaf_index = 0;
  uint32_t canopy_depth = 0;

  SolCompressedNftProofData();
  SolCompressedNftProofData(const SolCompressedNftProofData& data);
  ~SolCompressedNftProofData();

  bool operator==(const SolCompressedNftProofData& other) const;
};

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

  using FetchSolCompressedNftProofDataCallback =
      base::OnceCallback<void(std::optional<SolCompressedNftProofData>)>;

  using GetNftBalancesCallback =
      base::OnceCallback<void(const std::vector<uint64_t>& balances)>;

  using GetNftMetadatasCallback =
      base::OnceCallback<void(std::vector<mojom::NftMetadataPtr> metadatas)>;

  using GetNftsCallback =
      base::OnceCallback<void(std::vector<mojom::BlockchainTokenPtr> nfts)>;

  // Used for to autodiscover NFTs for an account address
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

  void FetchSolCompressedNftProofData(
      const std::string& token_address,
      FetchSolCompressedNftProofDataCallback callback);

  void GetNftBalances(const std::string& wallet_address,
                      std::vector<mojom::NftIdentifierPtr> nft_identifiers,
                      mojom::CoinType coin,
                      GetNftBalancesCallback callback);

  void GetNftMetadatas(mojom::CoinType coin,
                       std::vector<mojom::NftIdentifierPtr> nft_identifiers,
                       GetNftMetadatasCallback callback);

  void GetNfts(mojom::CoinType coin,
               std::vector<mojom::NftIdentifierPtr> nft_identifiers,
               GetNftsCallback callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest, DecodeMintAddress);
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest,
                           GetSimpleHashNftsByWalletUrl);
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest, ParseNFTsFromSimpleHash);
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest,
                           ParseSolCompressedNftProofData);
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest, GetNftsUrl);
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest, GetNfts);
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest, ParseTokenUrls);
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest, ParseBalances);
  FRIEND_TEST_ALL_PREFIXES(SimpleHashClientUnitTest, ParseMetadatas);

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

  void OnFetchSolCompressedNftProofData(
      FetchSolCompressedNftProofDataCallback callback,
      APIRequestResult api_request_result);

  void OnGetNftsForBalances(
      mojom::CoinType coin,
      const std::string& wallet_address,
      std::vector<mojom::NftIdentifierPtr> nft_identifiers,
      GetNftBalancesCallback callback,
      APIRequestResult api_request_result);

  void OnGetNftsForMetadatas(
      mojom::CoinType coin,
      std::vector<mojom::NftIdentifierPtr> nft_identifiers,
      GetNftMetadatasCallback callback,
      APIRequestResult api_request_result);

  void OnGetNfts(mojom::CoinType coin,
                 std::vector<mojom::BlockchainTokenPtr> nfts_so_far,
                 std::vector<mojom::NftIdentifierPtr> nft_identifiers,
                 GetNftsCallback callback,
                 APIRequestResult api_request_result);

  std::optional<std::pair<std::optional<std::string>,
                          std::vector<mojom::BlockchainTokenPtr>>>
  ParseNFTsFromSimpleHash(const base::Value& json_value,
                          mojom::CoinType coin,
                          bool skip_spam,
                          bool only_spam);

  std::optional<SolCompressedNftProofData> ParseSolCompressedNftProofData(
      const base::Value& json_value);

  std::optional<base::flat_map<mojom::NftIdentifierPtr,
                               base::flat_map<std::string, uint64_t>>>
  ParseBalances(const base::Value& json_value, mojom::CoinType coin);

  std::optional<base::flat_map<mojom::NftIdentifierPtr, mojom::NftMetadataPtr>>
  ParseMetadatas(const base::Value& json_value, mojom::CoinType coin);

  static GURL GetSimpleHashNftsByWalletUrl(
      const std::string& account_address,
      const std::vector<std::string>& chain_ids,
      const std::optional<std::string>& cursor);

  static GURL GetNftsUrl(
      mojom::CoinType coin,
      const std::vector<mojom::NftIdentifierPtr>& nft_identifiers);

  std::unique_ptr<APIRequestHelper> api_request_helper_;
  base::WeakPtrFactory<SimpleHashClient> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMPLE_HASH_CLIENT_H_
