/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_allowance_manager.h"

#include <algorithm>
#include <utility>

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace {
constexpr int kTopicElementsCountToCheck = 3;
constexpr char kLastBlockNumber[] = "last_block_number";
constexpr char kAllowanceFound[] = "allowances_found";
constexpr char kApproverAddress[] = "approver_address";
constexpr char kContractAddress[] = "contract_address";
constexpr char kSpenderAddress[] = "spender_address";
constexpr char kAmount[] = "amount";

constexpr char kAddress[] = "address";
constexpr char kTopics[] = "topics";
constexpr char kFromBlock[] = "fromBlock";
constexpr char kToBlock[] = "toBlock";
constexpr char kEarliestBlock[] = "earliest";

constexpr char kApprovalTopicFunctionSignature[] =
    "Approval(address,address,uint256)";

// Returns vector of chain_ids supported by allowance discovering.
const std::vector<std::string>& GetChainIdsForAlowanceDiscovering() {
  static base::NoDestructor<std::vector<std::string>> supported_chain_ids(
      {brave_wallet::mojom::kMainnetChainId,
       brave_wallet::mojom::kPolygonMainnetChainId,
       brave_wallet::mojom::kAvalancheMainnetChainId,
       brave_wallet::mojom::kCeloMainnetChainId,
       brave_wallet::mojom::kArbitrumMainnetChainId,
       brave_wallet::mojom::kOptimismMainnetChainId});
  return *supported_chain_ids;
}
std::string GetAllowanceMapKey(const std::string& contract_address,
                               const std::string& approver_addr,
                               const std::string& spender_address) {
  return base::JoinString({contract_address, approver_addr, spender_address},
                          "_");
}

std::string GetBlocknumberFilterFromCache(
    PrefService* prefs,
    const std::string& chain_id,
    const std::string& hex_account_address) {
  std::string result(kEarliestBlock);
  const auto& allowance_cache_dict =
      prefs->GetDict(kBraveWalletEthAllowancesCache);

  const auto* last_block_number_ptr =
      allowance_cache_dict.FindStringByDottedPath(base::JoinString(
          {chain_id, kLastBlockNumber, hex_account_address}, "."));

  brave_wallet::uint256_t block_number{0};
  if (last_block_number_ptr && !last_block_number_ptr->empty() &&
      brave_wallet::HexValueToUint256(*last_block_number_ptr, &block_number)) {
    result = brave_wallet::Uint256ValueToHex(++block_number);
  }
  return result;
}

}  // namespace

namespace brave_wallet {
EthAllowanceManager::EthAllowanceTask::EthAllowanceTask(
    const int& taskid,
    const std::string& chain_id,
    const std::string& account_address,
    const uint256_t& latest_block_number)
    : task_id(taskid),
      account_address_(account_address),
      latest_block_number_(latest_block_number),
      chain_id_(chain_id) {}
EthAllowanceManager::EthAllowanceTask::~EthAllowanceTask() = default;
void EthAllowanceManager::EthAllowanceTask::SetResults(
    std::vector<mojom::AllowanceInfoPtr> alwns) {
  allowances_ = std::move(alwns);
  MarkComplete();
}
void EthAllowanceManager::EthAllowanceTask::MarkComplete() {
  is_completed_ = true;
}

EthAllowanceManager::EthAllowanceManager(JsonRpcService* json_rpc_service,
                                         KeyringService* keyring_service,
                                         PrefService* prefs)
    : json_rpc_service_(json_rpc_service),
      keyring_service_(keyring_service),
      prefs_(prefs) {}

EthAllowanceManager::~EthAllowanceManager() = default;

// Start of the allowance discovering operation.
void EthAllowanceManager::DiscoverEthAllowancesOnAllSupportedChains(
    ResultCallback callback) {
  if (!discover_eth_allowance_callbacks_.empty()) {
    discover_eth_allowance_callbacks_.push_back(std::move(callback));
    return;
  }
  discover_eth_allowance_callbacks_.push_back(std::move(callback));

  std::vector<std::string> account_addresses;
  for (const auto& account_info : keyring_service_->GetAllAccountInfos()) {
    if (account_info->account_id->coin == mojom::CoinType::ETH) {
      account_addresses.push_back(account_info->address);
    }
  }

  if (account_addresses.empty()) {
    OnDiscoverEthAllowancesCompleted({});
    return;
  }

  allowance_discovery_tasks_.clear();
  get_block_tasks_ = 0;

  const auto token_list_map =
      BlockchainRegistry::GetInstance()->GetEthTokenListMap(
          GetChainIdsForAlowanceDiscovering());

  for (const auto& [chain_id, token_list] : token_list_map) {
    base::Value::List contract_addresses;
    for (const auto& token : token_list) {
      contract_addresses.Append(token->contract_address);
    }

    if (contract_addresses.empty()) {
      continue;
    }
    auto internal_callback = base::BindOnce(
        &EthAllowanceManager::OnGetCurrentBlock, weak_ptr_factory_.GetWeakPtr(),
        chain_id, std::move(contract_addresses), account_addresses);
    get_block_tasks_++;
    json_rpc_service_->GetBlockNumber(chain_id, std::move(internal_callback));
  }
}

void EthAllowanceManager::OnGetCurrentBlock(
    const std::string& chain_id,
    base::Value::List contract_addresses,
    const std::vector<std::string>& account_addresses,
    uint256_t block_num,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    get_block_tasks_--;
    MaybeMergeAllResultsAndCallBack();
    return;
  }

  const auto approval_topic_hash = ToHex(KeccakHash(
      base::byte_span_from_cstring(kApprovalTopicFunctionSignature)));
  for (const auto& account_address : account_addresses) {
    std::string account_address_hex;
    if (!PadHexEncodedParameter(account_address, &account_address_hex)) {
      continue;
    }
    const int task_id(allowance_discovery_tasks_.size());
    auto internal_callback =
        base::BindOnce(&EthAllowanceManager::OnGetAllowances,
                       weak_ptr_factory_.GetWeakPtr(), task_id);
    allowance_discovery_tasks_.insert_or_assign(
        task_id, std::make_unique<EthAllowanceTask>(
                     task_id, chain_id, account_address_hex, block_num));

    base::Value::List topics;
    topics.Append(approval_topic_hash);
    topics.Append(account_address_hex);

    base::Value::Dict filter_options;
    filter_options.Set(kAddress, contract_addresses.Clone());
    filter_options.Set(kTopics, std::move(topics));
    filter_options.Set(kFromBlock, GetBlocknumberFilterFromCache(
                                       prefs_, chain_id, account_address_hex));
    filter_options.Set(kToBlock, Uint256ValueToHex(block_num));
    json_rpc_service_->EthGetLogs(chain_id, std::move(filter_options),
                                  std::move(internal_callback));
  }
  get_block_tasks_--;
}

void EthAllowanceManager::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  allowance_discovery_tasks_.clear();
  OnDiscoverEthAllowancesCompleted({});
}

// Loads cached data from the preferences.
//
// Cached data, always must be loaded first as we loading retrieved from the
// logs "fresh" data.
void EthAllowanceManager::LoadCachedAllowances(
    const std::string& chain_id,
    const std::string& hex_account_address,
    std::map<std::string, mojom::AllowanceInfoPtr>& allowance_map) {
  const auto& allowance_cache_dict =
      prefs_->GetDict(kBraveWalletEthAllowancesCache);
  const auto* chain_cached_data = allowance_cache_dict.FindDict(chain_id);
  if (!chain_cached_data) {
    return;
  }

  const auto* cached_allowances_ptr =
      chain_cached_data->FindList(kAllowanceFound);
  if (!cached_allowances_ptr) {
    return;
  }

  const auto* block_number_section_ptr =
      chain_cached_data->FindDict(kLastBlockNumber);
  if (!block_number_section_ptr) {
    return;
  }

  for (const auto& ca_item : *cached_allowances_ptr) {
    const auto* ca_dict = ca_item.GetIfDict();
    if (!ca_dict) {
      continue;
    }
    const auto* approver_address = ca_dict->FindString(kApproverAddress);
    const auto* contract_address = ca_dict->FindString(kContractAddress);
    const auto* spender_address = ca_dict->FindString(kSpenderAddress);
    const auto* amount = ca_dict->FindString(kAmount);

    if (!approver_address || !contract_address || !spender_address || !amount) {
      NOTREACHED_IN_MIGRATION() << " Wrong allowance cache format";
      continue;
    }

    if (!base::EqualsCaseInsensitiveASCII(*approver_address,
                                          hex_account_address)) {
      continue;
    }

    allowance_map.insert_or_assign(
        GetAllowanceMapKey(*contract_address, *approver_address,
                           *spender_address),
        mojom::AllowanceInfo::New(chain_id, *contract_address,
                                  *approver_address, *spender_address,
                                  *amount));
  }
}

// Processes each data portion retrieved from logs
void EthAllowanceManager::OnGetAllowances(const int& task_id,
                                          const std::vector<Log>& logs,
                                          base::Value rawlogs,
                                          mojom::ProviderError error,
                                          const std::string& error_message) {
  auto task_iter = allowance_discovery_tasks_.find(task_id);
  if (task_iter == allowance_discovery_tasks_.end()) {
    MaybeMergeAllResultsAndCallBack();
    return;
  }

  if (error != mojom::ProviderError::kSuccess) {
    task_iter->second->MarkComplete();
    MaybeMergeAllResultsAndCallBack();
    return;
  }

  std::vector<Log> logs_tmp{logs};
  std::sort(logs_tmp.begin(), logs_tmp.end(),
            [](const Log& first, const Log& second) -> bool {
              return std::tie(first.block_number, first.log_index) <
                     std::tie(second.block_number, second.log_index);
            });

  // Collection of the latest allowances per contract & spender & approver.
  std::map<std::string, mojom::AllowanceInfoPtr> allowance_map;
  // Put cached data into the map first if available.
  LoadCachedAllowances(task_iter->second->chain_id_,
                       task_iter->second->account_address_, allowance_map);
  for (const auto& log_item : logs_tmp) {
    // Skip pending logs.
    if (log_item.block_number == 0) {
      continue;
    }

    if (log_item.topics.size() < kTopicElementsCountToCheck) {
      continue;
    }
    const auto current_map_key = GetAllowanceMapKey(
        log_item.address, log_item.topics[1], log_item.topics[2]);

    uint256_t parsed_amount{0};
    if (!HexValueToUint256(log_item.data, &parsed_amount)) {
      continue;
    }

    if (parsed_amount > 0) {
      // Replace if same key exists by the fresh allowance data.
      allowance_map.insert_or_assign(
          current_map_key,
          mojom::AllowanceInfo::New(task_iter->second->chain_id_,
                                    log_item.address, log_item.topics[1],
                                    log_item.topics[2], log_item.data));
    } else if (allowance_map.find(current_map_key) != allowance_map.end()) {
      allowance_map.erase(current_map_key);
    }
  }
  std::vector<mojom::AllowanceInfoPtr> allowances_found;
  for (const auto& [allowance_map_key, allowance] : allowance_map) {
    allowances_found.push_back(allowance.Clone());
  }
  task_iter->second->SetResults(std::move(allowances_found));
  MaybeMergeAllResultsAndCallBack();
}

bool EthAllowanceManager::IsAllTasksCompleted() const {
  DCHECK(!discover_eth_allowance_callbacks_.empty());

  return get_block_tasks_ == 0 &&
         base::ranges::all_of(allowance_discovery_tasks_, [](const auto& item) {
           return item.second->is_completed_;
         });
}

void EthAllowanceManager::MaybeMergeAllResultsAndCallBack() {
  if (!IsAllTasksCompleted()) {
    return;
  }
  std::vector<mojom::AllowanceInfoPtr> result;
  ScopedDictPrefUpdate allowance_cache_update(prefs_,
                                              kBraveWalletEthAllowancesCache);
  auto& allowance_cache = allowance_cache_update.Get();

  std::map<std::string, std::vector<std::unique_ptr<EthAllowanceTask>>>
      allowance_tasks_by_chain_ids;
  for (auto& [tid, allowance_task_info] : allowance_discovery_tasks_) {
    allowance_tasks_by_chain_ids[allowance_task_info->chain_id_].push_back(
        std::move(allowance_task_info));
  }

  for (const auto& [chain_id, allowance_task_infos] :
       allowance_tasks_by_chain_ids) {
    auto* chain_section = allowance_cache.EnsureDict(chain_id);
    auto* last_block_section = chain_section->EnsureDict(kLastBlockNumber);
    base::Value::List allw_list;
    for (const auto& allowance_task_info : allowance_task_infos) {
      last_block_section->Set(
          allowance_task_info->account_address_,
          Uint256ValueToHex(allowance_task_info->latest_block_number_));
      for (const auto& allowance : allowance_task_info->allowances_) {
        if (allowance.is_null()) {
          continue;
        }
        base::Value::Dict allw_dict_item;
        allw_dict_item.Set(kContractAddress, allowance->contract_address);
        allw_dict_item.Set(kApproverAddress, allowance->approver_address);
        allw_dict_item.Set(kSpenderAddress, allowance->spender_address);
        allw_dict_item.Set(kAmount, allowance->amount);
        allw_list.Append(std::move(allw_dict_item));
        result.push_back(allowance.Clone());
      }
    }
    chain_section->Set(kAllowanceFound, std::move(allw_list));
  }
  OnDiscoverEthAllowancesCompleted(result);
}

void EthAllowanceManager::OnDiscoverEthAllowancesCompleted(
    const std::vector<mojom::AllowanceInfoPtr>& result) {
  for (auto& callback : discover_eth_allowance_callbacks_) {
    std::vector<mojom::AllowanceInfoPtr> results;
    for (const auto& allowance_info : result) {
      results.push_back(allowance_info.Clone());
    }
    std::move(callback).Run(std::move(results));
  }
  discover_eth_allowance_callbacks_.clear();
}

}  // namespace brave_wallet
