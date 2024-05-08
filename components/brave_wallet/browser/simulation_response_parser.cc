/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/simulation_response_parser.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/simulation_responses.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

namespace {

mojom::BlowfishWarningSeverity ParseWarningSeverity(
    const simulation_responses::WarningSeverity& severity) {
  switch (severity) {
    case simulation_responses::WarningSeverity::kCritical:
      return mojom::BlowfishWarningSeverity::kCritical;

    case simulation_responses::WarningSeverity::kWarning:
      return mojom::BlowfishWarningSeverity::kWarning;

    default:
      return mojom::BlowfishWarningSeverity::kWarning;
  }
}

mojom::BlowfishWarningKind ParseWarningKind(const std::string& kind) {
  if (kind == "APPROVAL_TO_EOA") {
    return mojom::BlowfishWarningKind::kApprovalToEOA;
  } else if (kind == "BLOCKLISTED_DOMAIN_CROSS_ORIGIN") {
    return mojom::BlowfishWarningKind::kBlocklistedDomainCrossOrigin;
  } else if (kind == "BLUR_BULK_ORDER_NOT_ON_BLUR") {
    return mojom::BlowfishWarningKind::kBlurBulkOrderNotOnBlur;
  } else if (kind == "BLUR_V2_ORDER_NOT_ON_BLUR") {
    return mojom::BlowfishWarningKind::kBlurV2OrderNotOnBlur;
  } else if (kind == "BULK_APPROVALS_REQUEST") {
    return mojom::BlowfishWarningKind::kBulkApprovalsRequest;
  } else if (kind == "COMPROMISED_AUTHORITY_UPGRADE") {
    return mojom::BlowfishWarningKind::kCompromisedAuthorityUpgrade;
  } else if (kind == "COPY_CAT_DOMAIN") {
    return mojom::BlowfishWarningKind::kCopyCatDomain;
  } else if (kind == "COPY_CAT_IMAGE_UNRESPONSIVE_DOMAIN") {
    return mojom::BlowfishWarningKind::kCopyCatImageUnresponsiveDomain;
  } else if (kind == "DANGLING_APPROVAL") {
    return mojom::BlowfishWarningKind::kDanglingApproval;
  } else if (kind == "DEBUGGER_PAUSED") {
    return mojom::BlowfishWarningKind::kDebuggerPaused;
  } else if (kind == "DURABLE_NONCE") {
    return mojom::BlowfishWarningKind::kDurableNonce;
  } else if (kind == "ETH_SIGN_TX_HASH") {
    return mojom::BlowfishWarningKind::kEthSignTxHash;
  } else if (kind == "FORTA") {
    return mojom::BlowfishWarningKind::kForta;
  } else if (kind == "IMBALANCED_DOLLAR_VALUE") {
    return mojom::BlowfishWarningKind::kImbalancedDollarValue;
  } else if (kind == "KNOWN_MALICIOUS") {
    return mojom::BlowfishWarningKind::kKnownMalicious;
  } else if (kind == "MALICIOUS_PACKAGES") {
    return mojom::BlowfishWarningKind::kMaliciousPackages;
  } else if (kind == "MULTI_COPY_CAT_DOMAIN") {
    return mojom::BlowfishWarningKind::kMultiCopyCatDomain;
  } else if (kind == "NEW_DOMAIN") {
    return mojom::BlowfishWarningKind::kNewDomain;
  } else if (kind == "PERMIT_NO_EXPIRATION") {
    return mojom::BlowfishWarningKind::kPermitNoExpiration;
  } else if (kind == "PERMIT_UNLIMITED_ALLOWANCE") {
    return mojom::BlowfishWarningKind::kPermitUnlimitedAllowance;
  } else if (kind == "POISONED_ADDRESS") {
    return mojom::BlowfishWarningKind::kPoisonedAddress;
  } else if (kind == "REFERENCED_OFAC_ADDRESS") {
    return mojom::BlowfishWarningKind::kReferencedOfacAddress;
  } else if (kind == "SEMI_TRUSTED_BLOCKLIST_DOMAIN") {
    return mojom::BlowfishWarningKind::kSemiTrustedBlocklistDomain;
  } else if (kind == "SET_OWNER_AUTHORITY") {
    return mojom::BlowfishWarningKind::kSetOwnerAuthority;
  } else if (kind == "SUSPECTED_MALICIOUS") {
    return mojom::BlowfishWarningKind::kSuspectedMalicious;
  } else if (kind == "TOO_MANY_TRANSACTIONS") {
    return mojom::BlowfishWarningKind::kTooManyTransactions;
  } else if (kind == "TRADE_FOR_NOTHING") {
    return mojom::BlowfishWarningKind::kTradeForNothing;
  } else if (kind == "TRANSFERRING_ERC20_TO_OWN_CONTRACT") {
    return mojom::BlowfishWarningKind::kTransferringErc20ToOwnContract;
  } else if (kind == "TRANSFERRING_TOO_MUCH_SOL") {
    return mojom::BlowfishWarningKind::kTransferringTooMuchSol;
  } else if (kind == "TRANSFERS_MAJORITY_OF_YOUR_SOL") {
    return mojom::BlowfishWarningKind::kTransfersMajorityOfYourSol;
  } else if (kind == "TRUSTED_BLOCKLIST_DOMAIN") {
    return mojom::BlowfishWarningKind::kTrustedBlocklistDomain;
  } else if (kind == "UNLIMITED_ALLOWANCE_TO_NFTS") {
    return mojom::BlowfishWarningKind::kUnlimitedAllowanceToNfts;
  } else if (kind == "UNUSUAL_GAS_CONSUMPTION") {
    return mojom::BlowfishWarningKind::kUnusualGasConsumption;
  } else if (kind == "USER_ACCOUNT_OWNER_CHANGE") {
    return mojom::BlowfishWarningKind::kUserAccountOwnerChange;
  } else if (kind == "TRANSFER_TO_MINT_ACCOUNT") {
    return mojom::BlowfishWarningKind::kTransferToMintAccount;
  } else if (kind == "WHITELISTED_DOMAIN_CROSS_ORIGIN") {
    return mojom::BlowfishWarningKind::kWhitelistedDomainCrossOrigin;
  } else if (kind == "YAKOA_NFT_IP_INFRINGEMENT") {
    return mojom::BlowfishWarningKind::kYakoaNftIpInfringement;
  } else if (kind == "RELIABLE_SIMULATION_NOT_POSSIBLE") {
    return mojom::BlowfishWarningKind::kReliableSimulationNotPossible;
  } else if (kind == "INTERACTING_WITH_ACCOUNT_WITHOUT_ACTIVITY") {
    return mojom::BlowfishWarningKind::kInteractingWithAccountWithoutActivity;
  } else {
    return mojom::BlowfishWarningKind::kUnknown;
  }
}

mojom::BlowfishAssetPriceSource ParseAssetPriceSource(
    const simulation_responses::AssetPriceSource& source) {
  switch (source) {
    case simulation_responses::AssetPriceSource::kCoingecko:
      return mojom::BlowfishAssetPriceSource::kCoingecko;

    case simulation_responses::AssetPriceSource::kDefillama:
      return mojom::BlowfishAssetPriceSource::kDefillama;

    case simulation_responses::AssetPriceSource::kSimplehash:
      return mojom::BlowfishAssetPriceSource::kSimplehash;

    default:
      return mojom::BlowfishAssetPriceSource::kUnknown;
  }
}

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

std::optional<std::string> ParseNullableString(const base::Value& value) {
  if (value.is_none()) {
    return "";
  }

  if (value.is_string()) {
    return value.GetString();
  }

  return std::nullopt;
}

std::optional<std::string> ParseOptionalNullableString(
    const std::optional<base::Value>& value) {
  if (!value.has_value()) {
    return "";
  }

  return ParseNullableString(value.value());
}

}  // namespace

namespace evm {

namespace {

mojom::BlowfishEVMErrorKind ParseErrorKind(
    const simulation_responses::EVMErrorKind& kind) {
  switch (kind) {
    case simulation_responses::EVMErrorKind::kSimulationFailed:
      return mojom::BlowfishEVMErrorKind::kSimulationFailed;

    case simulation_responses::EVMErrorKind::kTransactionError:
      return mojom::BlowfishEVMErrorKind::kTransactionError;

    case simulation_responses::EVMErrorKind::kTransactionReverted:
      return mojom::BlowfishEVMErrorKind::kTransactionReverted;

    case simulation_responses::EVMErrorKind::kUnknownError:
    default:
      return mojom::BlowfishEVMErrorKind::kUnknownError;
  }
}

mojom::BlowfishEVMAddressKind ParseBlowfishAddressKind(
    const simulation_responses::EVMAddressKind& kind) {
  switch (kind) {
    case simulation_responses::EVMAddressKind::kAccount:
      return mojom::BlowfishEVMAddressKind::kAccount;

    default:
      return mojom::BlowfishEVMAddressKind::kUnknown;
  }
}

mojom::BlowfishEVMRawInfoKind ParseRawInfoKind(
    const simulation_responses::EVMRawInfoKind& kind) {
  switch (kind) {
    case simulation_responses::EVMRawInfoKind::kAnyNftFromCollectionTransfer:
      return mojom::BlowfishEVMRawInfoKind::kAnyNftFromCollectionTransfer;

    case simulation_responses::EVMRawInfoKind::kErc1155ApprovalForAll:
      return mojom::BlowfishEVMRawInfoKind::kErc1155ApprovalForAll;

    case simulation_responses::EVMRawInfoKind::kErc1155Transfer:
      return mojom::BlowfishEVMRawInfoKind::kErc1155Transfer;

    case simulation_responses::EVMRawInfoKind::kErc20Approval:
      return mojom::BlowfishEVMRawInfoKind::kErc20Approval;

    case simulation_responses::EVMRawInfoKind::kErc20Transfer:
      return mojom::BlowfishEVMRawInfoKind::kErc20Transfer;

    case simulation_responses::EVMRawInfoKind::kErc721Approval:
      return mojom::BlowfishEVMRawInfoKind::kErc721Approval;

    case simulation_responses::EVMRawInfoKind::kErc721ApprovalForAll:
      return mojom::BlowfishEVMRawInfoKind::kErc721ApprovalForAll;

    case simulation_responses::EVMRawInfoKind::kErc721Transfer:
      return mojom::BlowfishEVMRawInfoKind::kErc721Transfer;

    case simulation_responses::EVMRawInfoKind::kNativeAssetTransfer:
      return mojom::BlowfishEVMRawInfoKind::kNativeAssetTransfer;

    default:
      return mojom::BlowfishEVMRawInfoKind::kUnknown;
  }
}

mojom::BlowfishEVMCounterpartyPtr ParseCounterparty(
    const simulation_responses::EVMCounterparty& value) {
  return mojom::BlowfishEVMCounterparty::New(
      value.address, ParseBlowfishAddressKind(value.kind));
}

mojom::BlowfishEVMCounterpartyPtr ParseCounterparty(const base::Value& value) {
  if (!value.is_dict()) {
    return nullptr;
  }

  const auto& counterparty_value =
      simulation_responses::EVMCounterparty::FromValue(value.GetDict());
  if (!counterparty_value) {
    return nullptr;
  }

  return ParseCounterparty(counterparty_value.value());
}

mojom::BlowfishEVMAmountPtr ParseAmount(
    const simulation_responses::EVMAmount& value) {
  return mojom::BlowfishEVMAmount::New(value.before, value.after);
}

mojom::BlowfishEVMAssetPtr ParseAsset(
    const simulation_responses::EVMAsset& value) {
  auto asset = mojom::BlowfishEVMAsset::New();
  asset->address = value.address;

  if (auto symbol = ParseOptionalNullableString(value.symbol)) {
    asset->symbol = symbol.value();
  } else {
    return nullptr;
  }

  if (auto name = ParseOptionalNullableString(value.name)) {
    asset->name = name.value();
  } else {
    return nullptr;
  }

  if (auto collection = ParseOptionalNullableString(value.collection)) {
    asset->collection = collection.value();
  } else {
    return nullptr;
  }

  if (!base::StringToInt(value.decimals.value_or("0"), &asset->decimals)) {
    return nullptr;
  }

  asset->verified = value.verified.value_or(false);
  asset->lists = value.lists.value_or(std::vector<std::string>{});
  asset->image_url = ParseOptionalNullableString(value.image_url).value_or("");
  asset->price = ParsePrice(value.price);
  return asset;
}

mojom::BlowfishEVMStateChangeRawInfoPtr ParseStateChangeRawInfo(
    const simulation_responses::EVMStateChangeRawInfo& value) {
  if (!value.data.is_dict()) {
    return nullptr;
  }

  auto raw_info = mojom::BlowfishEVMStateChangeRawInfo::New();
  raw_info->kind = ParseRawInfoKind(value.kind);

  if (value.kind == simulation_responses::EVMRawInfoKind::kErc20Transfer) {
    auto data_value = simulation_responses::ERC20TransferData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC20TransferData::New();
    data->amount = ParseAmount(data_value->amount);
    data->counterparty = ParseCounterparty(data_value->counterparty);

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }

    raw_info->data =
        mojom::BlowfishEVMStateChangeRawInfoDataUnion::NewErc20TransferData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::EVMRawInfoKind::kErc20Approval) {
    auto data_value = simulation_responses::ERC20ApprovalData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC20ApprovalData::New();
    if (auto counterparty = ParseCounterparty(data_value->owner)) {
      data->owner = std::move(counterparty);
    } else {
      return nullptr;
    }

    if (auto counterparty = ParseCounterparty(data_value->spender)) {
      data->spender = std::move(counterparty);
    } else {
      return nullptr;
    }

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
             simulation_responses::EVMRawInfoKind::kNativeAssetTransfer) {
    auto data_value = simulation_responses::NativeAssetTransferData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishNativeAssetTransferData::New();
    data->amount = ParseAmount(data_value->amount);
    data->counterparty = ParseCounterparty(data_value->counterparty);
    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }

    raw_info->data = mojom::BlowfishEVMStateChangeRawInfoDataUnion::
        NewNativeAssetTransferData(std::move(data));
  } else if (value.kind ==
             simulation_responses::EVMRawInfoKind::kErc721Transfer) {
    auto data_value = simulation_responses::ERC721TransferData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC721TransferData::New();
    data->amount = ParseAmount(data_value->amount);
    data->counterparty = ParseCounterparty(data_value->counterparty);
    data->metadata =
        mojom::BlowfishEVMMetadata::New(data_value->metadata.raw_image_url);

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }
    if (auto token_id = ParseNullableString(data_value->token_id)) {
      data->asset->token_id = token_id.value();
    } else {
      return nullptr;
    }

    raw_info->data =
        mojom::BlowfishEVMStateChangeRawInfoDataUnion::NewErc721TransferData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::EVMRawInfoKind::kErc721Approval) {
    auto data_value = simulation_responses::ERC721ApprovalData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC721ApprovalData::New();
    data->amount = ParseAmount(data_value->amount);
    data->metadata =
        mojom::BlowfishEVMMetadata::New(data_value->metadata.raw_image_url);

    if (auto counterparty = ParseCounterparty(data_value->owner)) {
      data->owner = std::move(counterparty);
    } else {
      return nullptr;
    }

    if (auto counterparty = ParseCounterparty(data_value->spender)) {
      data->spender = std::move(counterparty);
    } else {
      return nullptr;
    }

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }
    if (auto token_id = ParseNullableString(data_value->token_id)) {
      data->asset->token_id = token_id.value();
    } else {
      return nullptr;
    }

    raw_info->data =
        mojom::BlowfishEVMStateChangeRawInfoDataUnion::NewErc721ApprovalData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::EVMRawInfoKind::kErc721ApprovalForAll) {
    auto data_value = simulation_responses::ERC721ApprovalForAllData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC721ApprovalForAllData::New();
    data->amount = ParseAmount(data_value->amount);

    if (auto counterparty = ParseCounterparty(data_value->owner)) {
      data->owner = std::move(counterparty);
    } else {
      return nullptr;
    }

    if (auto counterparty = ParseCounterparty(data_value->spender)) {
      data->spender = std::move(counterparty);
    } else {
      return nullptr;
    }

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }

    raw_info->data = mojom::BlowfishEVMStateChangeRawInfoDataUnion::
        NewErc721ApprovalForAllData(std::move(data));
  } else if (value.kind ==
             simulation_responses::EVMRawInfoKind::kErc1155Transfer) {
    auto data_value = simulation_responses::ERC1155TransferData::FromValue(
        value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC1155TransferData::New();
    data->amount = ParseAmount(data_value->amount);
    data->metadata =
        mojom::BlowfishEVMMetadata::New(data_value->metadata.raw_image_url);
    data->counterparty = ParseCounterparty(data_value->counterparty);

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }
    if (auto token_id = ParseNullableString(data_value->token_id)) {
      data->asset->token_id = token_id.value();
    } else {
      return nullptr;
    }

    raw_info->data =
        mojom::BlowfishEVMStateChangeRawInfoDataUnion::NewErc1155TransferData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::EVMRawInfoKind::kErc1155ApprovalForAll) {
    auto data_value =
        simulation_responses::ERC1155ApprovalForAllData::FromValue(
            value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishERC1155ApprovalForAllData::New();
    data->amount = ParseAmount(data_value->amount);

    if (auto counterparty = ParseCounterparty(data_value->owner)) {
      data->owner = std::move(counterparty);
    } else {
      return nullptr;
    }

    if (auto counterparty = ParseCounterparty(data_value->spender)) {
      data->spender = std::move(counterparty);
    } else {
      return nullptr;
    }

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }

    raw_info->data = mojom::BlowfishEVMStateChangeRawInfoDataUnion::
        NewErc1155ApprovalForAllData(std::move(data));
  } else {
    return nullptr;
  }

  return raw_info;
}

mojom::BlowfishEVMStateChangePtr ParseStateChange(const base::Value& value) {
  if (!value.is_dict()) {
    return nullptr;
  }

  auto state_change_value =
      simulation_responses::EVMStateChange::FromValue(value.GetDict());
  if (!state_change_value) {
    return nullptr;
  }

  auto state_change = mojom::BlowfishEVMStateChange::New();
  state_change->human_readable_diff = state_change_value->human_readable_diff;

  if (auto raw_info = ParseStateChangeRawInfo(state_change_value->raw_info)) {
    state_change->raw_info = std::move(raw_info);
  } else {
    return nullptr;
  }

  return state_change;
}

}  // namespace

mojom::EVMSimulationResponsePtr ParseSimulationResponse(
    const base::Value& json_value,
    const std::string& user_account) {
  // {
  //   "action": "NONE",
  //   "warnings": [],
  //   "simulationResults": {
  //     "aggregated": {
  //       "expectedStateChanges": {
  //         "0x397ff1542f962076d0bfe58ea045ffa2d347aca0": [
  //           {
  //             "humanReadableDiff": "Send 1 ETH",
  //             "rawInfo": {
  //               "kind": "NATIVE_ASSET_TRANSFER",
  //               "data": {
  //                 "amount": {
  //                   "after": "1182957389356504134754",
  //                   "before": "1183957389356504134754"
  //                 },
  //                 "counterparty": {
  //                   "kind": "ACCOUNT",
  //                   "address": "0xd8da6bf26964af9d7eed9e03e53415d37aa96045"
  //                 },
  //                 "asset": {
  //                   "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
  //                   "symbol": "ETH",
  //                   "name": "Ether",
  //                   "decimals": "18",
  //                   "verified": true,
  //                   "imageUrl": "https://eth.png",
  //                   "price": {
  //                     "source": "Coingecko",
  //                     "updatedAt": "1681958792",
  //                     "dollarValuePerToken": "1945.92"
  //                   }
  //                 }
  //               }
  //             }
  //           }
  //         ]
  //       }
  //     }
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

  // Parse nullable field "error" of type EVMError.
  if (simulation_response_value->simulation_results.aggregated.error
          .is_dict()) {
    const auto& error_value = simulation_responses::EVMError::FromValue(
        simulation_response_value->simulation_results.aggregated.error
            .GetDict());
    if (!error_value) {
      return nullptr;
    }

    simulation_response->error = mojom::BlowfishEVMError::New(
        ParseErrorKind(error_value->kind), error_value->human_readable_error);
  } else if (simulation_response_value->simulation_results.aggregated.error
                 .is_none()) {
    simulation_response->error = nullptr;
  } else {
    return nullptr;
  }

  if (!simulation_response_value->simulation_results.aggregated
           .expected_state_changes.is_dict()) {
    return nullptr;
  }

  auto& expected_state_changes_value =
      simulation_response_value->simulation_results.aggregated
          .expected_state_changes.GetDict();

  auto* account_state_changes_value =
      expected_state_changes_value.FindList(user_account);
  if (!account_state_changes_value) {
    account_state_changes_value =
        expected_state_changes_value.FindList(base::ToLowerASCII(user_account));
    if (!account_state_changes_value) {
      return simulation_response;
    }
  }

  for (const auto& state_change_value : *account_state_changes_value) {
    if (auto state_change = ParseStateChange(state_change_value)) {
      simulation_response->expected_state_changes.push_back(
          std::move(state_change));
    } else {
      return nullptr;
    }
  }

  return simulation_response;
}

}  // namespace evm

namespace solana {

namespace {

mojom::BlowfishMetaplexTokenStandardKind ParseMetaplexTokenStandard(
    const std::optional<base::Value>& value) {
  if (!value || value->is_none() || !value->is_string()) {
    return mojom::BlowfishMetaplexTokenStandardKind::kUnknown;
  }

  const auto& kind =
      simulation_responses::ParseMetaplexTokenStandardKind(value->GetString());
  switch (kind) {
    case simulation_responses::MetaplexTokenStandardKind::kFungible:
      return mojom::BlowfishMetaplexTokenStandardKind::kFungible;

    case simulation_responses::MetaplexTokenStandardKind::kFungibleAsset:
      return mojom::BlowfishMetaplexTokenStandardKind::kFungibleAsset;

    case simulation_responses::MetaplexTokenStandardKind::kNonFungible:
      return mojom::BlowfishMetaplexTokenStandardKind::kNonFungible;

    case simulation_responses::MetaplexTokenStandardKind::kNonFungibleEdition:
      return mojom::BlowfishMetaplexTokenStandardKind::kNonFungibleEdition;

    default:
      return mojom::BlowfishMetaplexTokenStandardKind::kUnknown;
  }
}

mojom::BlowfishSolanaRawInfoKind ParseRawInfoKind(
    const simulation_responses::SolanaRawInfoKind& kind) {
  switch (kind) {
    case simulation_responses::SolanaRawInfoKind::kSolStakeAuthorityChange:
      return mojom::BlowfishSolanaRawInfoKind::kSolStakeAuthorityChange;

    case simulation_responses::SolanaRawInfoKind::kSolTransfer:
      return mojom::BlowfishSolanaRawInfoKind::kSolTransfer;

    case simulation_responses::SolanaRawInfoKind::kSplApproval:
      return mojom::BlowfishSolanaRawInfoKind::kSplApproval;

    case simulation_responses::SolanaRawInfoKind::kSplTransfer:
      return mojom::BlowfishSolanaRawInfoKind::kSplTransfer;

    case simulation_responses::SolanaRawInfoKind::kUserAccountOwnerChange:
      return mojom::BlowfishSolanaRawInfoKind::kUserAccountOwnerChange;

    default:
      return mojom::BlowfishSolanaRawInfoKind::kUnknown;
  }
}

mojom::BlowfishDiffSign ParseDiffSign(
    const simulation_responses::DiffSign& sign) {
  switch (sign) {
    case simulation_responses::DiffSign::kMinus:
      return mojom::BlowfishDiffSign::kMinus;

    case simulation_responses::DiffSign::kPlus:
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

mojom::BlowfishSolanaAssetPtr ParseAsset(
    const simulation_responses::SolanaAsset& value) {
  auto asset = mojom::BlowfishSolanaAsset::New();

  asset->symbol = value.symbol;
  asset->name = value.name;
  asset->mint = value.mint.value_or("");

  if (!base::StringToInt(value.decimals, &asset->decimals)) {
    return nullptr;
  }

  asset->image_url = ParseOptionalNullableString(value.image_url).value_or("");
  asset->price = ParsePrice(value.price);

  asset->metaplex_token_standard =
      ParseMetaplexTokenStandard(value.metaplex_token_standard);

  return asset;
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
  raw_info->kind = ParseRawInfoKind(value.kind);

  if (value.kind == simulation_responses::SolanaRawInfoKind::kSolTransfer) {
    auto data_value =
        simulation_responses::SOLTransferData::FromValue(value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishSOLTransferData::New();

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }

    if (auto diff = ParseDiff(data_value->diff)) {
      data->diff = std::move(diff);
    } else {
      return nullptr;
    }

    raw_info->data =
        mojom::BlowfishSolanaStateChangeRawInfoDataUnion::NewSolTransferData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::SolanaRawInfoKind::kSplTransfer) {
    auto data_value =
        simulation_responses::SPLTransferData::FromValue(value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishSPLTransferData::New();

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }

    if (auto diff = ParseDiff(data_value->diff)) {
      data->diff = std::move(diff);
    } else {
      return nullptr;
    }

    data->counterparty =
        ParseNullableString(data_value->counterparty).value_or("");

    raw_info->data =
        mojom::BlowfishSolanaStateChangeRawInfoDataUnion::NewSplTransferData(
            std::move(data));
  } else if (value.kind ==
             simulation_responses::SolanaRawInfoKind::kSplApproval) {
    auto data_value =
        simulation_responses::SPLApprovalData::FromValue(value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishSPLApprovalData::New();
    data->delegate = data_value->delegate;

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }

    if (auto diff = ParseDiff(data_value->diff)) {
      data->diff = std::move(diff);
    } else {
      return nullptr;
    }

    raw_info->data =
        mojom::BlowfishSolanaStateChangeRawInfoDataUnion::NewSplApprovalData(
            std::move(data));
  } else if (value.kind == simulation_responses::SolanaRawInfoKind::
                               kSolStakeAuthorityChange) {
    auto data_value =
        simulation_responses::SOLStakeAuthorityChangeData::FromValue(
            value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishSOLStakeAuthorityChangeData::New();
    data->stake_account = data_value->stake_account;
    data->current_authorities =
        ParseStakeAuthorities(data_value->current_authorities);
    data->future_authorities =
        ParseStakeAuthorities(data_value->future_authorities);

    if (auto asset = ParseAsset(data_value->asset)) {
      data->asset = std::move(asset);
    } else {
      return nullptr;
    }

    if (!base::StringToUint64(data_value->sol_staked, &data->sol_staked)) {
      return nullptr;
    }

    raw_info->data = mojom::BlowfishSolanaStateChangeRawInfoDataUnion::
        NewSolStakeAuthorityChangeData(std::move(data));
  } else if (value.kind ==
             simulation_responses::SolanaRawInfoKind::kUserAccountOwnerChange) {
    auto data_value =
        simulation_responses::SolanaUserAccountOwnerChangeData::FromValue(
            value.data.GetDict());
    if (!data_value) {
      return nullptr;
    }

    auto data = mojom::BlowfishSolanaUserAccountOwnerChangeData::New();
    data->account = data_value->account;
    data->current_owner = data_value->current_owner;
    data->future_owner = data_value->future_owner;

    raw_info->data = mojom::BlowfishSolanaStateChangeRawInfoDataUnion::
        NewUserAccountOwnerChangeData(std::move(data));
  } else {
    return nullptr;
  }

  return raw_info;
}

mojom::BlowfishSuggestedColor ParseSuggestedColor(
    const simulation_responses::SuggestedColor& color) {
  switch (color) {
    case simulation_responses::SuggestedColor::kCredit:
      return mojom::BlowfishSuggestedColor::kCredit;

    case simulation_responses::SuggestedColor::kDebit:
      return mojom::BlowfishSuggestedColor::kDebit;

    case simulation_responses::SuggestedColor::kInfo:
    default:
      return mojom::BlowfishSuggestedColor::kInfo;
  }
}

// Detects documented error kinds (see:
// https://docs.blowfish.xyz/v2023-03-08/reference/scan-transactions-solana)
mojom::BlowfishSolanaErrorKind ParseErrorKind(const std::string& error) {
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

mojom::BlowfishSolanaStateChangePtr ParseStateChange(const base::Value& value) {
  if (!value.is_dict()) {
    return nullptr;
  }

  auto state_change_value =
      simulation_responses::SolanaStateChange::FromValue(value.GetDict());
  if (!state_change_value) {
    return nullptr;
  }

  auto state_change = mojom::BlowfishSolanaStateChange::New();
  state_change->human_readable_diff = state_change_value->human_readable_diff;
  state_change->suggested_color =
      ParseSuggestedColor(state_change_value->suggested_color);

  if (auto raw_info = ParseStateChangeRawInfo(state_change_value->raw_info)) {
    state_change->raw_info = std::move(raw_info);
  } else {
    return nullptr;
  }

  return state_change;
}

}  // namespace

mojom::SolanaSimulationResponsePtr ParseSimulationResponse(
    const base::Value& json_value,
    const std::string& user_account) {
  // {
  //   "aggregated": {
  //     "action": "WARN",
  //     "warnings": [
  //       {
  //         "severity": "WARNING",
  //         "kind": "SUSPECTED_MALICIOUS",
  //         "message": "Domain found on blocklists maintained by: Blowfish.
  //         This website is very likely to be a scam."
  //       }
  //     ],
  //     "error": {
  //       "kind": "BAD_REQUEST",
  //       "humanReadableError": "User account supplied in request not found"
  //     },
  //     "expectedStateChanges": {
  //       "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT": [
  //         {
  //           "humanReadableDiff": "Receive 0.05657 SOL",
  //           "suggestedColor": "CREDIT",
  //           "rawInfo": {
  //             "kind": "SOL_TRANSFER",
  //             "data": {
  //               "asset": {
  //                 "symbol": "SOL",
  //                 "name": "Solana Native Token",
  //                 "decimals": "9",
  //                 "price": {
  //                   "source": "Coingecko",
  //                   "updatedAt": "1679331222",
  //                   "dollarValuePerToken": "0.2784"
  //                 },
  //                 "imageUrl": "https://sol.png"
  //               },
  //               "diff": {
  //                 "sign": "PLUS",
  //                 "digits": "500000"
  //               }
  //             }
  //           }
  //         }
  //       ]
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
      ParseBlowfishActionKind(simulation_response_value->aggregated.action);
  simulation_response->warnings =
      ParseWarnings(simulation_response_value->aggregated.warnings);

  // Parse nullable field "error" of type SolanaError.
  if (simulation_response_value->aggregated.error.is_dict()) {
    const auto& error_value = simulation_responses::SolanaError::FromValue(
        simulation_response_value->aggregated.error.GetDict());
    if (!error_value) {
      return nullptr;
    }

    simulation_response->error = mojom::BlowfishSolanaError::New(
        ParseErrorKind(error_value->kind), error_value->human_readable_error);
  } else if (simulation_response_value->aggregated.error.is_none()) {
    simulation_response->error = nullptr;
  } else {
    return nullptr;
  }

  if (!simulation_response_value->aggregated.expected_state_changes.is_dict()) {
    return nullptr;
  }

  auto& expected_state_changes_value =
      simulation_response_value->aggregated.expected_state_changes.GetDict();

  auto* account_state_changes_value =
      expected_state_changes_value.FindList(user_account);
  if (!account_state_changes_value) {
    account_state_changes_value =
        expected_state_changes_value.FindList(base::ToLowerASCII(user_account));
    if (!account_state_changes_value) {
      return simulation_response;
    }
  }

  for (const auto& state_change_value : *account_state_changes_value) {
    if (auto state_change = ParseStateChange(state_change_value)) {
      simulation_response->expected_state_changes.push_back(
          std::move(state_change));
    } else {
      return nullptr;
    }
  }

  return simulation_response;
}

}  // namespace solana

std::optional<std::string> ParseSimulationErrorResponse(
    const base::Value& json_value) {
  if (!json_value.is_dict()) {
    return std::nullopt;
  }

  auto error_response =
      simulation_responses::HTTPError::FromValue(json_value.GetDict());
  if (!error_response) {
    return std::nullopt;
  }

  return error_response->error;
}

}  // namespace brave_wallet
