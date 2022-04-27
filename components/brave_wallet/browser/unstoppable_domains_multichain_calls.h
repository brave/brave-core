/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_UNSTOPPABLE_DOMAINS_MULTICHAIN_CALLS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_UNSTOPPABLE_DOMAINS_MULTICHAIN_CALLS_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet::unstoppable_domains {

template <class ResultType>
class MultichainCall {
 public:
  using CallbackType = base::OnceCallback<
      void(const ResultType&, mojom::ProviderError, const std::string&)>;

  struct Response {
    absl::optional<ResultType> result;
    absl::optional<mojom::ProviderError> error;
    absl::optional<std::string> error_message;
  };

  MultichainCall() = default;
  ~MultichainCall() = default;
  MultichainCall(const MultichainCall&) = delete;
  MultichainCall& operator=(const MultichainCall&) = delete;
  MultichainCall(MultichainCall&&) = default;
  MultichainCall& operator=(MultichainCall&&) = default;

  bool SetNoResult(const std::string& chain_id);
  bool SetResult(const std::string& chain_id, ResultType result);
  bool SetError(const std::string& chain_id,
                mojom::ProviderError error,
                std::string error_message);
  void AddCallback(CallbackType cb);

 private:
  Response* GetEffectiveResponse();
  bool MaybeResolveCallbacks();

  // chain_id -> response.
  base::flat_map<std::string, Response> responses_;
  std::vector<CallbackType> callbacks_;
};

template <class ResultType>
class MultichainCalls {
 public:
  using CallbackType = typename MultichainCall<ResultType>::CallbackType;

  MultichainCalls() = default;
  ~MultichainCalls() = default;

  std::vector<std::string> GetChains() const;

  bool HasCall(const std::string& domain);
  void AddCallback(const std::string& domain, CallbackType callback);
  void SetNoResult(const std::string& domain, const std::string& chain_id);
  void SetResult(const std::string& domain,
                 const std::string& chain_id,
                 ResultType result);
  void SetError(const std::string& domain,
                const std::string& chain_id,
                mojom::ProviderError error,
                std::string error_message);

 private:
  // domain -> call
  base::flat_map<std::string, MultichainCall<ResultType>> calls_;
};
}  // namespace brave_wallet::unstoppable_domains

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_UNSTOPPABLE_DOMAINS_MULTICHAIN_CALLS_H_
