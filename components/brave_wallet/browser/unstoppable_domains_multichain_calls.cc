/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/unstoppable_domains_multichain_calls.h"

#include <optional>
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

  auto base_result = responses_.find(mojom::kBaseMainnetChainId);
  if (base_result == responses_.end()) {
    return nullptr;
  }

  auto eth_mainnet_result = responses_.find(mojom::kMainnetChainId);
  if (eth_mainnet_result == responses_.end()) {
    return nullptr;
  }

  if (polygon_result->second.result || polygon_result->second.error) {
    return &polygon_result->second;
  }

  if (base_result->second.result || base_result->second.error) {
    return &base_result->second;
  }

  return &eth_mainnet_result->second;
}

template <class ResultType>
bool MultichainCall<ResultType>::MaybeResolveCallbacks() {
  auto* response = GetEffectiveResponse();
  if (!response) {
    return false;
  }

  for (auto& callback : callbacks_) {
    std::move(callback).Run(
        response->result.value_or(ResultType()),
        response->error.value_or(mojom::ProviderError::kSuccess),
        response->error_message.value_or(""));
  }

  return true;
}

template <class KeyType, class ResultType>
std::vector<std::string> MultichainCalls<KeyType, ResultType>::GetChains()
    const {
  return {mojom::kPolygonMainnetChainId, mojom::kBaseMainnetChainId,
          mojom::kMainnetChainId};
}

template <class KeyType, class ResultType>
bool MultichainCalls<KeyType, ResultType>::HasCall(const KeyType& key) {
  return calls_.count(key) > 0;
}

template <class KeyType, class ResultType>
void MultichainCalls<KeyType, ResultType>::AddCallback(const KeyType& key,
                                                       CallbackType callback) {
  calls_[key].AddCallback(std::move(callback));
}

template <class KeyType, class ResultType>
void MultichainCalls<KeyType, ResultType>::SetNoResult(
    const KeyType& key,
    const std::string& chain_id) {
  auto call = calls_.find(key);
  if (call == calls_.end()) {
    return;
  }

  if (call->second.SetNoResult(chain_id)) {
    calls_.erase(call);
  }
}

template <class KeyType, class ResultType>
void MultichainCalls<KeyType, ResultType>::SetResult(
    const KeyType& key,
    const std::string& chain_id,
    ResultType result) {
  auto call = calls_.find(key);
  if (call == calls_.end()) {
    return;
  }

  if (call->second.SetResult(chain_id, std::move(result))) {
    calls_.erase(call);
  }
}

template <class KeyType, class ResultType>
void MultichainCalls<KeyType, ResultType>::SetError(const KeyType& key,
                                                    const std::string& chain_id,
                                                    mojom::ProviderError error,
                                                    std::string error_message) {
  auto call = calls_.find(key);
  if (call == calls_.end()) {
    return;
  }

  if (call->second.SetError(chain_id, std::move(error),
                            std::move(error_message))) {
    calls_.erase(call);
  }
}

template class MultichainCalls<std::string, std::string>;
template class MultichainCalls<WalletAddressKey, std::string>;
template class MultichainCalls<std::string, std::optional<GURL>>;

}  // namespace brave_wallet::unstoppable_domains
