/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_RESOLVER_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_RESOLVER_TASK_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

// Selector for "OffchainLookup(address,string[],bytes,bytes4,bytes)"
constexpr uint8_t kOffchainLookupSelector[] = {0x55, 0x6f, 0x18, 0x30};

// Selector for resolve(bytes,bytes)
constexpr uint8_t kResolveBytesBytesSelector[] = {0x90, 0x61, 0xb9, 0x23};

struct OffchainLookupData {
  OffchainLookupData();
  OffchainLookupData(const OffchainLookupData&);
  OffchainLookupData(OffchainLookupData&&);
  OffchainLookupData& operator=(const OffchainLookupData&);
  OffchainLookupData& operator=(OffchainLookupData&&);
  ~OffchainLookupData();

  static absl::optional<OffchainLookupData> ExtractFromJson(
      const std::string& json);
  static absl::optional<OffchainLookupData> ExtractFromEthAbiPayload(
      eth_abi::Span bytes);

  EthAddress sender;
  std::vector<std::string> urls;
  std::vector<uint8_t> call_data;
  std::vector<uint8_t> callback_function;
  std::vector<uint8_t> extra_data;
};

class EnsResolverTask {
 public:
  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using DoneCallback =
      base::OnceCallback<void(EnsResolverTask* task,
                              std::vector<uint8_t> resolved_result,
                              mojom::ProviderError error,
                              std::string error_message)>;
  using RequestIntermediateCallback = base::OnceCallback<void(
      int http_code,
      const std::string& response,
      const base::flat_map<std::string, std::string>& headers)>;

  EnsResolverTask(DoneCallback done_callback,
                  APIRequestHelper* api_request_helper,
                  APIRequestHelper* api_request_helper_ens_offchain,
                  std::vector<uint8_t> ens_call,
                  const std::string& domain,
                  const GURL& network_url);
  EnsResolverTask(const EnsResolverTask&) = delete;
  EnsResolverTask& operator=(const EnsResolverTask&) = delete;
  ~EnsResolverTask();

  const std::string& domain() const { return domain_; }

  void WorkOnTask();

 private:
  void FetchEnsResolver();
  void OnFetchEnsResolverDone(
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void FetchEnsip10Support();
  void OnFetchEnsip10SupportDone(
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void FetchEnsRecord();
  void OnFetchEnsRecordDone(
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void FetchWithEnsip10Resolve();
  void OnFetchWithEnsip10ResolveDone(
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void FetchOffchainData();
  void OnFetchOffchainDone(
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void RequestInternal(const std::string& json_payload,
                       RequestIntermediateCallback callback);

  DoneCallback done_callback_;
  raw_ptr<APIRequestHelper> api_request_helper_;
  raw_ptr<APIRequestHelper> api_request_helper_ens_offchain_;
  std::vector<uint8_t> ens_call_;
  std::string domain_;
  absl::optional<std::vector<uint8_t>> dns_encoded_name_;
  GURL network_url_;
  int offchain_lookup_attemps_left_ = 4;

  absl::optional<std::vector<uint8_t>> resolve_result_;

  absl::optional<mojom::ProviderError> error_;
  absl::optional<std::string> error_message_;
  std::vector<uint8_t> ens_resolve_call_;
  absl::optional<std::string> resolver_;
  absl::optional<bool> supports_ensip_10_;
  absl::optional<OffchainLookupData> offchain_lookup_data_;

  base::WeakPtrFactory<EnsResolverTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_RESOLVER_TASK_H_
