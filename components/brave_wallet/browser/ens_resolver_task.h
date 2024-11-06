/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_RESOLVER_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_RESOLVER_TASK_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/notreached.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"

namespace brave_wallet {

// Selector for `OffchainLookup(address,string[],bytes,bytes4,bytes)`
inline constexpr uint8_t kOffchainLookupSelector[] = {0x55, 0x6f, 0x18, 0x30};

// Selector for `resolve(bytes,bytes)`
inline constexpr uint8_t kResolveBytesBytesSelector[] = {0x90, 0x61, 0xb9,
                                                         0x23};

// Selector for `addr(bytes32)`
inline constexpr uint8_t kAddrBytes32Selector[] = {0x3b, 0x3b, 0x57, 0xde};

// Selector for `contenthash(bytes32)`
inline constexpr uint8_t kContentHashBytes32Selector[] = {0xbc, 0x1c, 0x58,
                                                          0xd1};

std::vector<uint8_t> MakeAddrCall(const std::string& domain);
std::vector<uint8_t> MakeContentHashCall(const std::string& domain);

struct OffchainLookupData {
  OffchainLookupData();
  OffchainLookupData(const OffchainLookupData&);
  OffchainLookupData(OffchainLookupData&&);
  OffchainLookupData& operator=(const OffchainLookupData&);
  OffchainLookupData& operator=(OffchainLookupData&&);
  ~OffchainLookupData();

  static std::optional<OffchainLookupData> ExtractFromJson(
      const base::Value& json_value);
  static std::optional<OffchainLookupData> ExtractFromEthAbiPayload(
      eth_abi::Span bytes);

  EthAddress sender;
  std::vector<std::string> urls;
  std::vector<uint8_t> call_data;
  std::vector<uint8_t> callback_function;
  std::vector<uint8_t> extra_data;
};

struct EnsResolverTaskResult {
  EnsResolverTaskResult();
  EnsResolverTaskResult(std::vector<uint8_t> resolved_result,
                        bool need_to_allow_offchain);
  EnsResolverTaskResult(const EnsResolverTaskResult&);
  EnsResolverTaskResult(EnsResolverTaskResult&&);
  EnsResolverTaskResult& operator=(const EnsResolverTaskResult&);
  EnsResolverTaskResult& operator=(EnsResolverTaskResult&&);
  ~EnsResolverTaskResult();

  std::vector<uint8_t> resolved_result;
  bool need_to_allow_offchain = false;
};

struct EnsResolverTaskError {
  EnsResolverTaskError();
  EnsResolverTaskError(mojom::ProviderError error, std::string error_message);
  EnsResolverTaskError(const EnsResolverTaskError&);
  EnsResolverTaskError(EnsResolverTaskError&&);
  EnsResolverTaskError& operator=(const EnsResolverTaskError&);
  EnsResolverTaskError& operator=(EnsResolverTaskError&&);
  ~EnsResolverTaskError();

  mojom::ProviderError error;
  std::string error_message;
};

class EnsResolverTask {
 public:
  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;
  using DoneCallback =
      base::OnceCallback<void(EnsResolverTask* task,
                              std::optional<EnsResolverTaskResult> result,
                              std::optional<EnsResolverTaskError> error)>;
  using RequestIntermediateCallback =
      base::OnceCallback<void(APIRequestResult api_request_result)>;

  EnsResolverTask(DoneCallback done_callback,
                  APIRequestHelper* api_request_helper,
                  APIRequestHelper* api_request_helper_ens_offchain,
                  std::vector<uint8_t> ens_call,
                  const std::string& domain,
                  const GURL& network_url,
                  std::optional<bool> allow_offchain);
  EnsResolverTask(const EnsResolverTask&) = delete;
  EnsResolverTask& operator=(const EnsResolverTask&) = delete;
  ~EnsResolverTask();

  const std::string& domain() const { return domain_; }
  const std::optional<bool>& allow_offchain() const { return allow_offchain_; }

  static base::RepeatingCallback<void(EnsResolverTask* task)>&
  GetWorkOnTaskForTesting();
  void SetResultForTesting(std::optional<EnsResolverTaskResult> task_result,
                           std::optional<EnsResolverTaskError> task_error);

 private:
  template <typename T>
  friend class EnsResolverTaskContainer;
  void ScheduleWorkOnTask();
  void WorkOnTask();

  void FetchEnsResolver();
  void OnFetchEnsResolverDone(APIRequestResult api_request_result);
  void FetchEnsip10Support();
  void OnFetchEnsip10SupportDone(APIRequestResult api_request_result);

  void FetchEnsRecord();
  void OnFetchEnsRecordDone(APIRequestResult api_request_result);

  void FetchWithEnsip10Resolve();
  void OnFetchWithEnsip10ResolveDone(APIRequestResult api_request_result);

  void FetchOffchainData();
  void OnFetchOffchainDone(APIRequestResult api_request_result);

  void FetchOffchainCallback();
  void OnFetchOffchainCallbackDone(APIRequestResult api_request_result);

  void RequestInternal(const std::string& json_payload,
                       RequestIntermediateCallback callback);

  DoneCallback done_callback_;
  raw_ptr<APIRequestHelper> api_request_helper_;
  raw_ptr<APIRequestHelper> api_request_helper_ens_offchain_;
  std::vector<uint8_t> ens_call_;
  std::string domain_;
  std::string resolver_domain_;
  std::optional<std::vector<uint8_t>> dns_encoded_name_;
  GURL network_url_;
  std::optional<bool> allow_offchain_;
  int offchain_lookup_attemps_left_ = 4;

  std::optional<EnsResolverTaskResult> task_result_;
  std::optional<EnsResolverTaskError> task_error_;

  EthAddress resolver_address_;
  std::optional<bool> supports_ensip_10_;

  std::optional<std::vector<uint8_t>> offchain_callback_call_;
  std::optional<OffchainLookupData> offchain_lookup_data_;

  base::WeakPtrFactory<EnsResolverTask> weak_ptr_factory_{this};
};

template <typename ResultCallback>
class EnsResolverTaskContainer {
 public:
  EnsResolverTaskContainer() = default;
  EnsResolverTaskContainer(const EnsResolverTaskContainer&) = delete;
  EnsResolverTaskContainer(EnsResolverTaskContainer&&) = delete;
  ~EnsResolverTaskContainer() = default;

  void AddTask(std::unique_ptr<EnsResolverTask> task, ResultCallback cb) {
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
    NOTREACHED_IN_MIGRATION();
  }

  std::vector<ResultCallback> TaskDone(EnsResolverTask* task) {
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
      EnsResolverTask*,
      std::pair<std::unique_ptr<EnsResolverTask>, std::vector<ResultCallback>>>
      tasks_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_RESOLVER_TASK_H_
