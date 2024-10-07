/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NFT_METADATA_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NFT_METADATA_FETCHER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/containers/span.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"

class PrefService;

namespace brave_wallet {

class JsonRpcService;

class NftMetadataFetcher {
 public:
  NftMetadataFetcher(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      JsonRpcService* json_rpc_service,
      PrefService* prefs);

  NftMetadataFetcher(const NftMetadataFetcher&) = delete;
  NftMetadataFetcher& operator=(NftMetadataFetcher&) = delete;
  ~NftMetadataFetcher();

  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;
  using GetEthTokenMetadataCallback =
      base::OnceCallback<void(const std::string& token_url,
                              const std::string& result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetEthTokenMetadata(const std::string& contract_address,
                           const std::string& token_id,
                           const std::string& chain_id,
                           const std::string& interface_id,
                           GetEthTokenMetadataCallback callback);
  using GetSolTokenMetadataCallback =
      base::OnceCallback<void(const std::string& token_url,
                              const std::string& result,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetSolTokenMetadata(const std::string& chain_id,
                           const std::string& token_mint_address,
                           GetSolTokenMetadataCallback callback);
  using GetTokenMetadataIntermediateCallback =
      base::OnceCallback<void(const std::string& response,
                              int error,
                              const std::string& error_message)>;
  void FetchMetadata(GURL url, GetTokenMetadataIntermediateCallback callback);

 private:
  void OnGetSupportsInterface(const std::string& contract_address,
                              const std::string& interface_id,
                              const std::string& token_id,
                              const std::string& chain_id,
                              // const GURL& network_url,
                              GetEthTokenMetadataCallback callback,
                              bool is_supported,
                              mojom::ProviderError error,
                              const std::string& error_message);

  void OnGetEthTokenUri(GetEthTokenMetadataCallback callback,
                        const GURL& uri,
                        mojom::ProviderError error,
                        const std::string& error_message);
  void OnSanitizeTokenMetadata(GetTokenMetadataIntermediateCallback callback,
                               api_request_helper::ValueOrError result);
  void OnGetTokenMetadataPayload(GetTokenMetadataIntermediateCallback callback,
                                 APIRequestResult api_request_result);
  void OnGetSolanaAccountInfoTokenMetadata(
      GetSolTokenMetadataCallback callback,
      std::optional<SolanaAccountInfo> account_info,
      mojom::SolanaProviderError error,
      const std::string& error_message);
  void CompleteGetEthTokenMetadata(GetEthTokenMetadataCallback callback,
                                   const GURL& uri,
                                   const std::string& response,
                                   int error,
                                   const std::string& error_message);
  void CompleteGetSolTokenMetadata(const GURL& uri,
                                   GetSolTokenMetadataCallback callback,
                                   const std::string& response,
                                   int error,
                                   const std::string& error_message);

  friend class NftMetadataFetcherUnitTest;
  FRIEND_TEST_ALL_PREFIXES(NftMetadataFetcherUnitTest, DecodeMetadataUri);

  static std::optional<GURL> DecodeMetadataUri(base::span<const uint8_t> data);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<APIRequestHelper> api_request_helper_;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  base::WeakPtrFactory<NftMetadataFetcher> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NFT_METADATA_FETCHER_H_
