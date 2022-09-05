/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_pin_service.h"

#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task/bind_post_task.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pin/ipfs_remote_pin_service.h"
#include "components/prefs/scoped_user_pref_update.h"

using brave_wallet::mojom::BlockchainToken;
using brave_wallet::mojom::BlockchainTokenPtr;

namespace brave_wallet {


// const char kAssetsItemsDictKey[] = "items";

const char kAssetStatus[] = "status";
const char kValidateTimestamp[] = "validate_timestamp";
const char kAssetUrlListKey[] = "urls";

namespace {

std::string GetPath(const absl::optional<std::string>& service,
                    const BlockchainTokenPtr& token) {
  return base::StrCat({"nft", ".", service.value_or("local"), ".",
                       base::NumberToString(static_cast<int>(token->coin)), ".",
                       token->chain_id, ".", token->contract_address, ".",
                       token->token_id});
}

std::string GetGroupName(const absl::optional<std::string>& service,
                         const BlockchainTokenPtr& token) {
  return base::StrCat({"nft", "/", service.value_or("local"), "/",
                       base::NumberToString(static_cast<int>(token->coin)), "/",
                       token->chain_id, "/", token->contract_address, "/",
                       token->token_id});
}

std::string StatusToString(const mojom::TokenPinStatusCode& status) {
  switch (status) {
    case mojom::TokenPinStatusCode::STATUS_NOT_PINNED:
      return "not_pinned";
    case mojom::TokenPinStatusCode::STATUS_PINNED:
      return "pinned";
    case mojom::TokenPinStatusCode::STATUS_PINNING_IN_PROGRESS:
      return "pinning_in_progress";
    case mojom::TokenPinStatusCode::STATUS_UNPINNING_IN_PROGRESS:
      return "unpining_in_progress";
    case mojom::TokenPinStatusCode::STATUS_UNPINNING_FAILED:
      return "unpining_failed";
    case mojom::TokenPinStatusCode::STATUS_PINNING_FAILED:
      return "pining_failed";
    case mojom::TokenPinStatusCode::STATUS_PINNING_PENDING:
      return "pinning_pendig";
    case mojom::TokenPinStatusCode::STATUS_UNPINNING_PENDING:
      return "unpinning_pendig";
  }
  NOTREACHED();
  return "";
}

absl::optional<mojom::TokenPinStatusCode> StringToStatus(
    const std::string& status) {
  if (status == "not_pinned") {
    return mojom::TokenPinStatusCode::STATUS_NOT_PINNED;
  } else if (status == "pining_failed") {
    return mojom::TokenPinStatusCode::STATUS_PINNING_FAILED;
  } else if (status == "pinned") {
    return mojom::TokenPinStatusCode::STATUS_PINNED;
  } else if (status == "pinning_in_progress") {
    return mojom::TokenPinStatusCode::STATUS_PINNING_IN_PROGRESS;
  } else if (status == "unpining_in_progress") {
    return mojom::TokenPinStatusCode::STATUS_UNPINNING_IN_PROGRESS;
  } else if (status == "unpining_failed") {
    return mojom::TokenPinStatusCode::STATUS_UNPINNING_FAILED;
  } else if (status == "pinning_pendig") {
    return mojom::TokenPinStatusCode::STATUS_PINNING_PENDING;
  } else if (status == "unpinning_pendig") {
    return mojom::TokenPinStatusCode::STATUS_UNPINNING_PENDING;
  }
  return absl::nullopt;
}

absl::optional<std::string> ExtractCID(const std::string& ipfs_url) {
  GURL gurl = GURL(ipfs_url);

  if (!gurl.SchemeIs(ipfs::kIPFSScheme)) {
    return absl::nullopt;
  }

  std::vector<std::string> result = base::SplitString(
      gurl.path(), "/", base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  if (result.size() == 0) {
    return absl::nullopt;
  }

  if (!ipfs::IsValidCID(result.at(0))) {
    return absl::nullopt;
  }

  return result.at(0);
}

}  // namespace

BraveWalletPinService::BraveWalletPinService(
    PrefService* prefs,
    JsonRpcService* service,
    ipfs::IPFSRemotePinService* remote_pin_service,
    ipfs::IpfsLocalPinService* local_pin_service)
    : prefs_(prefs),
      json_rpc_service_(service),
      remote_pin_service_(remote_pin_service),
      local_pin_service_(local_pin_service) {}

BraveWalletPinService::~BraveWalletPinService() {}

mojo::PendingRemote<mojom::WalletPinService>
BraveWalletPinService::MakeRemote() {
  mojo::PendingRemote<WalletPinService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BraveWalletPinService::Bind(
    mojo::PendingReceiver<mojom::WalletPinService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BraveWalletPinService::AddObserver(
    ::mojo::PendingRemote<mojom::BraveWalletPinServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

void BraveWalletPinService::Validate(BlockchainTokenPtr token,
                                     const absl::optional<std::string>& service,
                                     ValidateCallback callback) {
  ValidateTokenInternal(std::move(token), service, std::move(callback));
}

void BraveWalletPinService::MarkAsPendingForPinning(const mojom::BlockchainTokenPtr& token,
                             const absl::optional<std::string>& service) {
  SetTokenStatus(service, token, mojom::TokenPinStatusCode::STATUS_PINNING_PENDING);
}

void BraveWalletPinService::MarkAsPendingForUnpinning(const mojom::BlockchainTokenPtr& token,
                               const absl::optional<std::string>& service) {
  SetTokenStatus(service, token, mojom::TokenPinStatusCode::STATUS_UNPINNING_PENDING);
}

void BraveWalletPinService::AddRemotePinService(
    mojom::RemotePinServicePtr service,
    AddRemotePinServiceCallback callback) {
  remote_pin_service_->AddRemotePinService(
      service->name, service->endpoint, service->key,
      base::BindOnce(&BraveWalletPinService::OnAddRemotePinServiceResult,
                     base::Unretained(this), std::move(callback)));
}

void BraveWalletPinService::RemoveRemotePinService(
    const std::string& name,
    RemoveRemotePinServiceCallback callback) {
  remote_pin_service_->RemoveRemotePinService(
      name,
      base::BindOnce(&BraveWalletPinService::OnRemoveRemotePinServiceResult,
                     base::Unretained(this), std::move(callback)));
}

void BraveWalletPinService::GetRemotePinServices(
    GetRemotePinServicesCallback callback) {
  remote_pin_service_->GetRemotePinServices(
      base::BindOnce(&BraveWalletPinService::OnGetRemotePinServicesResult,
                     base::Unretained(this), std::move(callback)));
}

void BraveWalletPinService::OnAddRemotePinServiceResult(
    AddRemotePinServiceCallback callback,
    bool result) {
  std::move(callback).Run(result, nullptr);
}

void BraveWalletPinService::OnRemoveRemotePinServiceResult(
    RemoveRemotePinServiceCallback callback,
    bool result) {
  std::move(callback).Run(result, nullptr);
}

void BraveWalletPinService::OnGetRemotePinServicesResult(
    GetRemotePinServicesCallback callback,
    absl::optional<ipfs::GetRemotePinServicesResult> result) {
 // std::move(callback).Run(result, absl::nullopt);
}

void BraveWalletPinService::AddPin(BlockchainTokenPtr token,
                                   const absl::optional<std::string>& service,
                                   AddPinCallback callback) {
  LOG(ERROR) << "XXXZZZ Add pin " << token->contract_address << " " << service.value_or("local");
  if (!service) {
    this->AddPinInternal(std::move(token), absl::nullopt, std::move(callback));
  } else if (service.value() == brave_wallet::mojom::kNftStorageServiceName) {
    EnsureNFTStorageServiceAdded(base::BindOnce(
        [](BraveWalletPinService* this_, BlockchainTokenPtr token,
           const absl::optional<std::string>& service, AddPinCallback callback,
           bool result) {
          LOG(ERROR) << "XXXZZZ on before AddPinInternal";

          if (result) {
            this_->AddPinInternal(std::move(token), service,
                                  std::move(callback));
          } else {
            this_->FinishAddingWithResult(service, std::move(token),
                                          false, absl::nullopt, std::move(callback));
          }
        },
        this, std::move(token), service, std::move(callback)));
  }
}

void BraveWalletPinService::RemovePin(
    BlockchainTokenPtr token,
    const absl::optional<std::string>& service,
    RemovePinCallback callback) {
  if (!service) {
    this->RemovePinInternal(std::move(token), absl::nullopt,
                            std::move(callback));
  } else if (service.value() == brave_wallet::mojom::kNftStorageServiceName) {
    EnsureNFTStorageServiceAdded(base::BindOnce(
        [](BraveWalletPinService* this_, BlockchainTokenPtr token,
           const absl::optional<std::string>& service, AddPinCallback callback,
           bool result) {
          LOG(ERROR) << "XXXZZZ on before AddPinInternal";

          if (result) {
            this_->RemovePinInternal(std::move(token), service,
                                     std::move(callback));
          } else {
            this_->FinishAddingWithResult(service, std::move(token),
                                          false, absl::nullopt, std::move(callback));
          }
        },
        this, std::move(token), service, std::move(callback)));
  }
}

void BraveWalletPinService::AddPinInternal(
    BlockchainTokenPtr token,
    const absl::optional<std::string>& service,
    AddPinCallback callback) {
  LOG(ERROR) << "XXXZZZ AddPinInternal " << token->contract_address;
  if (!token->is_erc721) {
    VLOG(1) << "Token is not erc721";
    FinishAddingWithResult(service, token, false, absl::nullopt, std::move(callback));
    return;
  }

  if (GetTokenStatus(service, token) ==
      mojom::TokenPinStatusCode::STATUS_PINNED) {
    FinishAddingWithResult(service, token, false, absl::nullopt, std::move(callback));
    return;
  }

  json_rpc_service_->GetERC721Metadata(
      token->contract_address, token->token_id, token->chain_id,
      base::BindOnce(&BraveWalletPinService::OnTokenMetaDataReceived,
                     base::Unretained(this), service, std::move(callback),
                     token.Clone()));
}

void BraveWalletPinService::RemovePinInternal(
    BlockchainTokenPtr token,
    const absl::optional<std::string>& service,
    RemovePinCallback callback) {
  if (GetTokenStatus(service, token) ==
      mojom::TokenPinStatusCode::STATUS_NOT_PINNED) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  absl::optional<std::vector<std::string>> items =
      ResolvePinItems(service, token);
  if (!items) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  SetTokenStatus(service, token,
                 mojom::TokenPinStatusCode::STATUS_UNPINNING_IN_PROGRESS);

  if (service) {
    remote_pin_service_->RemovePins(
        service.value(), GetPath(service, token), items.value(),
        base::BindOnce(&BraveWalletPinService::OnPinsRemoved,
                       base::Unretained(this), service, std::move(callback),
                       std::move(token)));
  } else {
    local_pin_service_->RemovePins(
        GetPath(service, token), items.value(),
        base::BindOnce(&BraveWalletPinService::OnPinsRemoved,
                       base::Unretained(this), service, std::move(callback),
                       std::move(token)));
  }
}

void BraveWalletPinService::GetTokenStatus(BlockchainTokenPtr token,
                                           GetTokenStatusCallback callback) {
  LOG(ERROR) << "XXXZZZ GetPinStatuses";
  remote_pin_service_->GetRemotePinServices(base::BindOnce(
      [](BraveWalletPinService* this_, BlockchainTokenPtr token,
         GetTokenStatusCallback callback,
         absl::optional<ipfs::GetRemotePinServicesResult> remote_services) {
        mojom::TokenPinOverviewPtr result = mojom::TokenPinOverview::New();
        result->local = this_->GetTokenStatus(absl::nullopt, token);
        if (remote_services) {
          for (const auto& remote_service : remote_services->remote_services) {
            LOG(ERROR) << "XXXZZZ GetPinStatuses for remote "
                       << remote_service.service << " "
                       << static_cast<int>(this_->GetTokenStatus(
                              remote_service.service, token));
            result->remotes[remote_service.service] =
                this_->GetTokenStatus(remote_service.service, token);
          }
        }
        std::move(callback).Run(std::move(result), nullptr);
      },
      this, std::move(token), std::move(callback)));
}

void BraveWalletPinService::ValidateTokenInternal(
    BlockchainTokenPtr token,
    const absl::optional<std::string>& service,
    ValidateCallback callback) {
  mojom::TokenPinStatusCode status = GetTokenStatus(service, token);
  if (status != mojom::TokenPinStatusCode::STATUS_PINNED) {
    return;
  }
  absl::optional<std::vector<std::string>> cids =
      ResolvePinItems(service, token);

  if (!cids) {
    return;
  }

  if (!service) {
    local_pin_service_->ValidatePins(
        GetPath(absl::nullopt, token), cids.value(),
        base::BindOnce(&BraveWalletPinService::OnTokenValidated,
                       base::Unretained(this), service, std::move(callback),
                       std::move(token)));
  } else {
  }
}

void BraveWalletPinService::OnPinsRemoved(absl::optional<std::string> service,
                                          RemovePinCallback callback,
                                          mojom::BlockchainTokenPtr token,
                                          bool result) {
  SetTokenStatus(service, token,
                 result ? mojom::TokenPinStatusCode::STATUS_NOT_PINNED
                        : mojom::TokenPinStatusCode::STATUS_UNPINNING_FAILED);
  std::move(callback).Run(result, nullptr);
}

void BraveWalletPinService::OnTokenMetaDataReceived(
    absl::optional<std::string> service,
    AddPinCallback callback,
    mojom::BlockchainTokenPtr token,
    const std::string& token_url,
    const std::string& result,
    mojom::ProviderError error,
    const std::string& error_message) {
  // TODO: move on bg thread
  if (error != mojom::ProviderError::kSuccess) {
    FinishAddingWithResult(service, token, false, absl::nullopt,
                           std::move(callback));
    return;
  }

  GURL token_gurl = GURL(token_url);
  if (!token_gurl.SchemeIs(ipfs::kIPFSScheme)) {
    FinishAddingWithResult(service, token, false, absl::nullopt,
                           std::move(callback));
    return;
  }

  absl::optional<base::Value> parsed_result = base::JSONReader::Read(
      result, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
  if (!parsed_result || !parsed_result->is_dict()) {
    FinishAddingWithResult(service, token, false, absl::nullopt,
                           std::move(callback));
    return;
  }

  //  const base::Value::Dict* properties =
  //  parsed_result->FindDictKey("properties"); if (!properties) {
  //    FinishAddingWithResult(service, token, false, absl::nullopt,
  //                           std::move(callback));
  //    return;
  //  }

  LOG(ERROR) << "XXXZZZ token metadata received 5";

  std::vector<std::string> cids;

  cids.push_back(ExtractCID(token_url).value());
  auto* image = parsed_result->FindStringKey("image");
  if (image) {
    cids.push_back(ExtractCID(*image).value());
  }
  //  for (const auto property : *properties) {
  //    const std::string* description =
  //        property.second.FindStringKey("description");
  //    if (!description) {
  //      continue;
  //    }

  //    const GURL url(*description);

  //    if (url.is_valid() &&
  //    url.SchemeIs(ipfs::kIPFSSchFinishAddingWithResulteme)) {
  //      cids.push_back(url.spec());
  //    }
  //  }

  LOG(ERROR) << "XXXZZZ token metadata received 6";

  CreateToken(service, token, cids);
  SetTokenStatus(service, token,
                 mojom::TokenPinStatusCode::STATUS_PINNING_IN_PROGRESS);

  LOG(ERROR) << "XXXZZZ token metadata received 7";

  if (service) {
    remote_pin_service_->AddPins(
        service.value(), GetGroupName(service, token), cids,
        base::BindOnce(&BraveWalletPinService::OnTokenPinned,
                       base::Unretained(this), service, std::move(callback),
                       std::move(token)));
  } else {
    local_pin_service_->AddPins(
        GetGroupName(service, token), cids,
        base::BindOnce(&BraveWalletPinService::OnTokenPinned,
                       base::Unretained(this), absl::nullopt,
                       std::move(callback), std::move(token)));
  }
}

void BraveWalletPinService::OnTokenPinned(absl::optional<std::string> service,
                                          AddPinCallback callback,
                                          mojom::BlockchainTokenPtr token,
                                          bool result) {
  SetTokenStatus(service, token,
                 result ? mojom::TokenPinStatusCode::STATUS_PINNED
                        : mojom::TokenPinStatusCode::STATUS_PINNING_FAILED);

  FinishAddingWithResult(service, token, result, absl::nullopt,
                         std::move(callback));
}

void BraveWalletPinService::OnTokenValidated(
    absl::optional<std::string> service,
    ValidateCallback callback,
    mojom::BlockchainTokenPtr token,
    bool result) {
  if (!result) {
    SetTokenStatus(service, token,
                   mojom::TokenPinStatusCode::STATUS_PINNING_IN_PROGRESS);
    std::move(callback).Run(false, nullptr);
    return;
  }
  // Also updates verification timestamp
  SetTokenStatus(service, token, mojom::TokenPinStatusCode::STATUS_PINNED);
  std::move(callback).Run(true, nullptr);
}

void BraveWalletPinService::CreateToken(absl::optional<std::string> service,
                                        const mojom::BlockchainTokenPtr& token,
                                        const std::vector<std::string>& cids) {
  LOG(ERROR) << "XXXZZZ create token " << token->name << " "
             << base::JoinString(cids, ",");
  DictionaryPrefUpdate update(prefs_, kPinnedErc721Assets);
  base::Value::Dict& update_dict = update->GetDict();

  base::Value::Dict token_data;
  base::Value::List cids_list;

  for (const auto& cid : cids) {
    cids_list.Append(cid);
  }

  token_data.Set(kAssetUrlListKey, std::move(cids_list));
  token_data.Set(kAssetStatus,
                 StatusToString(mojom::TokenPinStatusCode::STATUS_NOT_PINNED));

  update_dict.SetByDottedPath(GetPath(service, token), std::move(token_data));
}

void BraveWalletPinService::SetTokenStatus(
    absl::optional<std::string> service,
    const mojom::BlockchainTokenPtr& token,
    mojom::TokenPinStatusCode status) {
  LOG(ERROR) << "XXXZZZ set token status " << StatusToString(status);
  {
    DictionaryPrefUpdate update(prefs_, kPinnedErc721Assets);
    base::Value::Dict& update_dict = update->GetDict();

    update_dict.SetByDottedPath(GetPath(service, token) + "." + kAssetStatus,
                                StatusToString(status));

    if (status == mojom::TokenPinStatusCode::STATUS_PINNED) {
      update_dict.SetByDottedPath(
          GetPath(service, token) + "." + kValidateTimestamp,
          base::NumberToString(base::Time::Now().ToInternalValue()));
    }
  }
  for (const auto& observer : observers_) {
    observer->OnTokenStatusChanged(service, token.Clone(), status);
  }
}

absl::optional<std::vector<std::string>> BraveWalletPinService::ResolvePinItems(
    const absl::optional<std::string>& service,
    const BlockchainTokenPtr& token) {
  const base::Value::Dict& pinned_assets_pref =
      prefs_->GetDict(kPinnedErc721Assets);

  const std::string& path = GetPath(service, token);

  auto* token_data_as_dict = pinned_assets_pref.FindDictByDottedPath(path);
  if (!token_data_as_dict) {
    return absl::nullopt;
  }

  auto* cids = token_data_as_dict->FindList(kAssetUrlListKey);
  if (!cids) {
    return absl::nullopt;
  }

  std::vector<std::string> result;
  for (const base::Value& item : *cids) {
    result.push_back(item.GetString());
  }

  return result;
}

mojom::TokenPinStatusCode BraveWalletPinService::GetTokenStatus(
    absl::optional<std::string> service,
    const mojom::BlockchainTokenPtr& token) {
  const base::Value::Dict& pinned_assets_pref =
      prefs_->GetDict(kPinnedErc721Assets);

  const std::string& path = GetPath(service, token);

  auto* token_data_as_dict = pinned_assets_pref.FindDictByDottedPath(path);
  if (!token_data_as_dict) {
    return mojom::TokenPinStatusCode::STATUS_NOT_PINNED;
  }

  auto* status = token_data_as_dict->FindString(kAssetStatus);
  if (!status) {
    return mojom::TokenPinStatusCode::STATUS_NOT_PINNED;
  }

  return StringToStatus(*status).value_or(
      mojom::TokenPinStatusCode::STATUS_NOT_PINNED);
}

absl::optional<base::Time> BraveWalletPinService::GetLastValidateTime(
    absl::optional<std::string> service,
    const mojom::BlockchainTokenPtr& token) {
  const base::Value::Dict& pinned_assets_pref =
      prefs_->GetDict(kPinnedErc721Assets);

  const std::string& path = GetPath(service, token);

  auto* token_data_as_dict = pinned_assets_pref.FindDictByDottedPath(path);
  if (!token_data_as_dict) {
    return absl::nullopt;
  }

  auto* time = token_data_as_dict->FindString(kValidateTimestamp);
  int64_t value;
  if (time && base::StringToInt64(*time, &value)) {
    return base::Time::FromInternalValue(value);
  }
  return absl::nullopt;
}

void BraveWalletPinService::FinishAddingWithResult(
    absl::optional<std::string> service,
    const mojom::BlockchainTokenPtr& token,
    bool result,
    absl::optional<mojom::WalletPinServiceErrorCode> response,
    AddPinCallback callback) {
  std::move(callback).Run(result, nullptr);
}

void BraveWalletPinService::EnsureNFTStorageServiceAdded(
    EnsureNFTStorageServiceAddedCallback callback) {
  LOG(ERROR) << "XXXZZZ EnsureNFTStorageServiceAdded";
  remote_pin_service_->GetRemotePinServices(
      base::BindOnce(&BraveWalletPinService::OnAvailableServicesReady,
                     base::Unretained(this), std::move(callback)));
}

void BraveWalletPinService::OnAvailableServicesReady(
    EnsureNFTStorageServiceAddedCallback callback,
    absl::optional<ipfs::GetRemotePinServicesResult> result) {
  LOG(ERROR) << "XXXZZZ OnAvailableServicesReady " << result.has_value();

  if (!result) {
    std::move(callback).Run(false);
    return;
  }
  for (const auto& item : result->remote_services) {
    if (mojom::kNftStorageServiceName == item.service) {
      std::move(callback).Run(true);
      return;
    }
  }
  LOG(ERROR) << "XXXZZZ remote_pin_service_->AddRemotePinService";

  remote_pin_service_->AddRemotePinService(
      mojom::kNftStorageServiceName, "https://api.nft.storage/",
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJzdWIiOiJkaWQ6ZXRocjoweDE4RDRlNERFMDcyMEU1MmI3OWQ1NDkwODlFMTMyNmNCNUFk"
      "ZmRCYzciLCJpc3MiOiJuZnQtc3RvcmFnZSIsImlhdCI6MTY2NTY2Njc2NDYwNSwibmFtZSI6"
      "ImEifQ.P2VfLUjTdAglXxrQjj0WSkYo_S-7eyaVVf9-RDzt7qg",
      base::BindOnce([](EnsureNFTStorageServiceAddedCallback callback,
                        bool result) { std::move(callback).Run(result); },
                     std::move(callback)));
}

}  // namespace brave_wallet
