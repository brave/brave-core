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

std::vector<mojom::BlowfishWarningPtr> ParseWarnings(
    const std::vector<simulation_responses::Warning>& values) {
  std::vector<mojom::BlowfishWarningPtr> warnings;
  for (const auto& warning : values) {
    warnings.push_back(mojom::BlowfishWarning::New(
        warning.severity, warning.kind, warning.message));
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

    return mojom::BlowfishPrice::New(price_value->source,
                                     price_value->updated_at,
                                     price_value->dollar_value_per_token);
  }

  return nullptr;
}

mojom::BlowfishEVMContractPtr ParseContract(
    const simulation_responses::EVMContract& value) {
  return mojom::BlowfishEVMContract::New(value.address, value.kind);
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
  raw_info->kind = value.kind;

  if (value.kind == "ERC20_TRANSFER") {
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
  } else if (value.kind == "ERC20_APPROVAL") {
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
  } else if (value.kind == "NATIVE_ASSET_TRANSFER") {
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
  } else if (value.kind == "ERC721_TRANSFER") {
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
  } else if (value.kind == "ERC721_APPROVAL") {
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
  } else if (value.kind == "ERC721_APPROVAL_FOR_ALL") {
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
  } else if (value.kind == "ERC1155_TRANSFER") {
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
  } else if (value.kind == "ERC1155_APPROVAL_FOR_ALL") {
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
  simulation_response->action = simulation_response_value->action;
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

    simulation_results->error = mojom::BlowfishEVMError::New(
        error_value->kind, error_value->human_readable_error);
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

    return mojom::BlowfishPrice::New(price_value->source,
                                     price_value->last_updated_at,
                                     price_value->dollar_value_per_token);
  }

  return nullptr;
}

mojom::BlowfishSolanaDiffPtr ParseDiff(
    const simulation_responses::SolanaDiff& value) {
  auto diff = mojom::BlowfishSolanaDiff::New();
  diff->sign = value.sign;
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
  raw_info->kind = value.kind;

  if (value.kind == "SOL_TRANSFER") {
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
  } else if (value.kind == "SPL_TRANSFER") {
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
    data->metaplex_token_standard = data_value->metaplex_token_standard;
    data->asset_price = ParsePrice(data_value->asset_price);

    raw_info->data =
        mojom::BlowfishSolanaStateChangeRawInfoDataUnion::NewSplTransferData(
            std::move(data));
  } else if (value.kind == "SPL_APPROVAL") {
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
    data->metaplex_token_standard = data_value->metaplex_token_standard;
    data->asset_price = ParsePrice(data_value->asset_price);

    raw_info->data =
        mojom::BlowfishSolanaStateChangeRawInfoDataUnion::NewSplApprovalData(
            std::move(data));
  } else if (value.kind == "SOL_STAKE_AUTHORITY_CHANGE") {
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
  simulation_response->action = simulation_response_value->action;
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

    simulation_results->error = mojom::BlowfishSolanaError::New(
        error_value->kind, error_value->human_readable_error);
  } else if (simulation_response_value->simulation_results.error.is_none()) {
    simulation_results->error = nullptr;
  } else {
    return nullptr;
  }

  for (const auto& state_change_value :
       simulation_response_value->simulation_results.expected_state_changes) {
    auto state_change = mojom::BlowfishSolanaStateChange::New();
    state_change->human_readable_diff = state_change_value.human_readable_diff;
    state_change->suggested_color = state_change_value.suggested_color;

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
