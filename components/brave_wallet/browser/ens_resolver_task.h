/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_RESOLVER_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_RESOLVER_TASK_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/json_rpc_service_base.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

struct OffchainLookupData {
  OffchainLookupData();
  OffchainLookupData(const OffchainLookupData&);
  OffchainLookupData(OffchainLookupData&&);
  OffchainLookupData& operator=(const OffchainLookupData&);
  OffchainLookupData& operator=(OffchainLookupData&&);
  ~OffchainLookupData();

  EthAddress sender;
  std::vector<std::string> urls;
  std::vector<uint8_t> call_data;
  std::vector<uint8_t> callback_function;
  std::vector<uint8_t> extra_data;
};

class EnsGetEthAddrTask {
 public:
  EnsGetEthAddrTask(
      JsonRpcServiceBase* json_rpc_service_base,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::vector<uint8_t> ens_call,
      const std::string& domain,
      const std::vector<uint8_t>& dns_encoded_name,
      const GURL& network_url);
  ~EnsGetEthAddrTask();

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
  void OnFetchOffchainDone(std::unique_ptr<std::string> response_body);

  raw_ptr<JsonRpcServiceBase> json_rpc_service_base_ = nullptr;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::vector<uint8_t> ens_call_;
  std::string domain_;
  std::vector<uint8_t> dns_encoded_name_;
  GURL network_url_;
  int offchain_lookup_attemps_left_ = 4;

  absl::optional<std::vector<uint8_t>> resolve_result_;

  absl::optional<mojom::ProviderError> error_;
  absl::optional<std::string> error_message_;
  std::vector<uint8_t> ens_resolve_call_;
  absl::optional<std::string> resolver_;
  absl::optional<bool> supports_ensip_10_;
  absl::optional<OffchainLookupData> offchain_lookup_data_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  base::WeakPtrFactory<EnsGetEthAddrTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_RESOLVER_TASK_H_
