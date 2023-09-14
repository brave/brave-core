/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/nft_metadata_fetcher.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "build/build_config.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#endif

namespace {

absl::optional<uint32_t> DecodeUint32(const std::vector<uint8_t>& input,
                                      size_t& offset) {
  if (offset >= input.size() || input.size() - offset < sizeof(uint32_t)) {
    return absl::nullopt;
  }

  // Read bytes in little endian order.
  base::span<const uint8_t> s =
      base::make_span(input.begin() + offset, sizeof(uint32_t));
  uint32_t uint32_le = *reinterpret_cast<const uint32_t*>(s.data());

  offset += sizeof(uint32_t);

#if defined(ARCH_CPU_LITTLE_ENDIAN)
  return uint32_le;
#else
  return base::ByteSwap(uint32_le);
#endif
}

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("nft_metadata_fetcher", R"(
      semantics {
        sender: "NFT Metata Fetcher"
        description:
          "This service is used to fetch NFT metadata "
          "on behalf of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "NFT Metadata JSON."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on chrome://flags."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace

namespace brave_wallet {

NftMetadataFetcher::NftMetadataFetcher(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    JsonRpcService* json_rpc_service,
    PrefService* prefs)
    : api_request_helper_(new APIRequestHelper(GetNetworkTrafficAnnotationTag(),
                                               url_loader_factory)),
      json_rpc_service_(json_rpc_service),
      prefs_(prefs),
      weak_ptr_factory_(this) {}

NftMetadataFetcher::~NftMetadataFetcher() = default;

void NftMetadataFetcher::GetEthTokenMetadata(
    const std::string& contract_address,
    const std::string& token_id,
    const std::string& chain_id,
    const std::string& interface_id,
    GetEthTokenMetadataCallback callback) {
  auto network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        "", "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  if (!EthAddress::IsValidAddress(contract_address)) {
    std::move(callback).Run(
        "", "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&NftMetadataFetcher::OnGetSupportsInterface,
                     weak_ptr_factory_.GetWeakPtr(), contract_address,
                     interface_id, token_id, chain_id, std::move(callback));

  json_rpc_service_->GetSupportsInterface(
      contract_address, interface_id, chain_id, std::move(internal_callback));
}

void NftMetadataFetcher::OnGetSupportsInterface(
    const std::string& contract_address,
    const std::string& interface_id,
    const std::string& token_id,
    const std::string& chain_id,
    GetEthTokenMetadataCallback callback,
    bool is_supported,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    std::move(callback).Run("", "", error, error_message);
    return;
  }

  if (!is_supported) {
    std::move(callback).Run(
        "", "", mojom::ProviderError::kMethodNotSupported,
        l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&NftMetadataFetcher::OnGetEthTokenUri,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  json_rpc_service_->GetEthTokenUri(chain_id, contract_address, token_id,
                                    interface_id, std::move(internal_callback));
}

void NftMetadataFetcher::OnGetEthTokenUri(GetEthTokenMetadataCallback callback,
                                          const GURL& uri,
                                          mojom::ProviderError error,
                                          const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    std::move(callback).Run("", "", error, error_message);
    return;
  }

  if (!uri.is_valid()) {
    std::move(callback).Run(
        "", "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&NftMetadataFetcher::CompleteGetEthTokenMetadata,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback), uri);
  FetchMetadata(uri, std::move(internal_callback));
}

void NftMetadataFetcher::FetchMetadata(
    GURL url,
    GetTokenMetadataIntermediateCallback callback) {
  // Obtain JSON from the URL depending on the scheme.
  // IPFS, HTTPS, and data URIs are supported.
  // IPFS and HTTPS URIs require an additional request to fetch the metadata.
  std::string metadata_json;
  std::string scheme = url.scheme();
#if BUILDFLAG(ENABLE_IPFS)
  if (scheme != url::kDataScheme && scheme != url::kHttpsScheme &&
      scheme != ipfs::kIPFSScheme) {
#else
  if (scheme != url::kDataScheme && scheme != url::kHttpsScheme) {
#endif
    std::move(callback).Run(
        "", static_cast<int>(mojom::JsonRpcError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  if (scheme == url::kDataScheme) {
    if (!eth::ParseDataURIAndExtractJSON(url, &metadata_json)) {
      std::move(callback).Run(
          "", static_cast<int>(mojom::JsonRpcError::kParsingError),
          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
      return;
    }

    // Sanitize JSON
    data_decoder::JsonSanitizer::Sanitize(
        std::move(metadata_json),
        base::BindOnce(&NftMetadataFetcher::OnSanitizeTokenMetadata,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }

#if BUILDFLAG(ENABLE_IPFS)
  if (scheme == ipfs::kIPFSScheme &&
      !ipfs::TranslateIPFSURI(url, &url, ipfs::GetDefaultNFTIPFSGateway(prefs_),
                              false)) {
    std::move(callback).Run(
        "", static_cast<int>(mojom::JsonRpcError::kParsingError),
        l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }
#endif

  auto internal_callback =
      base::BindOnce(&NftMetadataFetcher::OnGetTokenMetadataPayload,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_->Request(
      "GET", url, "", "", std::move(internal_callback), {},
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void NftMetadataFetcher::OnSanitizeTokenMetadata(
    GetTokenMetadataIntermediateCallback callback,
    data_decoder::JsonSanitizer::Result result) {
  if (!result.has_value()) {
    VLOG(1) << "Data URI JSON validation error:" << result.error();
    std::move(callback).Run(
        "", static_cast<int>(mojom::JsonRpcError::kParsingError),
        l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  std::move(callback).Run(*result, 0, "");  // 0 is kSuccess
}

void NftMetadataFetcher::OnGetTokenMetadataPayload(
    GetTokenMetadataIntermediateCallback callback,
    APIRequestResult api_request_result) {
  mojom::ProviderErrorUnionPtr error;
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", static_cast<int>(mojom::JsonRpcError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  // Invalid JSON becomes an empty string after sanitization
  if (api_request_result.body().empty()) {
    std::move(callback).Run(
        "", static_cast<int>(mojom::JsonRpcError::kParsingError),
        l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  std::move(callback).Run(api_request_result.body(), 0, "");  // 0 is kSuccess
}

void NftMetadataFetcher::CompleteGetEthTokenMetadata(
    GetEthTokenMetadataCallback callback,
    const GURL& uri,
    const std::string& response,
    int error,
    const std::string& error_message) {
  mojom::ProviderError mojo_err = static_cast<mojom::ProviderError>(error);
  if (!mojom::IsKnownEnumValue(mojo_err)) {
    mojo_err = mojom::ProviderError::kUnknown;
  }
  std::move(callback).Run(uri.spec(), response, mojo_err, error_message);
}

void NftMetadataFetcher::GetSolTokenMetadata(
    const std::string& chain_id,
    const std::string& token_mint_address,
    GetSolTokenMetadataCallback callback) {
  // Derive metadata PDA for the NFT accounts
  absl::optional<std::string> associated_metadata_account =
      SolanaKeyring::GetAssociatedMetadataAccount(token_mint_address);
  if (!associated_metadata_account) {
    std::move(callback).Run(
        "", "", mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&NftMetadataFetcher::OnGetSolanaAccountInfoTokenMetadata,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  json_rpc_service_->GetSolanaAccountInfo(
      chain_id, *associated_metadata_account, std::move(internal_callback));
}

void NftMetadataFetcher::OnGetSolanaAccountInfoTokenMetadata(
    GetSolTokenMetadataCallback callback,
    absl::optional<SolanaAccountInfo> account_info,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess || !account_info) {
    std::move(callback).Run("", "", error, error_message);
    return;
  }

  absl::optional<std::vector<uint8_t>> metadata =
      base::Base64Decode(account_info->data);

  if (!metadata) {
    std::move(callback).Run("", "", mojom::SolanaProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  absl::optional<GURL> url = DecodeMetadataUri(*metadata);
  if (!url || !url.value().is_valid()) {
    std::move(callback).Run("", "", mojom::SolanaProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  FetchMetadata(*url,
                base::BindOnce(&NftMetadataFetcher::CompleteGetSolTokenMetadata,
                               weak_ptr_factory_.GetWeakPtr(), *url,
                               std::move(callback)));
}

void NftMetadataFetcher::CompleteGetSolTokenMetadata(
    const GURL& uri,
    GetSolTokenMetadataCallback callback,
    const std::string& response,
    int error,
    const std::string& error_message) {
  mojom::SolanaProviderError mojo_err =
      static_cast<mojom::SolanaProviderError>(error);
  if (!mojom::IsKnownEnumValue(mojo_err)) {
    mojo_err = mojom::SolanaProviderError::kUnknown;
  }
  std::move(callback).Run(uri.spec(), response, mojo_err, error_message);
}

// static
// Expects a the bytes of a Borsh encoded Metadata struct (see
// https://docs.rs/spl-token-metadata/latest/spl_token_metadata/state/struct.Metadata.html)
// and returns the URI string in of the nested Data struct (see
// https://docs.rs/spl-token-metadata/latest/spl_token_metadata/state/struct.Data.html)
// as a GURL.
absl::optional<GURL> NftMetadataFetcher::DecodeMetadataUri(
    const std::vector<uint8_t>& data) {
  size_t offset = 0;
  offset = offset + /* Skip first byte for metadata.key */ 1 +
           /* Skip next 32 bytes for `metadata.update_authority` */ 32 +
           /* Skip next 32 bytes for `metadata.mint` */ 32;

  // Skip next field, metdata.data.name, a string
  // whose length is represented by a leading 32 bit integer
  auto length = DecodeUint32(data, offset);
  if (!length) {
    return absl::nullopt;
  }
  offset += static_cast<size_t>(*length);

  // Skip next field, `metdata.data.symbol`, a string
  // whose length is represented by a leading 32 bit integer
  length = DecodeUint32(data, offset);
  if (!length) {
    return absl::nullopt;
  }
  offset += static_cast<size_t>(*length);

  // Parse next field, metadata.data.uri, a string
  length = DecodeUint32(data, offset);
  if (!length) {
    return absl::nullopt;
  }

  // Prevent out of bounds access in case length value incorrent
  if (data.size() <= offset + *length) {
    return absl::nullopt;
  }
  std::string uri =
      std::string(data.begin() + offset, data.begin() + offset + *length);
  return GURL(uri);
}

}  // namespace brave_wallet
