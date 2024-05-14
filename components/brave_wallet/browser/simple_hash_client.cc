/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/simple_hash_client.h"

#include <map>
#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/environment.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/string_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

net::NetworkTrafficAnnotationTag
GetSimpleHashClientNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("brave_wallet_service", R"(
      semantics {
        sender: "SimpleHash Client"
        description:
          "This client is used to make requests to SimpleHash "
          "of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "NFT assets."
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

constexpr char kEthereum[] = "ethereum";
constexpr char kSolana[] = "solana";
constexpr char kPolygon[] = "polygon";
constexpr char kArbitrum[] = "arbitrum";
constexpr char kOptimism[] = "optimism";
constexpr char kAvalanche[] = "avalanche";
constexpr char kBsc[] = "bsc";
constexpr char kEthereumGoerli[] = "ethereum-goerli";
constexpr char kSolanaTestnet[] = "solana-testnet";
constexpr char kSolanaDevnet[] = "solana-devnet";
constexpr char kArbitrumNova[] = "arbitrum-nova";
constexpr char kGnosis[] = "gnosis";
constexpr char kGodwoken[] = "godwoken";
constexpr char kPalm[] = "palm";
constexpr char kPolygonZkEvm[] = "polygon-zkevm";
constexpr char kZkSyncEra[] = "zksync-era";

std::optional<std::string> ChainIdToSimpleHashChainId(
    const std::string& chain_id) {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      chain_id_lookup({
          {brave_wallet::mojom::kMainnetChainId, kEthereum},
          {brave_wallet::mojom::kSolanaMainnet, kSolana},
          {brave_wallet::mojom::kPolygonMainnetChainId, kPolygon},
          {brave_wallet::mojom::kArbitrumMainnetChainId, kArbitrum},
          {brave_wallet::mojom::kOptimismMainnetChainId, kOptimism},
          {brave_wallet::mojom::kAvalancheMainnetChainId, kAvalanche},
          {brave_wallet::mojom::kBnbSmartChainMainnetChainId, kBsc},
          {brave_wallet::mojom::kGoerliChainId, kEthereumGoerli},
          {brave_wallet::mojom::kSolanaTestnet, kSolanaTestnet},
          {brave_wallet::mojom::kSolanaDevnet, kSolanaDevnet},
          {brave_wallet::mojom::kArbitrumNovaChainId, kArbitrumNova},
          {brave_wallet::mojom::kGnosisChainId, kGnosis},
          {brave_wallet::mojom::kGodwokenChainId, kGodwoken},
          {brave_wallet::mojom::kPalmChainId, kPalm},
          {brave_wallet::mojom::kPolygonZKEVMChainId, kPolygonZkEvm},
          {brave_wallet::mojom::kZkSyncEraChainId, kZkSyncEra},
      });
  if (!chain_id_lookup->contains(chain_id)) {
    return std::nullopt;
  }

  return chain_id_lookup->at(chain_id);
}

std::optional<std::string> SimpleHashChainIdToChainId(
    const std::string& simple_hash_chain_id) {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      simple_hash_chain_id_lookup({
          {kEthereum, brave_wallet::mojom::kMainnetChainId},
          {kSolana, brave_wallet::mojom::kSolanaMainnet},
          {kPolygon, brave_wallet::mojom::kPolygonMainnetChainId},
          {kArbitrum, brave_wallet::mojom::kArbitrumMainnetChainId},
          {kOptimism, brave_wallet::mojom::kOptimismMainnetChainId},
          {kAvalanche, brave_wallet::mojom::kAvalancheMainnetChainId},
          {kBsc, brave_wallet::mojom::kBnbSmartChainMainnetChainId},
          {kEthereumGoerli, brave_wallet::mojom::kGoerliChainId},
          {kSolanaTestnet, brave_wallet::mojom::kSolanaTestnet},
          {kSolanaDevnet, brave_wallet::mojom::kSolanaDevnet},
          {kArbitrumNova, brave_wallet::mojom::kArbitrumNovaChainId},
          {kGnosis, brave_wallet::mojom::kGnosisChainId},
          {kGodwoken, brave_wallet::mojom::kGodwokenChainId},
          {kPalm, brave_wallet::mojom::kPalmChainId},
          {kPolygonZkEvm, brave_wallet::mojom::kPolygonZKEVMChainId},
          {kZkSyncEra, brave_wallet::mojom::kZkSyncEraChainId},
      });
  if (!simple_hash_chain_id_lookup->contains(simple_hash_chain_id)) {
    return std::nullopt;
  }

  return simple_hash_chain_id_lookup->at(simple_hash_chain_id);
}

base::flat_map<std::string, std::string> MakeBraveServicesKeyHeader() {
  base::flat_map<std::string, std::string> request_headers;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string brave_key(BUILDFLAG(BRAVE_SERVICES_KEY));
  if (env->HasVar("BRAVE_SERVICES_KEY")) {
    env->GetVar("BRAVE_SERVICES_KEY", &brave_key);
  }
  request_headers["x-brave-key"] = std::move(brave_key);

  return request_headers;
}

}  // namespace

namespace brave_wallet {

SimpleHashClient::SimpleHashClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(std::make_unique<APIRequestHelper>(
          GetSimpleHashClientNetworkTrafficAnnotationTag(),
          url_loader_factory)),
      weak_ptr_factory_(this) {}

SimpleHashClient::~SimpleHashClient() = default;

// Calls
// https://simplehash.wallet.brave.com/api/v0/nfts/owners?chains={chains}&wallet_addresses={wallet_addresses}
void SimpleHashClient::FetchNFTsFromSimpleHash(
    const std::string& account_address,
    const std::vector<std::string>& chain_ids,
    mojom::CoinType coin,
    const std::optional<std::string>& cursor,
    bool skip_spam,
    bool only_spam,
    FetchNFTsFromSimpleHashCallback callback) {
  if (!(coin == mojom::CoinType::ETH || coin == mojom::CoinType::SOL)) {
    std::move(callback).Run({}, std::nullopt);
    return;
  }

  GURL url = GetSimpleHashNftsByWalletUrl(account_address, chain_ids, cursor);
  if (!url.is_valid()) {
    std::move(callback).Run({}, std::nullopt);
    return;
  }

  auto internal_callback =
      base::BindOnce(&SimpleHashClient::OnFetchNFTsFromSimpleHash,
                     weak_ptr_factory_.GetWeakPtr(), coin, skip_spam, only_spam,
                     std::move(callback));

  api_request_helper_->Request("GET", url, "", "", std::move(internal_callback),
                               MakeBraveServicesKeyHeader(),
                               {.auto_retry_on_network_change = true});
}

void SimpleHashClient::OnFetchNFTsFromSimpleHash(
    mojom::CoinType coin,
    bool skip_spam,
    bool only_spam,
    FetchNFTsFromSimpleHashCallback callback,
    APIRequestResult api_request_result) {
  std::vector<mojom::BlockchainTokenPtr> nfts;
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(std::move(nfts), std::nullopt);
    return;
  }

  // Invalid JSON becomes an empty string after sanitization
  if (api_request_result.value_body().is_none()) {
    std::move(callback).Run(std::move(nfts), std::nullopt);
    return;
  }

  std::optional<std::pair<std::optional<std::string>,
                          std::vector<mojom::BlockchainTokenPtr>>>
      result = ParseNFTsFromSimpleHash(api_request_result.TakeBody(), coin,
                                       skip_spam, only_spam);
  if (!result) {
    std::move(callback).Run(std::move(nfts), std::nullopt);
    return;
  }

  for (auto& token : result.value().second) {
    nfts.push_back(std::move(token));
  }

  // Otherwise, return the nfts
  std::move(callback).Run(std::move(nfts), result->first);
}

void SimpleHashClient::FetchAllNFTsFromSimpleHash(
    const std::string& account_address,
    const std::vector<std::string>& chain_ids,
    mojom::CoinType coin,
    FetchAllNFTsFromSimpleHashCallback callback) {
  auto internal_callback = base::BindOnce(
      &SimpleHashClient::OnFetchAllNFTsFromSimpleHash,
      weak_ptr_factory_.GetWeakPtr(), std::vector<mojom::BlockchainTokenPtr>(),
      account_address, chain_ids, coin, std::move(callback));

  FetchNFTsFromSimpleHash(account_address, chain_ids, coin, std::nullopt,
                          true /* skip_spam*/, false /* only spam */,
                          std::move(internal_callback));
}

void SimpleHashClient::OnFetchAllNFTsFromSimpleHash(
    std::vector<mojom::BlockchainTokenPtr> nfts_so_far,
    const std::string& account_address,
    const std::vector<std::string>& chain_ids,
    mojom::CoinType coin,
    FetchAllNFTsFromSimpleHashCallback callback,
    std::vector<mojom::BlockchainTokenPtr> nfts,
    const std::optional<std::string>& next_cursor) {
  // Combine the NFTs with the ones fetched already
  for (auto& token : nfts) {
    nfts_so_far.push_back(std::move(token));
  }

  // If there is a next page, fetch it
  if (next_cursor) {
    auto internal_callback =
        base::BindOnce(&SimpleHashClient::OnFetchAllNFTsFromSimpleHash,
                       weak_ptr_factory_.GetWeakPtr(), std::move(nfts_so_far),
                       account_address, chain_ids, coin, std::move(callback));

    FetchNFTsFromSimpleHash(account_address, chain_ids, coin, *next_cursor,
                            true /* skip_spam */, false /* only_spam */,
                            std::move(internal_callback));
    return;
  }

  // Otherwise, return the nfts_so_far
  std::move(callback).Run(std::move(nfts_so_far));
}

std::optional<std::pair<std::optional<std::string>,
                        std::vector<mojom::BlockchainTokenPtr>>>
SimpleHashClient::ParseNFTsFromSimpleHash(const base::Value& json_value,
                                          mojom::CoinType coin,
                                          bool skip_spam,
                                          bool only_spam) {
  // Parses responses like this
  // {
  //   "next_cursor": null,
  //   "next": null,
  //   "previous": null,
  //   "nfts": [
  //     {
  //       "nft_id":
  //       "ethereum.0x57f1887a8bf19b14fc0df6fd9b2acc9af147ea85.537620",
  //       "chain": "ethereum",
  //       "contract_address": "0x57f1887a8BF19b14fC0dF6Fd9B2acc9Af147eA85",
  //       "token_id":
  //       "537620017325758495279955950362494277305103906517231892215",
  //       "name": "stochasticparrot.eth",
  //       "description": "stochasticparrot.eth, an ENS name.",
  //       "previews": {
  //         "image_small_url":
  //         "https://lh3.googleusercontent.com/KV7QzwejzheyvEsTvYogBPJnKUdNlrid6PwMnrA4WzOU0eWfOF6w6RRdnVM7n7DHWBFVAy7ocS-mAdM_GmwTvw4DV7kLdogg_e8=s250",
  //         "image_medium_url":
  //         "https://lh3.googleusercontent.com/KV7QzwejzheyvEsTvYogBPJnKUdNlrid6PwMnrA4WzOU0eWfOF6w6RRdnVM7n7DHWBFVAy7ocS-mAdM_GmwTvw4DV7kLdogg_e8",
  //         "image_large_url":
  //         "https://lh3.googleusercontent.com/KV7QzwejzheyvEsTvYogBPJnKUdNlrid6PwMnrA4WzOU0eWfOF6w6RRdnVM7n7DHWBFVAy7ocS-mAdM_GmwTvw4DV7kLdogg_e8=s1000",
  //         "image_opengraph_url":
  //         "https://lh3.googleusercontent.com/KV7QzwejzheyvEsTvYogBPJnKUdNlrid6PwMnrA4WzOU0eWfOF6w6RRdnVM7n7DHWBFVAy7ocS-mAdM_GmwTvw4DV7kLdogg_e8=k-w1200-s2400-rj",
  //         "blurhash": "UCBiFG+PX7aPz5tht3az%HowWWa#j0WVagj?",
  //         "predominant_color": "#5b99f3"
  //       },
  //       "image_url":
  //       "https://cdn.simplehash.com/assets/6e174a2e0091ffd5c0c63904366a62da8890508b01e7e85b13d5475b038e6544.svg",
  //       "image_properties": {
  //         "width": 1000,
  //         "height": 1000,
  //         "size": 101101,
  //         "mime_type": "image/svg+xml"
  //       },
  //       "video_url": null,
  //       "video_properties": null,
  //       "audio_url": null,
  //       "audio_properties": null,
  //       "model_url": null,
  //       "model_properties": null,
  //       "background_color": null,
  //       "external_url": "https://app.ens.domains/name/stochasticparrot.eth",
  //       "created_date": "2022-12-08T00:52:23",
  //       "status": "minted",
  //       "token_count": 1,
  //       "owner_count": 1,
  //       "owners": [
  //         {
  //           "owner_address": "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
  //           "quantity": 1,
  //           "first_acquired_date": "2022-12-08T00:52:23",
  //           "last_acquired_date": "2022-12-08T00:52:23"
  //         }
  //       ],
  //       "last_sale": null,
  //       "first_created": {
  //         "minted_to": "0x283af0b28c62c092c9727f1ee09c02ca627eb7f5",
  //         "quantity": 1,
  //         "timestamp": "2022-12-08T00:52:23",
  //         "block_number": 16136530,
  //         "transaction":
  //         "0xe06e9d1f6a3bfcc9f1fc6b9731524c69d09569cab746f2e24571a010d4ce99eb",
  //         "transaction_initiator":
  //         "0xb4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
  //       },
  //       "contract": {
  //         "type": "ERC721",
  //         "name": null,
  //         "symbol": null,
  //         "deployed_by": "0x4fe4e666be5752f1fdd210f4ab5de2cc26e3e0e8",
  //         "deployed_via_contract": null
  //       },
  //       "collection": {
  //         "collection_id": "e34baafc65deb66d52d11be5d44f523e",
  //         "name": "ENS: Ethereum Name Service",
  //         "description": "Ethereum Name Service (ENS) domains are secure
  //         domain names for the decentralized world. ENS domains provide a way
  //         for users to map human readable names to blockchain and
  //         non-blockchain resources, like Ethereum addresses, IPFS hashes, or
  //         website URLs. ENS domains can be bought and sold on secondary
  //         markets.", "image_url":
  //         "https://lh3.googleusercontent.com/yXNjPUCCTHyvYNarrb81ln31I6hUIaoPzlGU8kki-OohiWuqxfrIkMaOdLzcO4iGuXcvE5mgCZ-ds9tZotEJi3hdkNusheEK_w2V",
  //         "banner_image_url": null,
  //         "external_url": "https://ens.domains",
  //         "twitter_username": "ensdomains",
  //         "discord_url": null,
  //         "marketplace_pages": [
  //           {
  //             "marketplace_id": "opensea",
  //             "marketplace_name": "OpenSea",
  //             "marketplace_collection_id": "ens",
  //             "nft_url":
  //             "https://opensea.io/assets/ethereum/0x57f1887a8bf19b14fc0df6fd9b2acc9af147ea85/53762001732575849527995595036249427730510390651723189221519398504820492711584",
  //             "collection_url": "https://opensea.io/collection/ens",
  //             "verified": true
  //           }
  //         ],
  //         "metaplex_mint": null,
  //         "metaplex_first_verified_creator": null,
  //         "spam_score": 0,
  //         "floor_prices": [
  //           {
  //             "marketplace_id": "opensea",
  //             "marketplace_name": "OpenSea",
  //             "value": 1,
  //             "payment_token": {
  //               "payment_token_id": "ethereum.native",
  //               "name": "Ether",
  //               "symbol": "ETH",
  //               "address": null,
  //               "decimals": 18
  //             }
  //           }
  //         ],
  //         "distinct_owner_count": 667362,
  //         "distinct_nft_count": 2962658,
  //         "total_quantity": 2962620,
  //         "top_contracts": [
  //           "ethereum.0x57f1887a8bf19b14fc0df6fd9b2acc9af147ea85"
  //         ]
  //       },
  //       "rarity": {
  //         "rank": null,
  //         "score": null,
  //         "unique_attributes": null
  //       },
  //       "extra_metadata": {
  //         "attributes": [
  //           {
  //             "trait_type": "Created Date",
  //             "value": "1670460743000",
  //             "display_type": "date"
  //           },
  //           {
  //             "trait_type": "Length",
  //             "value": "16",
  //             "display_type": "number"
  //           },
  //           {
  //             "trait_type": "Segment Length",
  //             "value": "16",
  //             "display_type": "number"
  //           },
  //           {
  //             "trait_type": "Character Set",
  //             "value": "letter",
  //             "display_type": "string"
  //           },
  //           {
  //             "trait_type": "Registration Date",
  //             "value": "1670460743000",
  //             "display_type": "date"
  //           },
  //           {
  //             "trait_type": "Expiration Date",
  //             "value": "1733574647000",
  //             "display_type": "date"
  //           }
  //         ],
  //         "is_normalized": true,
  //         "name_length": 16,
  //         "segment_length": 16,
  //         "version": 0,
  //         "background_image":
  //         "https://metadata.ens.domains/mainnet/avatar/stochasticparrot.eth",
  //         "image_url":
  //         "https://metadata.ens.domains/mainnet/0x57f1887a8BF19b14fC0dF6Fd9B2acc9Af147eA85/0x76dc36f2ff546436694c7eee18598a1309f0d382934ac4fd977ed24f3b9bb6a0/image",
  //         "image_original_url":
  //         "https://metadata.ens.domains/mainnet/0x57f1887a8BF19b14fC0dF6Fd9B2acc9Af147eA85/0x76dc36f2ff546436694c7eee18598a1309f0d382934ac4fd977ed24f3b9bb6a0/image",
  //         "animation_original_url": null,
  //         "metadata_original_url":
  //         "https://metadata.ens.domains/mainnet/0x57f1887a8BF19b14fC0dF6Fd9B2acc9Af147eA85/53762001732575849527995595036249427730510390651723189221519398504820492711584/"
  //       }
  //     },
  //     ...
  // }

  // Only ETH and SOL NFTs are supported.
  if (!(coin == mojom::CoinType::ETH || coin == mojom::CoinType::SOL)) {
    return std::nullopt;
  }

  // If both skip_spam and only_spam are true, return early.
  if (skip_spam && only_spam) {
    return std::nullopt;
  }

  const base::Value::Dict* dict = json_value.GetIfDict();
  if (!dict) {
    return std::nullopt;
  }

  auto* next_cursor_ptr = dict->FindString("next_cursor");
  std::optional<std::string> next_cursor;
  if (next_cursor_ptr) {
    next_cursor = *next_cursor_ptr;
  } else {
    next_cursor = std::nullopt;
  }

  const base::Value::List* nfts = dict->FindList("nfts");
  if (!nfts) {
    return std::nullopt;
  }

  std::vector<mojom::BlockchainTokenPtr> nft_tokens;
  for (const auto& nft_value : *nfts) {
    auto token = mojom::BlockchainToken::New();

    const base::Value::Dict* nft = nft_value.GetIfDict();
    if (!nft) {
      continue;
    }
    auto* collection = nft->FindDict("collection");
    if (!collection) {
      continue;
    }
    std::optional<int> spam_score = collection->FindInt("spam_score");
    if (skip_spam && (!spam_score || *spam_score > 0)) {
      continue;
    }
    if (only_spam && (spam_score && *spam_score <= 0)) {
      continue;
    }

    // contract_address (required)
    auto* contract_address = nft->FindString("contract_address");
    if (!contract_address) {
      continue;
    }
    token->contract_address = *contract_address;

    // chain_id (required)
    auto* chain = nft->FindString("chain");
    if (!chain) {
      continue;
    }
    std::optional<std::string> chain_id = SimpleHashChainIdToChainId(*chain);
    if (!chain_id) {
      continue;
    }
    token->chain_id = *chain_id;

    // name
    auto* name = nft->FindString("name");
    if (name) {
      token->name = *name;
    }

    // logo
    auto* logo = nft->FindString("image_url");
    if (logo) {
      token->logo = *logo;
    }

    // is_erc20
    token->is_erc20 = false;

    // The contract dict has the standard information
    // so we skip if it's not there.
    auto* contract = nft->FindDict("contract");
    if (!contract) {
      continue;
    }
    auto* type = contract->FindString("type");
    if (!type) {
      continue;
    }

    // is_erc721
    if (coin == mojom::CoinType::ETH) {
      bool is_erc721 = base::EqualsCaseInsensitiveASCII(*type, "ERC721");
      if (!is_erc721) {
        continue;
      }
      token->is_erc721 = true;
    } else {  // mojom::CoinType::SOL
      // Solana NFTs must be "NonFungible", "NonFungibleEdition", or
      // "ProgrammableNonFungible"
      if (!(base::EqualsCaseInsensitiveASCII(*type, "NonFungible") ||
            base::EqualsCaseInsensitiveASCII(*type, "NonFungibleEdition") ||
            base::EqualsCaseInsensitiveASCII(*type,
                                             "ProgrammableNonFungible"))) {
        continue;
      }
      token->is_erc721 = false;
    }

    // is_erc1155 TODO(nvonpentz) Support ERC1155 tokens by parsing type above
    // https://github.com/brave/brave-browser/issues/29304
    token->is_erc1155 = false;

    // is_nft
    token->is_nft = true;

    // symbol
    auto* symbol = contract->FindString("symbol");
    if (!symbol) {
      // If symbol is null, assign an empty string to avoid display issues
      // on the frontend
      token->symbol = "";
    } else {
      token->symbol = *symbol;
    }

    // decimals
    token->decimals = 0;

    // visible
    token->visible = true;

    // token_id (required for ETH only)
    if (coin == mojom::CoinType::ETH) {
      auto* token_id = nft->FindString("token_id");
      if (!token_id) {
        continue;
      }
      uint256_t token_id_uint256;
      if (!Base10ValueToUint256(*token_id, &token_id_uint256)) {
        continue;
      }
      token->token_id = Uint256ValueToHex(token_id_uint256);
    }

    // coin
    token->coin = coin;

    nft_tokens.push_back(std::move(token));
  }

  return std::make_pair(next_cursor, std::move(nft_tokens));
}

// static
// Creates a URL like
// https://simplehash.wallet.brave.com/api/v0/nfts/owners?chains={chains}&wallet_addresses={wallet_addresses}
GURL SimpleHashClient::GetSimpleHashNftsByWalletUrl(
    const std::string& account_address,
    const std::vector<std::string>& chain_ids,
    const std::optional<std::string>& cursor) {
  if (chain_ids.empty() || account_address.empty()) {
    return GURL();
  }

  std::string urlStr =
      base::StrCat({kSimpleHashBraveProxyUrl, "/api/v0/nfts/owners"});

  std::string chain_ids_param;
  for (const auto& chain_id : chain_ids) {
    std::optional<std::string> simple_hash_chain_id =
        ChainIdToSimpleHashChainId(chain_id);
    if (simple_hash_chain_id) {
      if (!chain_ids_param.empty()) {
        chain_ids_param += ",";
      }
      chain_ids_param += *simple_hash_chain_id;
    }
  }

  if (chain_ids_param.empty()) {
    return GURL();
  }

  GURL url = GURL(urlStr);
  url = net::AppendQueryParameter(url, "chains", chain_ids_param);
  url = net::AppendQueryParameter(url, "wallet_addresses", account_address);

  // If cursor is provided, add it as a query parameter
  if (cursor) {
    url = net::AppendQueryParameter(url, "cursor", *cursor);
  }

  return url;
}

}  // namespace brave_wallet
