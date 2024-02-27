/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNS_RESOLVER_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNS_RESOLVER_TASK_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/notreached.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/solana_address.h"

namespace brave_wallet {

using SnsNamehash = std::array<uint8_t, 32>;
inline constexpr char kSnsSolRecord[] = "SOL";
inline constexpr char kSnsUrlRecord[] = "url";
inline constexpr char kSnsIpfsRecord[] = "IPFS";

enum class SnsRecordsVersion { kRecordsV1, kRecordsV2 };
enum class SnsRecordV2ValidationType : uint16_t {
  kNone = 0,
  kSolana = 1,
  kEthereum = 2,
  kSolanaUnverified = 3
};

SnsNamehash GetHashedName(const std::string& prefix, const std::string& name);

std::optional<SolanaAddress> GetMintAddress(
    const SolanaAddress& domain_address);
std::optional<SolanaAddress> GetDomainKey(const std::string& domain);
std::optional<SolanaAddress> GetRecordKey(const std::string& domain,
                                          const std::string& record,
                                          SnsRecordsVersion version);

struct NameRegistryState {
  NameRegistryState();
  NameRegistryState(const NameRegistryState&);
  NameRegistryState(NameRegistryState&&);
  NameRegistryState& operator=(const NameRegistryState&);
  NameRegistryState& operator=(NameRegistryState&&);
  ~NameRegistryState();

  static std::optional<NameRegistryState> FromBytes(
      base::span<const uint8_t> data_span);
  static std::optional<NameRegistryState> FromBase64(const std::string& str);

  SolanaAddress parent_name;
  SolanaAddress owner;
  SolanaAddress data_class;
  std::vector<uint8_t> data;
};

struct SnsResolverTaskResult {
  SnsResolverTaskResult() = default;
  explicit SnsResolverTaskResult(SolanaAddress address);
  SnsResolverTaskResult(const SnsResolverTaskResult&) = default;
  SnsResolverTaskResult(SnsResolverTaskResult&&) = default;
  SnsResolverTaskResult& operator=(const SnsResolverTaskResult&) = default;
  SnsResolverTaskResult& operator=(SnsResolverTaskResult&&) = default;
  ~SnsResolverTaskResult() = default;

  SolanaAddress resolved_address;
  GURL resolved_url;
};

struct SnsResolverTaskError {
  SnsResolverTaskError() = default;
  SnsResolverTaskError(mojom::SolanaProviderError error,
                       std::string error_message);
  SnsResolverTaskError(const SnsResolverTaskError&) = default;
  SnsResolverTaskError(SnsResolverTaskError&&) = default;
  SnsResolverTaskError& operator=(const SnsResolverTaskError&) = default;
  SnsResolverTaskError& operator=(SnsResolverTaskError&&) = default;
  ~SnsResolverTaskError() = default;

  mojom::SolanaProviderError error;
  std::string error_message;
};

class SnsResolverTask {
 public:
  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;
  using DoneCallback =
      base::OnceCallback<void(SnsResolverTask* task,
                              std::optional<SnsResolverTaskResult> result,
                              std::optional<SnsResolverTaskError> error)>;
  using RequestIntermediateCallback =
      base::OnceCallback<void(APIRequestResult api_request_result)>;
  using ResponseConversionCallback =
      base::OnceCallback<std::optional<std::string>(
          const std::string& raw_response)>;

  enum class TaskType { kResolveWalletAddress, kResolveUrl };

  SnsResolverTask(DoneCallback done_callback,
                  APIRequestHelper* api_request_helper,
                  const std::string& domain,
                  const GURL& network_url,
                  TaskType type);
  SnsResolverTask(const SnsResolverTask&) = delete;
  SnsResolverTask& operator=(const SnsResolverTask&) = delete;
  ~SnsResolverTask();

  const std::string& domain() const { return domain_; }

  static base::RepeatingCallback<void(SnsResolverTask* task)>&
  GetWorkOnTaskForTesting();
  void SetResultForTesting(std::optional<SnsResolverTaskResult> task_result,
                           std::optional<SnsResolverTaskError> task_error);

  void FetchNftSplMint();
  void OnFetchNftSplMint(APIRequestResult api_request_result);
  void FetchNftTokenOwner();
  void OnFetchNftTokenOwner(APIRequestResult api_request_result);

  void FetchDomainRegistryState();
  void OnFetchDomainRegistryState(APIRequestResult api_request_result);

 private:
  template <typename T>
  friend class SnsResolverTaskContainer;
  void ScheduleWorkOnTask();
  bool FillWorkData();
  void WorkOnTask();
  void WorkOnWalletAddressTask();
  void WorkOnDomainResolveTask();

  void SetAddressResult(SolanaAddress address);
  void SetUrlResult(GURL url);
  void SetError(SnsResolverTaskError error);
  void NftOwnerDone(std::optional<SolanaAddress> nft_owner);

  void FetchNextRecord();
  void OnFetchNextRecord(APIRequestResult api_request_result);

  void RequestInternal(const std::string& json_payload,
                       RequestIntermediateCallback callback,
                       ResponseConversionCallback conversion_callback);

  DoneCallback done_callback_;
  raw_ptr<APIRequestHelper> api_request_helper_;
  std::string domain_;
  GURL network_url_;
  TaskType task_type_ = TaskType::kResolveWalletAddress;

  bool work_data_ready_ = false;
  SolanaAddress domain_address_;
  SolanaAddress nft_mint_address_;
  std::vector<struct SnsFetchRecordQueueItem> records_queue_;
  size_t cur_queue_item_pos_ = 0;

  bool nft_owner_check_done_ = false;
  std::optional<SolanaAddress> nft_owner_;
  bool nft_mint_supply_check_done_ = false;

  std::optional<NameRegistryState> domain_name_registry_state_;

  std::optional<SnsResolverTaskResult> task_result_;
  std::optional<SnsResolverTaskError> task_error_;

  base::WeakPtrFactory<SnsResolverTask> weak_ptr_factory_{this};
};

template <typename ResultCallback>
class SnsResolverTaskContainer {
 public:
  SnsResolverTaskContainer() = default;
  SnsResolverTaskContainer(const SnsResolverTaskContainer&) = delete;
  SnsResolverTaskContainer& operator=(const SnsResolverTaskContainer&) = delete;
  ~SnsResolverTaskContainer() = default;

  void AddTask(std::unique_ptr<SnsResolverTask> task, ResultCallback cb) {
    DCHECK(!ContainsTaskForDomain(task->domain()));

    auto* task_ptr = task.get();
    std::vector<ResultCallback> callbacks;
    callbacks.push_back(std::move(cb));
    tasks_.emplace(task_ptr,
                   std::make_pair(std::move(task), std::move(callbacks)));
    task_ptr->ScheduleWorkOnTask();
  }

  bool ContainsTaskForDomain(const std::string& domain) {
    for (auto& [_, task_pair] : tasks_) {
      if (task_pair.first->domain() == domain) {
        return true;
      }
    }
    return false;
  }

  void AddCallbackForDomain(const std::string& domain, ResultCallback cb) {
    for (auto& [_, task_pair] : tasks_) {
      if (task_pair.first->domain() == domain) {
        task_pair.second.push_back(std::move(cb));
        return;
      }
    }
    NOTREACHED();
  }

  std::vector<ResultCallback> TaskDone(SnsResolverTask* task) {
    auto item = tasks_.find(task);
    if (item == tasks_.end()) {
      return {};
    }

    std::vector<ResultCallback> callbacks = std::move(item->second.second);
    tasks_.erase(item);
    return callbacks;
  }

 private:
  base::flat_map<
      SnsResolverTask*,
      std::pair<std::unique_ptr<SnsResolverTask>, std::vector<ResultCallback>>>
      tasks_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNS_RESOLVER_TASK_H_
