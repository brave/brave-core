// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_BRAVE_REWARDS_CXX_SRC_SHIM_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_BRAVE_REWARDS_CXX_SRC_SHIM_H_

#include <functional>
#include <memory>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "brave/third_party/rust/cxx/include/cxx.h"

class PrefService;

namespace brave_rewards {

enum class RewardsResult : uint8_t;
enum class TracingLevel : uint8_t;
struct HttpRequest;
struct HttpResponse;
struct HttpRoundtripContext;
struct WakeupContext;

class FetchOrderCredentialsCallbackState {
 public:
  FetchOrderCredentialsCallbackState();
  ~FetchOrderCredentialsCallbackState();
  base::OnceCallback<void(const std::string&)> cb;
  std::string order_id;
};

class PrepareCredentialsPresentationCallbackState {
 public:
  PrepareCredentialsPresentationCallbackState();
  ~PrepareCredentialsPresentationCallbackState();
  base::OnceCallback<void(const std::string&)> cb;
  std::string domain;
  PrefService* prefs;
};

class CredentialSummaryCallbackState {
 public:
  CredentialSummaryCallbackState();
  ~CredentialSummaryCallbackState();
  base::OnceCallback<void(const std::string&)> cb;
  std::string domain;
  PrefService* prefs;
};

class RefreshOrderCallbackState {
 public:
  RefreshOrderCallbackState();
  ~RefreshOrderCallbackState();
  base::OnceCallback<void(const std::string&)> cb;
};

class SkusSdkFetcher {
 public:
  virtual ~SkusSdkFetcher() = default;
  virtual void BeginFetch(
      const brave_rewards::HttpRequest& req,
      rust::cxxbridge1::Fn<
          void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
               brave_rewards::HttpResponse)> callback,
      rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx) = 0;
};

class SkusSdkContext {
 public:
  virtual ~SkusSdkContext() = default;
  virtual std::unique_ptr<brave_rewards::SkusSdkFetcher> CreateFetcher()
      const = 0;
  virtual std::string GetValueFromStore(std::string key) const = 0;
  virtual void PurgeStore() const = 0;
  virtual void UpdateStoreValue(std::string key, std::string value) const = 0;
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

void shim_logMessage(rust::cxxbridge1::Str file,
              uint32_t line,
              TracingLevel level,
              rust::cxxbridge1::Str message);

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
