// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_BRAVE_REWARDS_CXX_SRC_SHIM_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_BRAVE_REWARDS_CXX_SRC_SHIM_H_

#include <functional>
#include <memory>

#ifndef NOT_BRAVE_CORE_SHIM
#include "brave/components/skus/browser/skus_sdk_context.h"
#include "brave/components/skus/browser/skus_sdk_fetcher.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#else
#include "cxx.h"
namespace brave_rewards {
class SkusSdkContext {};
class SkusSdkFetcher {};
}  // namespace brave_rewards
#endif  // NOT_BRAVE_CORE_SHIM

namespace brave_rewards {

enum class RewardsResult : uint8_t;
struct HttpRequest;
struct HttpResponse;
struct HttpRoundtripContext;
struct WakeupContext;
class SkusSdkFetcher;

class RefreshOrderCallbackState {
#ifndef NOT_BRAVE_CORE_SHIM
 public:
  skus::mojom::SkusSdk::RefreshOrderCallback cb;
#endif  // NOT_BRAVE_CORE_SHIM
};
class FetchOrderCredentialsCallbackState {
#ifndef NOT_BRAVE_CORE_SHIM
 public:
  skus::mojom::SkusSdk::FetchOrderCredentialsCallback cb;
#endif  // NOT_BRAVE_CORE_SHIM
};
class PrepareCredentialsPresentationCallbackState {
#ifndef NOT_BRAVE_CORE_SHIM
 public:
  skus::mojom::SkusSdk::PrepareCredentialsPresentationCallback cb;
#endif  // NOT_BRAVE_CORE_SHIM
};
class CredentialSummaryCallbackState {
#ifndef NOT_BRAVE_CORE_SHIM
 public:
  skus::mojom::SkusSdk::CredentialSummaryCallback cb;
#endif  // NOT_BRAVE_CORE_SHIM
};

using RefreshOrderCallback = void (*)(RefreshOrderCallbackState* callback_state,
                                      RewardsResult result,
                                      rust::cxxbridge1::Str order);
using FetchOrderCredentialsCallback =
    void (*)(FetchOrderCredentialsCallbackState* callback_state,
             RewardsResult result);
using PrepareCredentialsPresentationCallback =
    void (*)(PrepareCredentialsPresentationCallbackState* callback_state,
             RewardsResult result,
             rust::cxxbridge1::Str presentation);
using CredentialSummaryCallback =
    void (*)(CredentialSummaryCallbackState* callback_state,
             RewardsResult result,
             rust::cxxbridge1::Str summary);

void shim_purge(brave_rewards::SkusSdkContext& ctx);
void shim_set(brave_rewards::SkusSdkContext& ctx,
              rust::cxxbridge1::Str key,
              rust::cxxbridge1::Str value);
::rust::String shim_get(brave_rewards::SkusSdkContext& ctx,
                        rust::cxxbridge1::Str key);

void shim_scheduleWakeup(
    ::std::uint64_t delay_ms,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::WakeupContext>)> done,
    rust::cxxbridge1::Box<brave_rewards::WakeupContext> ctx);

std::unique_ptr<SkusSdkFetcher> shim_executeRequest(
    const brave_rewards::SkusSdkContext& ctx,
    const brave_rewards::HttpRequest& req,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
             brave_rewards::HttpResponse)> done,
    rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> rt_ctx);
}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_BRAVE_REWARDS_CXX_SRC_SHIM_H_
