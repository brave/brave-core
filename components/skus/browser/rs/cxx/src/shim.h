// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_RS_CXX_SRC_SHIM_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_RS_CXX_SRC_SHIM_H_

#include <functional>
#include <memory>
#include <string>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "third_party/rust/chromium_crates_io/vendor/cxx-1.0.117/include/cxx.h"

class PrefService;

namespace skus {

enum class SkusResult : uint8_t;
enum class TracingLevel : uint8_t;
struct HttpRequest;
struct HttpResponse;
struct HttpRoundtripContext;
struct WakeupContext;
struct StoragePurgeContext;
struct StorageSetContext;
struct StorageGetContext;

class FetchOrderCredentialsCallbackState {
 public:
  FetchOrderCredentialsCallbackState();
  ~FetchOrderCredentialsCallbackState();
  base::OnceCallback<void(const std::string&)> cb;
};

class PrepareCredentialsPresentationCallbackState {
 public:
  PrepareCredentialsPresentationCallbackState();
  ~PrepareCredentialsPresentationCallbackState();
  base::OnceCallback<void(const std::string&)> cb;
};

class CredentialSummaryCallbackState {
 public:
  CredentialSummaryCallbackState();
  ~CredentialSummaryCallbackState();
  base::OnceCallback<void(const std::string&)> cb;
};

class RefreshOrderCallbackState {
 public:
  RefreshOrderCallbackState();
  ~RefreshOrderCallbackState();
  base::OnceCallback<void(const std::string&)> cb;
};

class SubmitReceiptCallbackState {
 public:
  SubmitReceiptCallbackState();
  ~SubmitReceiptCallbackState();
  base::OnceCallback<void(const std::string&)> cb;
};

class CreateOrderFromReceiptCallbackState {
 public:
  CreateOrderFromReceiptCallbackState();
  ~CreateOrderFromReceiptCallbackState();
  base::OnceCallback<void(const std::string&)> cb;
};

class SkusUrlLoader {
 public:
  virtual ~SkusUrlLoader() = default;
  virtual void BeginFetch(
      const skus::HttpRequest& req,
      rust::cxxbridge1::Fn<
          void(rust::cxxbridge1::Box<skus::HttpRoundtripContext>,
               skus::HttpResponse)> callback,
      rust::cxxbridge1::Box<skus::HttpRoundtripContext> ctx) = 0;
};

class SkusContext {
 public:
  virtual ~SkusContext() = default;
  virtual std::unique_ptr<skus::SkusUrlLoader> CreateFetcher() const = 0;
  virtual void GetValueFromStore(
      const std::string& key,
      rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageGetContext>,
                                rust::String value,
                                bool success)> done,
      rust::cxxbridge1::Box<skus::StorageGetContext> st_ctx) const = 0;
  virtual void PurgeStore(
      rust::cxxbridge1::Fn<
          void(rust::cxxbridge1::Box<skus::StoragePurgeContext>, bool success)>
          done,
      rust::cxxbridge1::Box<skus::StoragePurgeContext> st_ctx) const = 0;
  virtual void UpdateStoreValue(
      const std::string& key,
      const std::string& value,
      rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageSetContext>,
                                bool success)> done,
      rust::cxxbridge1::Box<skus::StorageSetContext> st_ctx) const = 0;
};

using RefreshOrderCallback = void (*)(RefreshOrderCallbackState* callback_state,
                                      SkusResult result,
                                      rust::cxxbridge1::Str order);
using FetchOrderCredentialsCallback =
    void (*)(FetchOrderCredentialsCallbackState* callback_state,
             SkusResult result);
using PrepareCredentialsPresentationCallback =
    void (*)(PrepareCredentialsPresentationCallbackState* callback_state,
             SkusResult result,
             rust::cxxbridge1::Str presentation);
using CredentialSummaryCallback =
    void (*)(CredentialSummaryCallbackState* callback_state,
             SkusResult result,
             rust::cxxbridge1::Str summary);
using SubmitReceiptCallback =
    void (*)(SubmitReceiptCallbackState* callback_state, SkusResult result);

using CreateOrderFromReceiptCallback =
    void (*)(CreateOrderFromReceiptCallbackState* callback_state,
             SkusResult result,
             rust::cxxbridge1::Str order_id);

void shim_logMessage(rust::cxxbridge1::Str file,
                     uint32_t line,
                     TracingLevel level,
                     rust::cxxbridge1::Str message);

void shim_purge(
    skus::SkusContext& ctx,  // NOLINT
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StoragePurgeContext>,
                              bool success)> done,
    rust::cxxbridge1::Box<skus::StoragePurgeContext> st_ctx);
void shim_set(
    skus::SkusContext& ctx,  // NOLINT
    rust::cxxbridge1::Str key,
    rust::cxxbridge1::Str value,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageSetContext>,
                              bool success)> done,
    rust::cxxbridge1::Box<skus::StorageSetContext> st_ctx);
void shim_get(
    skus::SkusContext& ctx,  // NOLINT
    rust::cxxbridge1::Str key,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageGetContext>,
                              rust::String value,
                              bool success)> done,
    rust::cxxbridge1::Box<skus::StorageGetContext> st_ctx);

void shim_scheduleWakeup(
    ::std::uint64_t delay_ms,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::WakeupContext>)> done,
    rust::cxxbridge1::Box<skus::WakeupContext> ctx);

std::unique_ptr<SkusUrlLoader> shim_executeRequest(
    const skus::SkusContext& ctx,
    const skus::HttpRequest& req,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::HttpRoundtripContext>,
                              skus::HttpResponse)> done,
    rust::cxxbridge1::Box<skus::HttpRoundtripContext> rt_ctx);
}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_RS_CXX_SRC_SHIM_H_
