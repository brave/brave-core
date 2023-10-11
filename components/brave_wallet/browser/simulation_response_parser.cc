/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/simulation_response_parser.h"
#include "brave/components/brave_wallet/browser/simulation_responses.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

namespace {

mojom::BlowfishWarningSeverity ParseWarningSeverity(
    const simulation_responses::WarningSeverity& severity) {
  switch (severity) {
    case simulation_responses::WarningSeverity::WARNING_SEVERITY_CRITICAL:
      return mojom::BlowfishWarningSeverity::kCritical;

    case simulation_responses::WarningSeverity::WARNING_SEVERITY_WARNING:
      return mojom::BlowfishWarningSeverity::kWarning;

    default:
      return mojom::BlowfishWarningSeverity::kWarning;
  }
}

mojom::BlowfishWarningKind ParseWarningKind(
    const simulation_responses::WarningKind& kind) {
  switch (kind) {
    case simulation_responses::WarningKind::WARNING_KIND_APPROVAL_TO_E_O_A:
      return mojom::BlowfishWarningKind::kApprovalToEOA;

    case simulation_responses::WarningKind::
        WARNING_KIND_BLOCKLISTED_DOMAIN_CROSS_ORIGIN:
      return mojom::BlowfishWarningKind::kBlocklistedDomainCrossOrigin;

    case simulation_responses::WarningKind::WARNING_KIND_BULK_APPROVALS_REQUEST:
      return mojom::BlowfishWarningKind::kBulkApprovalsRequest;

    case simulation_responses::WarningKind::
        WARNING_KIND_COMPROMISED_AUTHORITY_UPGRADE:
      return mojom::BlowfishWarningKind::kCompromisedAuthorityUpgrade;

    case simulation_responses::WarningKind::WARNING_KIND_COPY_CAT_DOMAIN:
      return mojom::BlowfishWarningKind::kCopyCatDomain;

    case simulation_responses::WarningKind::
        WARNING_KIND_COPY_CAT_IMAGE_UNRESPONSIVE_DOMAIN:
      return mojom::BlowfishWarningKind::kCopyCatImageUnresponsiveDomain;

    case simulation_responses::WarningKind::WARNING_KIND_DANGLING_APPROVAL:
      return mojom::BlowfishWarningKind::kDanglingApproval;

    case simulation_responses::WarningKind::WARNING_KIND_DEVTOOLS_DISABLED:
      return mojom::BlowfishWarningKind::kDevtoolsDisabled;

    case simulation_responses::WarningKind::WARNING_KIND_ETH_SIGN_TX_HASH:
      return mojom::BlowfishWarningKind::kEthSignTxHash;

    case simulation_responses::WarningKind::WARNING_KIND_KNOWN_MALICIOUS:
      return mojom::BlowfishWarningKind::kKnownMalicious;

    case simulation_responses::WarningKind::
        WARNING_KIND_MAINNET_REPLAY_POSSIBLE:
      return mojom::BlowfishWarningKind::kMainnetReplayPossible;

    case simulation_responses::WarningKind::WARNING_KIND_MULTI_COPY_CAT_DOMAIN:
      return mojom::BlowfishWarningKind::kMultiCopyCatDomain;

    case simulation_responses::WarningKind::WARNING_KIND_NEW_DOMAIN:
      return mojom::BlowfishWarningKind::kNewDomain;

    case simulation_responses::WarningKind::WARNING_KIND_NON_ASCII_URL:
      return mojom::BlowfishWarningKind::kNonAsciiUrl;

    case simulation_responses::WarningKind::WARNING_KIND_OBFUSCATED_CODE:
      return mojom::BlowfishWarningKind::kObfuscatedCode;

    case simulation_responses::WarningKind::WARNING_KIND_PERMIT_NO_EXPIRATION:
      return mojom::BlowfishWarningKind::kPermitNoExpiration;

    case simulation_responses::WarningKind::
        WARNING_KIND_PERMIT_UNLIMITED_ALLOWANCE:
      return mojom::BlowfishWarningKind::kPermitUnlimitedAllowance;

    case simulation_responses::WarningKind::WARNING_KIND_POISONED_ADDRESS:
      return mojom::BlowfishWarningKind::kPoisonedAddress;

    case simulation_responses::WarningKind::
        WARNING_KIND_REFERENCED_OFAC_ADDRESS:
      return mojom::BlowfishWarningKind::kReferencedOfacAddress;

    case simulation_responses::WarningKind::
        WARNING_KIND_SEMI_TRUSTED_BLOCKLIST_DOMAIN:
      return mojom::BlowfishWarningKind::kSemiTrustedBlocklistDomain;

    case simulation_responses::WarningKind::WARNING_KIND_SET_OWNER_AUTHORITY:
      return mojom::BlowfishWarningKind::kSetOwnerAuthority;

    case simulation_responses::WarningKind::WARNING_KIND_SUSPECTED_MALICIOUS:
      return mojom::BlowfishWarningKind::kSuspectedMalicious;

    case simulation_responses::WarningKind::WARNING_KIND_TOO_MANY_TRANSACTIONS:
      return mojom::BlowfishWarningKind::kTooManyTransactions;

    case simulation_responses::WarningKind::WARNING_KIND_TRADE_FOR_NOTHING:
      return mojom::BlowfishWarningKind::kTradeForNothing;

    case simulation_responses::WarningKind::
        WARNING_KIND_TRANSFERRING_ERC20_TO_OWN_CONTRACT:
      return mojom::BlowfishWarningKind::kTransferringErc20ToOwnContract;

    case simulation_responses::WarningKind::
        WARNING_KIND_TRUSTED_BLOCKLIST_DOMAIN:
      return mojom::BlowfishWarningKind::kTrustedBlocklistDomain;

    case simulation_responses::WarningKind::
        WARNING_KIND_UNLIMITED_ALLOWANCE_TO_NFTS:
      return mojom::BlowfishWarningKind::kUnlimitedAllowanceToNfts;

    case simulation_responses::WarningKind::
        WARNING_KIND_UNUSUAL_GAS_CONSUMPTION:
      return mojom::BlowfishWarningKind::kUnusualGasConsumption;

    case simulation_responses::WarningKind::
        WARNING_KIND_USER_ACCOUNT_OWNER_CHANGE:
      return mojom::BlowfishWarningKind::kUserAccountOwnerChange;

    case simulation_responses::WarningKind::
        WARNING_KIND_WHITELISTED_DOMAIN_CROSS_ORIGIN:
      return mojom::BlowfishWarningKind::kWhitelistedDomainCrossOrigin;

    default:
      return mojom::BlowfishWarningKind::kUnknown;
  }
}

mojom::BlowfishAssetPriceSource ParseAssetPriceSource(
    const simulation_responses::AssetPriceSource& source) {
  switch (source) {
    case simulation_responses::AssetPriceSource::ASSET_PRICE_SOURCE_COINGECKO:
      return mojom::BlowfishAssetPriceSource::kCoingecko;

    case simulation_responses::AssetPriceSource::ASSET_PRICE_SOURCE_DEFILLAMA:
      return mojom::BlowfishAssetPriceSource::kDefillama;

    case simulation_responses::AssetPriceSource::ASSET_PRICE_SOURCE_SIMPLEHASH:
      return mojom::BlowfishAssetPriceSource::kSimplehash;

    default:
      return mojom::BlowfishAssetPriceSource::kUnknown;
  }
}

mojom::BlowfishEVMRawInfoKind ParseEVMRawInfoKind(
    const simulation_responses::EVMRawInfoKind& kind) {
  switch (kind) {
    case simulation_responses::EVMRawInfoKind::
        EVM_RAW_INFO_KIND_ANY_NFT_FROM_COLLECTION_TRANSFER:
      return mojom::BlowfishEVMRawInfoKind::kAnyNftFromCollectionTransfer;

    case simulation_responses::EVMRawInfoKind::
        EVM_RAW_INFO_KIND_ERC1155_APPROVAL_FOR_ALL:
      return mojom::BlowfishEVMRawInfoKind::kErc1155ApprovalForAll;

    case simulation_responses::EVMRawInfoKind::
        EVM_RAW_INFO_KIND_ERC1155_TRANSFER:
      return mojom::BlowfishEVMRawInfoKind::kErc1155Transfer;

    case simulation_responses::EVMRawInfoKind::EVM_RAW_INFO_KIND_ERC20_APPROVAL:
      return mojom::BlowfishEVMRawInfoKind::kErc20Approval;

    case simulation_responses::EVMRawInfoKind::EVM_RAW_INFO_KIND_ERC20_TRANSFER:
      return mojom::BlowfishEVMRawInfoKind::kErc20Transfer;

    case simulation_responses::EVMRawInfoKind::
        EVM_RAW_INFO_KIND_ERC721_APPROVAL:
      return mojom::BlowfishEVMRawInfoKind::kErc721Approval;

    case simulation_responses::EVMRawInfoKind::
        EVM_RAW_INFO_KIND_ERC721_APPROVAL_FOR_ALL:
      return mojom::BlowfishEVMRawInfoKind::kErc721ApprovalForAll;

    case simulation_responses::EVMRawInfoKind::
        EVM_RAW_INFO_KIND_ERC721_TRANSFER:
      return mojom::BlowfishEVMRawInfoKind::kErc721Transfer;

    case simulation_responses::EVMRawInfoKind::
        EVM_RAW_INFO_KIND_NATIVE_ASSET_TRANSFER:
      return mojom::BlowfishEVMRawInfoKind::kNativeAssetTransfer;

    default:
      return mojom::BlowfishEVMRawInfoKind::kUnknown;
  }
}

mojom::BlowfishSolanaRawInfoKind ParseSolanaRawInfoKind(
    const simulation_responses::SolanaRawInfoKind& kind) {
  switch (kind) {
    case simulation_responses::SolanaRawInfoKind::
        SOLANA_RAW_INFO_KIND_SOL_STAKE_AUTHORITY_CHANGE:
      return mojom::BlowfishSolanaRawInfoKind::kSolStakeAuthorityChange;

    case simulation_responses::SolanaRawInfoKind::
        SOLANA_RAW_INFO_KIND_SOL_TRANSFER:
      return mojom::BlowfishSolanaRawInfoKind::kSolTransfer;

    case simulation_responses::SolanaRawInfoKind::
        SOLANA_RAW_INFO_KIND_SPL_APPROVAL:
      return mojom::BlowfishSolanaRawInfoKind::kSplApproval;

    case simulation_responses::SolanaRawInfoKind::
        SOLANA_RAW_INFO_KIND_SPL_TRANSFER:
      return mojom::BlowfishSolanaRawInfoKind::kSplTransfer;

    case simulation_responses::SolanaRawInfoKind::
        SOLANA_RAW_INFO_KIND_USER_ACCOUNT_OWNER_CHANGE:
      return mojom::BlowfishSolanaRawInfoKind::kUserAccountOwnerChange;

    default:
      return mojom::BlowfishSolanaRawInfoKind::kUnknown;
  }
}

mojom::BlowfishMetaplexTokenStandardKind ParseMetaplexTokenStandardKind(
    const simulation_responses::MetaplexTokenStandardKind& kind) {
  switch (kind) {
    case simulation_responses::MetaplexTokenStandardKind::
        METAPLEX_TOKEN_STANDARD_KIND_FUNGIBLE:
      return mojom::BlowfishMetaplexTokenStandardKind::kFungible;

    case simulation_responses::MetaplexTokenStandardKind::
        METAPLEX_TOKEN_STANDARD_KIND_FUNGIBLE_ASSET:
      return mojom::BlowfishMetaplexTokenStandardKind::kFungibleAsset;

    case simulation_responses::MetaplexTokenStandardKind::
        METAPLEX_TOKEN_STANDARD_KIND_NON_FUNGIBLE:
      return mojom::BlowfishMetaplexTokenStandardKind::kNonFungible;

    case simulation_responses::MetaplexTokenStandardKind::
        METAPLEX_TOKEN_STANDARD_KIND_NON_FUNGIBLE_EDITION:
      return mojom::BlowfishMetaplexTokenStandardKind::kNonFungibleEdition;

    default:
      return mojom::BlowfishMetaplexTokenStandardKind::kUnknown;
  }
}

mojom::BlowfishEVMErrorKind ParseEVMErrorKind(
    const simulation_responses::EVMErrorKind& kind) {
  switch (kind) {
    case simulation_responses::EVMErrorKind::EVM_ERROR_KIND_SIMULATION_FAILED:
      return mojom::BlowfishEVMErrorKind::kSimulationFailed;

    case simulation_responses::EVMErrorKind::EVM_ERROR_KIND_TRANSACTION_ERROR:
      return mojom::BlowfishEVMErrorKind::kTransactionError;

    case simulation_responses::EVMErrorKind::
        EVM_ERROR_KIND_TRANSACTION_REVERTED:
      return mojom::BlowfishEVMErrorKind::kTransactionReverted;

    case simulation_responses::EVMErrorKind::EVM_ERROR_KIND_UNKNOWN_ERROR:
    default:
      return mojom::BlowfishEVMErrorKind::kUnknownError;
  }
}

mojom::BlowfishEVMAddressKind ParseBlowfishEVMAddressKind(
    const simulation_responses::EVMAddressKind& kind) {
  switch (kind) {
    case simulation_responses::EVMAddressKind::EVM_ADDRESS_KIND_ACCOUNT:
      return mojom::BlowfishEVMAddressKind::kAccount;

    default:
      return mojom::BlowfishEVMAddressKind::kUnknown;
  }
}

mojom::BlowfishSuggestedAction ParseBlowfishActionKind(
    const std::string& action) {
  if (action == "BLOCK") {
    return mojom::BlowfishSuggestedAction::kBlock;
  }
  if (action == "WARN") {
    return mojom::BlowfishSuggestedAction::kWarn;
  }
  if (action == "NONE") {
    return mojom::BlowfishSuggestedAction::kNone;
  }
  return mojom::BlowfishSuggestedAction::kNone;
}

std::vector<mojom::BlowfishWarningPtr> ParseWarnings(
    const std::vector<simulation_responses::Warning>& values) {
  std::vector<mojom::BlowfishWarningPtr> warnings;
  for (const auto& warning : values) {
    warnings.push_back(mojom::BlowfishWarning::New(
        ParseWarningSeverity(warning.severity), ParseWarningKind(warning.kind),
        warning.message));
  }
  return warnings;
}

absl::optional<std::string> ParseNullableString(const base::Value& value) {
  if (value.is_string()) {
    return value.GetString();
  }

  return absl::nullopt;
}

}  // namespace

namespace evm {

namespace {

mojom::BlowfishPricePtr ParsePrice(const base::Value& value) {
  if (value.is_dict()) {
    const auto& price_value =
        simulation_responses::Price::FromValue(value.GetDict());
    if (!price_value) {
      return nullptr;
    }

    return mojom::BlowfishPrice::New(ParseAssetPriceSource(price_value->source),
                                     price_value->updated_at,
                                     price_value->dollar_value_per_token);
  }

  return nullptr;
}

mojom::BlowfishEVMContractPtr ParseContract(
    const simulation_responses::EVMContract& value) {
  return mojom::BlowfishEVMContract::New(
      value.address, ParseBlowfishEVMAddressKind(value.kind));
}

mojom::BlowfishEVMAmountPtr ParseAmount(
    const simulation_responses::EVMAmount& value) {
  return mojom::BlowfishEVMAmount::New(value.before, value.after);
}

mojom::BlowfishEVMAssetPtr ParseAsset(
    const simulation_responses::EVMAsset& value) {
  auto asset = mojom::BlowfishEVMAsset::New();
  asset->address = value.address;
  asset->symbol = value.symbol;
  asset->name = value.name;

  if (!base::StringToInt(value.decimals, &asset->decimals)) {
    return nullptr;
  }

  asset->verified = value.verified;

  if (value.lists) {
    asset->lists = *value.lists;
  } else {
    asset->lists = {};
  }

  asset->image_url = ParseNullableString(value.image_url);
  asset->price = ParsePrice(value.price);
  return asset;
}

mojom::BlowfishEVMStateChangeRawInfoPtr ParseStateChangeRawInfo(
    const simulation_responses::EVMStateChangeRawInfo& value) {
  if (!value.data.is_dict()) {
    return nullptr;
  }

  auto raw_info = mojom::BlowfishEVMStateChangeRawInfo::New();
  raw_info->kind = ParseEVMRawInfoKind(value.kind);

  if (value.kind == simulation_responses::EVM_RAW_INFO_KIND_ERC20_TRANSFER) {
    auto data_value = simulation_responses::ERC20TransferData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC20TransferData::New();
    data->contract = ParseContract(data_value->contract);
    data->amount = ParseAmount(data_value->amount);

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }

    raw_info->data =
        mojom::BlowfishEVMStateChangeRawInfoDataUnion::NewErc20TransferData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::EVM_RAW_INFO_KIND_ERC20_APPROVAL) {
    auto data_value = simulation_responses::ERC20ApprovalData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC20ApprovalData::New();
    data->contract = ParseContract(data_value->contract);
    data->owner = ParseContract(data_value->owner);
    data->spender = ParseContract(data_value->spender);
    data->amount = ParseAmount(data_value->amount);

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }

    raw_info->data =
        mojom::BlowfishEVMStateChangeRawInfoDataUnion::NewErc20ApprovalData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::EVM_RAW_INFO_KIND_NATIVE_ASSET_TRANSFER) {
    auto data_value = simulation_responses::NativeAssetTransferData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishNativeAssetTransferData::New();
    data->amount = ParseAmount(data_value->amount);
    data->contract = ParseContract(data_value->contract);
    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }

    raw_info->data = mojom::BlowfishEVMStateChangeRawInfoDataUnion::
        NewNativeAssetTransferData(std::move(data));
  } else if (value.kind ==
             simulation_responses::EVM_RAW_INFO_KIND_ERC721_TRANSFER) {
    auto data_value = simulation_responses::ERC721TransferData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC721TransferData::New();
    data->amount = ParseAmount(data_value->amount);
    data->contract = ParseContract(data_value->contract);
    data->metadata =
        mojom::BlowfishEVMMetadata::New(data_value->metadata.raw_image_url);
    data->name = data_value->name;
    data->symbol = data_value->symbol;
    data->token_id = ParseNullableString(data_value->token_id);
    data->asset_price = ParsePrice(data_value->asset_price);

    raw_info->data =
        mojom::BlowfishEVMStateChangeRawInfoDataUnion::NewErc721TransferData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::EVM_RAW_INFO_KIND_ERC721_APPROVAL) {
    auto data_value = simulation_responses::ERC721ApprovalData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC721ApprovalData::New();
    data->amount = ParseAmount(data_value->amount);
    data->contract = ParseContract(data_value->contract);
    data->metadata =
        mojom::BlowfishEVMMetadata::New(data_value->metadata.raw_image_url);
    data->name = data_value->name;
    data->owner = ParseContract(data_value->owner);
    data->spender = ParseContract(data_value->spender);
    data->symbol = data_value->symbol;
    data->token_id = ParseNullableString(data_value->token_id);
    data->asset_price = ParsePrice(data_value->asset_price);

    raw_info->data =
        mojom::BlowfishEVMStateChangeRawInfoDataUnion::NewErc721ApprovalData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::EVM_RAW_INFO_KIND_ERC721_APPROVAL_FOR_ALL) {
    auto data_value = simulation_responses::ERC721ApprovalForAllData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC721ApprovalForAllData::New();
    data->amount = ParseAmount(data_value->amount);
    data->contract = ParseContract(data_value->contract);
    data->name = data_value->name;
    data->owner = ParseContract(data_value->owner);
    data->spender = ParseContract(data_value->spender);
    data->symbol = data_value->symbol;
    data->asset_price = ParsePrice(data_value->asset_price);

    raw_info->data = mojom::BlowfishEVMStateChangeRawInfoDataUnion::
        NewErc721ApprovalForAllData(std::move(data));
  } else if (value.kind ==
             simulation_responses::EVM_RAW_INFO_KIND_ERC1155_TRANSFER) {
    auto data_value = simulation_responses::ERC1155TransferData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC1155TransferData::New();
    data->amount = ParseAmount(data_value->amount);
    data->contract = ParseContract(data_value->contract);
    data->metadata =
        mojom::BlowfishEVMMetadata::New(data_value->metadata.raw_image_url);
    data->token_id = ParseNullableString(data_value->token_id);
    data->asset_price = ParsePrice(data_value->asset_price);
    data->name = data_value->name;

    raw_info->data =
        mojom::BlowfishEVMStateChangeRawInfoDataUnion::NewErc1155TransferData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::EVM_RAW_INFO_KIND_ERC1155_APPROVAL_FOR_ALL) {
    auto data_value =
        simulation_responses::ERC1155ApprovalForAllData::FromValue(
            value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC1155ApprovalForAllData::New();
    data->amount = ParseAmount(data_value->amount);
    data->contract = ParseContract(data_value->contract);
    data->owner = ParseContract(data_value->owner);
    data->spender = ParseContract(data_value->spender);
    data->asset_price = ParsePrice(data_value->asset_price);

    raw_info->data = mojom::BlowfishEVMStateChangeRawInfoDataUnion::
        NewErc1155ApprovalForAllData(std::move(data));
  } else {
    return nullptr;
  }

  return raw_info;
}

}  // namespace

mojom::EVMSimulationResponsePtr ParseSimulationResponse(
    const base::Value& json_value) {
  // {
  //   "action": "NONE",
  //   "warnings": [],
  //   "simulationResults": {
  //     "error": null,
  //     "gas": {
  //       "gasLimit": null
  //     },
  //     "expectedStateChanges": [
  //       {
  //         "humanReadableDiff": "Send 1 ETH",
  //         "rawInfo": {
  //           "kind": "NATIVE_ASSET_TRANSFER",
  //           "data": {
  //             "amount": {
  //               "after": "1182957389356504134754",
  //               "before": "1183957389356504134754"
  //             },
  //             "contract": {
  //               "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
  //               "kind": "ACCOUNT"
  //             },
  //             "asset": {
  //               "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
  //               "symbol": "ETH",
  //               "name": "Ether",
  //               "decimals": 18,
  //               "verified": true,
  //               "imageUrl":
  //               "https://d1ts37qlq4uz4s.cloudfront.net/evm__evm%3A%3Aethereum__evm%3A%3Aethereum%3A%3Amainnet__0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2.png",
  //               "price": {
  //                 "source": "Coingecko",
  //                 "updatedAt": 1681958792,
  //                 "dollarValuePerToken": 1945.92
  //               }
  //             }
  //           }
  //         }
  //       }
  //     ]
  //   }
  // }
  if (!json_value.is_dict()) {
    return nullptr;
  }

  auto simulation_response_value =
      simulation_responses::EVMSimulationResponse::FromValue(
          json_value.GetDict());
  if (!simulation_response_value) {
    return nullptr;
  }

  auto simulation_response = mojom::EVMSimulationResponse::New();
  simulation_response->action =
      ParseBlowfishActionKind(simulation_response_value->action);
  simulation_response->warnings =
      ParseWarnings(simulation_response_value->warnings);

  auto simulation_results = mojom::EVMSimulationResults::New();

  // Parse nullable field "error" of type EVMError.
  if (simulation_response_value->simulation_results.error.is_dict()) {
    const auto& error_value = simulation_responses::EVMError::FromValue(
        simulation_response_value->simulation_results.error.GetDict());
    if (!error_value) {
      return nullptr;
    }

    simulation_results->error =
        mojom::BlowfishEVMError::New(ParseEVMErrorKind(error_value->kind),
                                     error_value->human_readable_error);
  } else if (simulation_response_value->simulation_results.error.is_none()) {
    simulation_results->error = nullptr;
  } else {
    return nullptr;
  }

  for (const auto& state_change_value :
       simulation_response_value->simulation_results.expected_state_changes) {
    auto state_change = mojom::BlowfishEVMStateChange::New();
    state_change->human_readable_diff = state_change_value.human_readable_diff;

    if (auto raw_info = ParseStateChangeRawInfo(state_change_value.raw_info)) {
      state_change->raw_info = std::move(raw_info);
    } else {
      return nullptr;
    }

    simulation_results->expected_state_changes.push_back(
        std::move(state_change));
  }

  simulation_response->simulation_results = std::move(simulation_results);

  return simulation_response;
}

}  // namespace evm

namespace solana {

namespace {

mojom::BlowfishPricePtr ParsePrice(const base::Value& value) {
  if (value.is_dict()) {
    const auto& price_value =
        simulation_responses::SolanaPrice::FromValue(value.GetDict());
    if (!price_value) {
      return nullptr;
    }

    return mojom::BlowfishPrice::New(ParseAssetPriceSource(price_value->source),
                                     price_value->last_updated_at,
                                     price_value->dollar_value_per_token);
  }

  return nullptr;
}

mojom::BlowfishDiffSign ParseDiffSign(
    const simulation_responses::DiffSign& sign) {
  switch (sign) {
    case simulation_responses::DiffSign::DIFF_SIGN_MINUS:
      return mojom::BlowfishDiffSign::kMinus;

    case simulation_responses::DiffSign::DIFF_SIGN_PLUS:
    default:
      return mojom::BlowfishDiffSign::kPlus;
  }
}

mojom::BlowfishSolanaDiffPtr ParseDiff(
    const simulation_responses::SolanaDiff& value) {
  auto diff = mojom::BlowfishSolanaDiff::New();
  diff->sign = ParseDiffSign(value.sign);
  if (!base::StringToUint64(value.digits, &diff->digits)) {
    return nullptr;
  }

  return diff;
}

mojom::BlowfishSolanaStakeAuthoritiesPtr ParseStakeAuthorities(
    const simulation_responses::SolanaStakeAuthorities& value) {
  auto authorities = mojom::BlowfishSolanaStakeAuthorities::New();
  authorities->staker = value.staker;
  authorities->withdrawer = value.withdrawer;
  return authorities;
}

mojom::BlowfishSolanaStateChangeRawInfoPtr ParseStateChangeRawInfo(
    const simulation_responses::SolanaStateChangeRawInfo& value) {
  if (!value.data.is_dict()) {
    return nullptr;
  }

  auto raw_info = mojom::BlowfishSolanaStateChangeRawInfo::New();
  raw_info->kind = ParseSolanaRawInfoKind(value.kind);

  if (value.kind == simulation_responses::SOLANA_RAW_INFO_KIND_SOL_TRANSFER) {
    auto data_value =
        simulation_responses::SOLTransferData::FromValue(value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishSOLTransferData::New();
    data->symbol = data_value->symbol;
    data->name = data_value->name;

    if (!base::StringToInt(data_value->decimals, &data->decimals)) {
      return nullptr;
    }
    data->diff = ParseDiff(data_value->diff);

    raw_info->data =
        mojom::BlowfishSolanaStateChangeRawInfoDataUnion::NewSolTransferData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::SOLANA_RAW_INFO_KIND_SPL_TRANSFER) {
    auto data_value =
        simulation_responses::SPLTransferData::FromValue(value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishSPLTransferData::New();
    data->symbol = data_value->symbol;
    data->name = data_value->name;
    data->mint = data_value->mint;
    if (!base::StringToInt(data_value->decimals, &data->decimals)) {
      return nullptr;
    }
    data->diff = ParseDiff(data_value->diff);
    if (!base::StringToUint64(data_value->supply, &data->supply)) {
      return nullptr;
    }
    data->metaplex_token_standard =
        ParseMetaplexTokenStandardKind(data_value->metaplex_token_standard);
    data->asset_price = ParsePrice(data_value->asset_price);

    raw_info->data =
        mojom::BlowfishSolanaStateChangeRawInfoDataUnion::NewSplTransferData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::SOLANA_RAW_INFO_KIND_SPL_APPROVAL) {
    auto data_value =
        simulation_responses::SPLApprovalData::FromValue(value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishSPLApprovalData::New();
    data->delegate = data_value->delegate;
    data->mint = data_value->mint;
    data->symbol = data_value->symbol;
    data->name = data_value->name;

    if (!base::StringToInt(data_value->decimals, &data->decimals)) {
      return nullptr;
    }

    data->diff = ParseDiff(data_value->diff);
    if (!base::StringToUint64(data_value->supply, &data->supply)) {
      return nullptr;
    }
    data->metaplex_token_standard =
        ParseMetaplexTokenStandardKind(data_value->metaplex_token_standard);
    data->asset_price = ParsePrice(data_value->asset_price);

    raw_info->data =
        mojom::BlowfishSolanaStateChangeRawInfoDataUnion::NewSplApprovalData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::
                 SOLANA_RAW_INFO_KIND_SOL_STAKE_AUTHORITY_CHANGE) {
    auto data_value =
        simulation_responses::SOLStakeAuthorityChangeData::FromValue(
            value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishSOLStakeAuthorityChangeData::New();
    data->stake_account = data_value->stake_account;
    data->curr_authorities =
        ParseStakeAuthorities(data_value->curr_authorities);
    data->future_authorities =
        ParseStakeAuthorities(data_value->future_authorities);
    data->symbol = data_value->symbol;
    data->name = data_value->name;

    if (!base::StringToInt(data_value->decimals, &data->decimals)) {
      return nullptr;
    }

    if (!base::StringToUint64(data_value->sol_staked, &data->sol_staked)) {
      return nullptr;
    }
    raw_info->data = mojom::BlowfishSolanaStateChangeRawInfoDataUnion::
        NewSolStakeAuthorityChangeData(std::move(data));
  } else {
    return nullptr;
  }

  return raw_info;
}

mojom::BlowfishSuggestedColor ParseSuggestedColor(
    const simulation_responses::SuggestedColor& color) {
  switch (color) {
    case simulation_responses::SuggestedColor::SUGGESTED_COLOR_CREDIT:
      return mojom::BlowfishSuggestedColor::kCredit;

    case simulation_responses::SuggestedColor::SUGGESTED_COLOR_DEBIT:
    default:
      return mojom::BlowfishSuggestedColor::kDebit;
  }
}

// Detects documented error kinds (see:
// https://docs.blowfish.xyz/v2023-03-08/reference/scan-transactions-solana)
mojom::BlowfishSolanaErrorKind ParseSolanaErrorKind(const std::string& error) {
  // ERROR_PROCESSING_INSTRUCTION_{0}:_{1}
  if (error.find("ERROR_PROCESSING_INSTRUCTION") == 0) {
    return mojom::BlowfishSolanaErrorKind::kErrorProcessingInstruction;
  }

  // "TRANSACTION_CONTAINS_A_DUPLICATE_INSTRUCTION_({0})_THAT_IS_NOT_ALLOWED"
  if (error.find("TRANSACTION_CONTAINS_A_DUPLICATE_INSTRUCTION_") == 0) {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionContainsADuplicateInstructionThatIsNotAllowed;
  }

  if (error == "ACCOUNT_DOES_NOT_HAVE_ENOUGH_SOL_TO_PERFORM_THE_OPERATION") {
    return mojom::BlowfishSolanaErrorKind::
        kAccountDoesNotHaveEnoughSolToPerformTheOperation;
  } else if (error == "ACCOUNT_DOES_NOT_SUPPORT_SPECIFIED_AUTHORITY_TYPE") {
    return mojom::BlowfishSolanaErrorKind::
        kAccountDoesNotSupportSpecifiedAuthorityType;
  } else if (error == "ACCOUNT_IN_USE") {
    return mojom::BlowfishSolanaErrorKind::kAccountInUse;
  } else if (error == "ACCOUNT_IS_FROZEN") {
    return mojom::BlowfishSolanaErrorKind::kAccountIsFrozen;
  } else if (error == "ACCOUNT_LOADED_TWICE") {
    return mojom::BlowfishSolanaErrorKind::kAccountLoadedTwice;
  } else if (error == "ACCOUNT_NOT_ASSOCIATED_WITH_THIS_MINT") {
    return mojom::BlowfishSolanaErrorKind::kAccountNotAssociatedWithThisMint;
  } else if (error ==
             "ADVANCING_STORED_NONCE_REQUIRES_A_POPULATED_RECENTBLOCKHASHES_"
             "SYSVAR") {
    return mojom::BlowfishSolanaErrorKind::
        kAdvancingStoredNonceRequiresAPopulatedRecentblockhashesSysvar;
  } else if (error == "ALREADY_IN_USE") {
    return mojom::BlowfishSolanaErrorKind::kAlreadyInUse;
  } else if (error == "AN_ACCOUNT_WITH_THE_SAME_ADDRESS_ALREADY_EXISTS") {
    return mojom::BlowfishSolanaErrorKind::
        kAnAccountWithTheSameAddressAlreadyExists;
  } else if (error ==
             "ATTEMPT_TO_DEBIT_AN_ACCOUNT_BUT_FOUND_NO_RECORD_OF_A_PRIOR_"
             "CREDIT") {
    return mojom::BlowfishSolanaErrorKind::
        kAttemptToDebitAnAccountButFoundNoRecordOfAPriorCredit;
  } else if (error == "ATTEMPT_TO_LOAD_A_PROGRAM_THAT_DOES_NOT_EXIST") {
    return mojom::BlowfishSolanaErrorKind::
        kAttemptToLoadAProgramThatDoesNotExist;
  } else if (error == "BAD_REQUEST") {
    return mojom::BlowfishSolanaErrorKind::kBadRequest;
  } else if (error == "BLOCKHASH_NOT_FOUND") {
    return mojom::BlowfishSolanaErrorKind::kBlockhashNotFound;
  } else if (error == "CANNOT_ALLOCATE_ACCOUNT_DATA_OF_THIS_LENGTH") {
    return mojom::BlowfishSolanaErrorKind::
        kCannotAllocateAccountDataOfThisLength;
  } else if (error == "CANNOT_ASSIGN_ACCOUNT_TO_THIS_PROGRAM_ID") {
    return mojom::BlowfishSolanaErrorKind::kCannotAssignAccountToThisProgramId;
  } else if (error == "FIXED_SUPPLY") {
    return mojom::BlowfishSolanaErrorKind::kFixedSupply;
  } else if (error == "INSTRUCTION_DOES_NOT_SUPPORT_NATIVE_TOKENS") {
    return mojom::BlowfishSolanaErrorKind::
        kInstructionDoesNotSupportNativeTokens;
  } else if (error == "INSTRUCTION_DOES_NOT_SUPPORT_NON-NATIVE_TOKENS") {
    return mojom::BlowfishSolanaErrorKind::
        kInstructionDoesNotSupportNonNativeTokens;
  } else if (error == "INSUFFICIENT_FUNDS") {
    return mojom::BlowfishSolanaErrorKind::kInsufficientFunds;
  } else if (error == "INSUFFICIENT_FUNDS_FOR_FEE") {
    return mojom::BlowfishSolanaErrorKind::kInsufficientFundsForFee;
  } else if (error == "INVALID_INSTRUCTION") {
    return mojom::BlowfishSolanaErrorKind::kInvalidInstruction;
  } else if (error == "INVALID_MINT") {
    return mojom::BlowfishSolanaErrorKind::kInvalidMint;
  } else if (error == "INVALID_NUMBER_OF_PROVIDED_SIGNERS") {
    return mojom::BlowfishSolanaErrorKind::kInvalidNumberOfProvidedSigners;
  } else if (error == "INVALID_NUMBER_OF_REQUIRED_SIGNERS") {
    return mojom::BlowfishSolanaErrorKind::kInvalidNumberOfRequiredSigners;
  } else if (error == "LAMPORT_BALANCE_BELOW_RENT-EXEMPT_THRESHOLD") {
    return mojom::BlowfishSolanaErrorKind::
        kLamportBalanceBelowRentExemptThreshold;
  } else if (error == "LENGTH_OF_REQUESTED_SEED_IS_TOO_LONG") {
    return mojom::BlowfishSolanaErrorKind::kLengthOfRequestedSeedIsTooLong;
  } else if (error == "LOADER_CALL_CHAIN_IS_TOO_DEEP") {
    return mojom::BlowfishSolanaErrorKind::kLoaderCallChainIsTooDeep;
  } else if (error ==
             "NON-NATIVE_ACCOUNT_CAN_ONLY_BE_CLOSED_IF_ITS_BALANCE_IS_ZERO") {
    return mojom::BlowfishSolanaErrorKind::
        kNonNativeAccountCanOnlyBeClosedIfItsBalanceIsZero;
  } else if (error == "OPERATION_OVERFLOWED") {
    return mojom::BlowfishSolanaErrorKind::kOperationOverflowed;
  } else if (error == "OWNER_DOES_NOT_MATCH") {
    return mojom::BlowfishSolanaErrorKind::kOwnerDoesNotMatch;
  } else if (error ==
             "PROVIDED_ADDRESS_DOES_NOT_MATCH_ADDRESSED_DERIVED_FROM_SEED") {
    return mojom::BlowfishSolanaErrorKind::
        kProvidedAddressDoesNotMatchAddressedDerivedFromSeed;
  } else if (error == "SIMULATION_FAILED") {
    return mojom::BlowfishSolanaErrorKind::kSimulationFailed;
  } else if (error == "SIMULATION_TIMED_OUT") {
    return mojom::BlowfishSolanaErrorKind::kSimulationTimedOut;
  } else if (error == "SPECIFIED_NONCE_DOES_NOT_MATCH_STORED_NONCE") {
    return mojom::BlowfishSolanaErrorKind::
        kSpecifiedNonceDoesNotMatchStoredNonce;
  } else if (error == "STATE_IS_INVALID_FOR_REQUESTED_OPERATION") {
    return mojom::BlowfishSolanaErrorKind::kStateIsInvalidForRequestedOperation;
  } else if (error == "STATE_IS_UNINITIALIZED") {
    return mojom::BlowfishSolanaErrorKind::kStateIsUninitialized;
  } else if (error == "STORED_NONCE_IS_STILL_IN_RECENT_BLOCKHASHES") {
    return mojom::BlowfishSolanaErrorKind::
        kStoredNonceIsStillInRecentBlockhashes;
  } else if (error ==
             "THE_PROVIDED_DECIMALS_VALUE_DIFFERENT_FROM_THE_MINT_DECIMALS") {
    return mojom::BlowfishSolanaErrorKind::
        kTheProvidedDecimalsValueDifferentFromTheMintDecimals;
  } else if (error == "THIS_ACCOUNT_MAY_NOT_BE_USED_TO_PAY_TRANSACTION_FEES") {
    return mojom::BlowfishSolanaErrorKind::
        kThisAccountMayNotBeUsedToPayTransactionFees;
  } else if (error ==
             "THIS_PROGRAM_MAY_NOT_BE_USED_FOR_EXECUTING_INSTRUCTIONS") {
    return mojom::BlowfishSolanaErrorKind::
        kThisProgramMayNotBeUsedForExecutingInstructions;
  } else if (error == "THIS_TOKEN_MINT_CANNOT_FREEZE_ACCOUNTS") {
    return mojom::BlowfishSolanaErrorKind::kThisTokenMintCannotFreezeAccounts;
  } else if (error == "THIS_TRANSACTION_HAS_ALREADY_BEEN_PROCESSED") {
    return mojom::BlowfishSolanaErrorKind::
        kThisTransactionHasAlreadyBeenProcessed;
  } else if (error == "TOO_MANY_TRANSACTIONS") {
    return mojom::BlowfishSolanaErrorKind::kTooManyTransactions;
  } else if (error ==
             "TRANSACTION_ADDRESS_TABLE_LOOKUP_USES_AN_INVALID_INDEX") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionAddressTableLookupUsesAnInvalidIndex;
  } else if (error == "TRANSACTION_CONTAINS_AN_INVALID_ACCOUNT_REFERENCE") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionContainsAnInvalidAccountReference;
  } else if (error == "TRANSACTION_DID_NOT_PASS_SIGNATURE_VERIFICATION") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionDidNotPassSignatureVerification;
  } else if (error ==
             "TRANSACTION_FAILED_TO_SANITIZE_ACCOUNTS_OFFSETS_CORRECTLY") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionFailedToSanitizeAccountsOffsetsCorrectly;
  } else if (error ==
             "TRANSACTION_LEAVES_AN_ACCOUNT_WITH_A_LOWER_BALANCE_THAN_RENT-"
             "EXEMPT_MINIMUM") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionLeavesAnAccountWithALowerBalanceThanRentExemptMinimum;
  } else if (error ==
             "TRANSACTION_LOADS_A_WRITABLE_ACCOUNT_THAT_CANNOT_BE_WRITTEN") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionLoadsAWritableAccountThatCannotBeWritten;
  } else if (error ==
             "TRANSACTION_LOADS_AN_ADDRESS_TABLE_ACCOUNT_THAT_DOESN'T_EXIST") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionLoadsAnAddressTableAccountThatDoesntExist;
  } else if (error ==
             "TRANSACTION_LOADS_AN_ADDRESS_TABLE_ACCOUNT_WITH_AN_INVALID_"
             "OWNER") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionLoadsAnAddressTableAccountWithAnInvalidOwner;
  } else if (error ==
             "TRANSACTION_LOADS_AN_ADDRESS_TABLE_ACCOUNT_WITH_INVALID_DATA") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionLoadsAnAddressTableAccountWithInvalidData;
  } else if (error == "TRANSACTION_LOCKED_TOO_MANY_ACCOUNTS") {
    return mojom::BlowfishSolanaErrorKind::kTransactionLockedTooManyAccounts;
  } else if (error ==
             "TRANSACTION_PROCESSING_LEFT_AN_ACCOUNT_WITH_AN_OUTSTANDING_"
             "BORROWED_REFERENCE") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionProcessingLeftAnAccountWithAnOutstandingBorrowedReference;
  } else if (error ==
             "TRANSACTION_REQUIRES_A_FEE_BUT_HAS_NO_SIGNATURE_PRESENT") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionRequiresAFeeButHasNoSignaturePresent;
  } else if (error == "TRANSACTION_VERSION_IS_UNSUPPORTED") {
    return mojom::BlowfishSolanaErrorKind::kTransactionVersionIsUnsupported;
  } else if (error ==
             "TRANSACTION_WOULD_EXCEED_ACCOUNT_DATA_LIMIT_WITHIN_THE_BLOCK") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionWouldExceedAccountDataLimitWithinTheBlock;
  } else if (error ==
             "TRANSACTION_WOULD_EXCEED_MAX_ACCOUNT_LIMIT_WITHIN_THE_BLOCK") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionWouldExceedMaxAccountLimitWithinTheBlock;
  } else if (error == "TRANSACTION_WOULD_EXCEED_MAX_BLOCK_COST_LIMIT") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionWouldExceedMaxBlockCostLimit;
  } else if (error == "TRANSACTION_WOULD_EXCEED_MAX_VOTE_COST_LIMIT") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionWouldExceedMaxVoteCostLimit;
  } else if (error == "TRANSACTION_WOULD_EXCEED_TOTAL_ACCOUNT_DATA_LIMIT") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionWouldExceedTotalAccountDataLimit;
  } else if (error ==
             "TRANSACTIONS_ARE_CURRENTLY_DISABLED_DUE_TO_CLUSTER_MAINTENANCE") {
    return mojom::BlowfishSolanaErrorKind::
        kTransactionsAreCurrentlyDisabledDueToClusterMaintenance;
  } else if (error == "UNKNOWN_ERROR") {
    return mojom::BlowfishSolanaErrorKind::kUnknownError;
  }

  return mojom::BlowfishSolanaErrorKind::kUnknownError;
}

}  // namespace

mojom::SolanaSimulationResponsePtr ParseSimulationResponse(
    const base::Value& json_value) {
  // {
  //   "status": "CHECKS_PASSED",
  //   "action": "NONE",
  //   "warnings": [],
  //   "simulationResults": {
  //     "isRecentBlockhashExpired": false,
  //     "expectedStateChanges": [
  //       {
  //         "humanReadableDiff": "Send 2 USDT",
  //         "suggestedColor": "DEBIT",
  //         "rawInfo": {
  //           "kind": "SPL_TRANSFER",
  //           "data": {
  //             "symbol": "USDT",
  //             "name": "USDT",
  //             "mint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
  //             "decimals": 6,
  //             "supply": 1000000000,
  //             "metaplexTokenStandard": "unknown",
  //             "assetPrice": {
  //               "source": "Coingecko",
  //               "last_updated_at": 1679331222,
  //               "dollar_value_per_token": 0.99
  //             },
  //             "diff": {
  //               "sign": "MINUS",
  //               "digits": 2000000
  //             }
  //           }
  //         }
  //       }
  //     ],
  //     "error": null,
  //     "raw": {
  //       "err": null,
  //       "logs": [],
  //       "accounts": [],
  //       "returnData": null,
  //       "unitsConsumed": 148013
  //     }
  //   }
  // }

  if (!json_value.is_dict()) {
    return nullptr;
  }

  auto simulation_response_value =
      simulation_responses::SolanaSimulationResponse::FromValue(
          json_value.GetDict());
  if (!simulation_response_value) {
    return nullptr;
  }

  auto simulation_response = mojom::SolanaSimulationResponse::New();
  simulation_response->action =
      ParseBlowfishActionKind(simulation_response_value->action);
  simulation_response->warnings =
      ParseWarnings(simulation_response_value->warnings);

  auto simulation_results = mojom::SolanaSimulationResults::New();

  // Parse nullable field "error" of type SolanaError.
  if (simulation_response_value->simulation_results.error.is_dict()) {
    const auto& error_value = simulation_responses::SolanaError::FromValue(
        simulation_response_value->simulation_results.error.GetDict());
    if (!error_value) {
      return nullptr;
    }

    simulation_results->error =
        mojom::BlowfishSolanaError::New(ParseSolanaErrorKind(error_value->kind),
                                        error_value->human_readable_error);
  } else if (simulation_response_value->simulation_results.error.is_none()) {
    simulation_results->error = nullptr;
  } else {
    return nullptr;
  }

  for (const auto& state_change_value :
       simulation_response_value->simulation_results.expected_state_changes) {
    auto state_change = mojom::BlowfishSolanaStateChange::New();
    state_change->human_readable_diff = state_change_value.human_readable_diff;
    state_change->suggested_color =
        ParseSuggestedColor(state_change_value.suggested_color);

    if (auto raw_info = ParseStateChangeRawInfo(state_change_value.raw_info)) {
      state_change->raw_info = std::move(raw_info);
    } else {
      return nullptr;
    }

    simulation_results->expected_state_changes.push_back(
        std::move(state_change));
  }

  simulation_response->simulation_results = std::move(simulation_results);

  return simulation_response;
}

}  // namespace solana

absl::optional<std::string> ParseSimulationErrorResponse(
    const base::Value& json_value) {
  if (!json_value.is_dict()) {
    return absl::nullopt;
  }

  auto error_response =
      simulation_responses::HTTPError::FromValue(json_value.GetDict());
  if (!error_response) {
    return absl::nullopt;
  }

  return error_response->error;
}

}  // namespace brave_wallet
