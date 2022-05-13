/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/unstoppable_domains_multichain_calls.h"

#include <utility>
#include <vector>

#include "url/gurl.h"

namespace brave_wallet::unstoppable_domains {

template <class ResultType>
bool MultichainCall<ResultType>::SetNoResult(const std::string& chain_id) {
  DCHECK(!responses_.count(chain_id));
  responses_[chain_id] = {};

  return MaybeResolveCallbacks();
}

template <class ResultType>
bool MultichainCall<ResultType>::SetResult(const std::string& chain_id,
                                           ResultType result) {
  DCHECK(!responses_.count(chain_id));
  responses_[chain_id].result = std::move(result);

  return MaybeResolveCallbacks();
}

template <class ResultType>
bool MultichainCall<ResultType>::SetError(const std::string& chain_id,
                                          mojom::ProviderError error,
                                          std::string error_message) {
  DCHECK(!responses_.count(chain_id));
  responses_[chain_id].error = std::move(error);
  responses_[chain_id].error_message = std::move(error_message);

  return MaybeResolveCallbacks();
}

template <class ResultType>
void MultichainCall<ResultType>::AddCallback(CallbackType cb) {
  callbacks_.push_back(std::move(cb));
}

template <class ResultType>
typename MultichainCall<ResultType>::Response*
MultichainCall<ResultType>::GetEffectiveResponse() {
  auto polygon_result = responses_.find(mojom::kPolygonMainnetChainId);
  if (polygon_result == responses_.end()) {
    return nullptr;
  }

  auto eth_mainnet_result = responses_.find(mojom::kMainnetChainId);
  if (eth_mainnet_result == responses_.end()) {
    return nullptr;
  }

  if (polygon_result->second.result || polygon_result->second.error)
    return &polygon_result->second;

  return &eth_mainnet_result->second;
}

template <class ResultType>
bool MultichainCall<ResultType>::MaybeResolveCallbacks() {
  auto* response = GetEffectiveResponse();
  if (!response)
    return false;

  for (auto& callback : callbacks_) {
    std::move(callback).Run(
        response->result.value_or(ResultType()),
        response->error.value_or(mojom::ProviderError::kSuccess),
        response->error_message.value_or(""));
  }

  return true;
}

template <class ResultType>
std::vector<std::string> MultichainCalls<ResultType>::GetChains() const {
  return {mojom::kPolygonMainnetChainId, mojom::kMainnetChainId};
}

template <class ResultType>
bool MultichainCalls<ResultType>::HasCall(const std::string& domain) {
  return calls_.count(domain) > 0;
}

template <class ResultType>
void MultichainCalls<ResultType>::AddCallback(const std::string& domain,
                                              CallbackType callback) {
  calls_[domain].AddCallback(std::move(callback));
}

template <class ResultType>
void MultichainCalls<ResultType>::SetNoResult(const std::string& domain,
                                              const std::string& chain_id) {
  auto call = calls_.find(domain);
  if (call == calls_.end())
    return;

  if (call->second.SetNoResult(chain_id)) {
    calls_.erase(call);
  }
}

template <class ResultType>
void MultichainCalls<ResultType>::SetResult(const std::string& domain,
                                            const std::string& chain_id,
                                            ResultType result) {
  auto call = calls_.find(domain);
  if (call == calls_.end())
    return;

  if (call->second.SetResult(chain_id, std::move(result))) {
    calls_.erase(call);
  }
}

template <class ResultType>
void MultichainCalls<ResultType>::SetError(const std::string& domain,
                                           const std::string& chain_id,
                                           mojom::ProviderError error,
                                           std::string error_message) {
  auto call = calls_.find(domain);
  if (call == calls_.end())
    return;

  if (call->second.SetError(chain_id, std::move(error),
                            std::move(error_message))) {
    calls_.erase(call);
  }
}

template class MultichainCalls<std::string>;
template class MultichainCalls<GURL>;

}  // namespace brave_wallet::unstoppable_domains
