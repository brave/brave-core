// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_BRAVE_REWARDS_CXX_SRC_SHIM_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_BRAVE_REWARDS_CXX_SRC_SHIM_H_

#include <functional>
#include <memory>

// NOTE: when running cxxbridge or examples/main.cc, comment out the following
// two lines and instead `#include "cxx.h"` (please don't commit that change).
#include "brave/components/skus/browser/brave-rewards-cxx/src/cxx.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"

namespace brave_rewards {

enum class RewardsResult : uint8_t;
struct HttpRequest;
struct HttpResponse;
struct HttpRoundtripContext;
struct WakeupContext;
class SkusSdkImpl;
class SkusSdkFetcher;

class RefreshOrderCallbackState {
 public:
  skus::mojom::SkusSdk::RefreshOrderCallback cb;
  SkusSdkImpl* instance;
  SkusSdkFetcher* fetcher;
};
class FetchOrderCredentialsCallbackState {
 public:
  skus::mojom::SkusSdk::FetchOrderCredentialsCallback cb;
  SkusSdkImpl* instance;
  SkusSdkFetcher* fetcher;
};
class PrepareCredentialsPresentationCallbackState {
 public:
  skus::mojom::SkusSdk::PrepareCredentialsPresentationCallback cb;
  SkusSdkImpl* instance;
  SkusSdkFetcher* fetcher;
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

void shim_purge();
void shim_set(rust::cxxbridge1::Str key, rust::cxxbridge1::Str value);
::rust::String shim_get(rust::cxxbridge1::Str key);

void shim_scheduleWakeup(
    ::std::uint64_t delay_ms,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::WakeupContext>)> done,
    rust::cxxbridge1::Box<brave_rewards::WakeupContext> ctx);

void shim_executeRequest(
    const brave_rewards::HttpRequest& req,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
             brave_rewards::HttpResponse)> done,
    rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx);
}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_BRAVE_REWARDS_CXX_SRC_SHIM_H_
