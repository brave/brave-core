/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NFT_METADATA_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NFT_METADATA_FETCHER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "services/data_decoder/public/cpp/json_sanitizer.h"

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
  using GetTokenMetadataIntermediateCallback =
      base::OnceCallback<void(const std::string& response,
                              int error,
                              const std::string& error_message)>;
  using GetEthTokenMetadataCallback =
      base::OnceCallback<void(const std::string& result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using GetSolTokenMetadataCallback =
      base::OnceCallback<void(const std::string& result,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetEthTokenMetadata(const std::string& contract_address,
                           const std::string& token_id,
                           const std::string& chain_id,
                           const std::string& interface_id,
                           GetEthTokenMetadataCallback callback);
  void GetSolTokenMetadata(const std::string& nft_account_address,
                           GetSolTokenMetadataCallback callback);

 private:
  void OnGetSupportsInterfaceTokenMetadata(const std::string& contract_address,
                                           const std::string& interface_id,
                                           const std::string& token_id,
                                           const std::string& chain_id,
                                           // const GURL& network_url,
                                           GetEthTokenMetadataCallback callback,
                                           bool is_supported,
                                           mojom::ProviderError error,
                                           const std::string& error_message);

  void OnGetTokenUriTokenMetadata(GetEthTokenMetadataCallback callback,
                                  const GURL& uri,
                                  mojom::ProviderError error,
                                  const std::string& error_message);

  void FetchMetadata(GURL url, GetTokenMetadataIntermediateCallback callback);
  void OnSanitizeTokenMetadata(GetTokenMetadataIntermediateCallback callback,
                               data_decoder::JsonSanitizer::Result result);

  void OnGetTokenMetadataPayload(GetTokenMetadataIntermediateCallback callback,
                                 APIRequestResult api_request_result);

  void OnGetSolanaAccountInfoTokenMetadata(
      GetSolTokenMetadataCallback callback,
      absl::optional<SolanaAccountInfo> account_info,
      mojom::SolanaProviderError error,
      const std::string& error_message);
  void CompleteGetEthTokenMetadata(GetEthTokenMetadataCallback callback,
                                   const std::string& response,
                                   int error,
                                   const std::string& error_message);

  void CompleteGetSolTokenMetadata(GetSolTokenMetadataCallback callback,
                                   const std::string& response,
                                   int error,
                                   const std::string& error_message);

  friend class NftMetadataFetcherUnitTest;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<APIRequestHelper> api_request_helper_;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  base::WeakPtrFactory<NftMetadataFetcher> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NFT_METADATA_FETCHER_H_
