// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_sdk_context_impl.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/skus_sdk_fetcher_impl.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "services/preferences/public/cpp/dictionary_value_update.h"
#include "services/preferences/public/cpp/scoped_pref_update.h"

namespace {

void OnScheduleWakeup(
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<skus::WakeupContext>)> done,
    rust::cxxbridge1::Box<skus::WakeupContext> ctx) {
  done(std::move(ctx));
}

logging::LogSeverity getLogSeverity(skus::TracingLevel level) {
  switch (level) {
    case skus::TracingLevel::Trace:
      return -2;
    case skus::TracingLevel::Debug:
      return logging::LOGGING_VERBOSE;
    case skus::TracingLevel::Info:
      return logging::LOGGING_INFO;
    case skus::TracingLevel::Warn:
      return logging::LOGGING_WARNING;
    case skus::TracingLevel::Error:
      return logging::LOGGING_ERROR;
  }
  return logging::LOGGING_INFO;
}

}  // namespace

namespace skus {

FetchOrderCredentialsCallbackState::FetchOrderCredentialsCallbackState() {}
FetchOrderCredentialsCallbackState::~FetchOrderCredentialsCallbackState() {}

PrepareCredentialsPresentationCallbackState::
    PrepareCredentialsPresentationCallbackState() {}
PrepareCredentialsPresentationCallbackState::
    ~PrepareCredentialsPresentationCallbackState() {}

CredentialSummaryCallbackState::CredentialSummaryCallbackState() {}
CredentialSummaryCallbackState::~CredentialSummaryCallbackState() {}

RefreshOrderCallbackState::RefreshOrderCallbackState() {}
RefreshOrderCallbackState::~RefreshOrderCallbackState() {}

void shim_logMessage(rust::cxxbridge1::Str file,
                     uint32_t line,
                     skus::TracingLevel level,
                     rust::cxxbridge1::Str message) {
  const logging::LogSeverity severity = getLogSeverity(level);
  const std::string file_str = static_cast<std::string>(file);
  const int vlog_level =
      ::logging::GetVlogLevelHelper(file_str.c_str(), file_str.length() + 1);
  if (severity <= vlog_level) {
    logging::LogMessage(file_str.c_str(), line, severity).stream()
        << static_cast<std::string>(message);
  }
}

void shim_purge(skus::SkusSdkContext& ctx) {
  ctx.PurgeStore();
}

void shim_set(skus::SkusSdkContext& ctx,
              rust::cxxbridge1::Str key,
              rust::cxxbridge1::Str value) {
  ctx.UpdateStoreValue(static_cast<std::string>(key),
                       static_cast<std::string>(value));
}

::rust::String shim_get(skus::SkusSdkContext& ctx,
                        rust::cxxbridge1::Str key) {
  return ::rust::String(ctx.GetValueFromStore(static_cast<std::string>(key)));
}

void shim_scheduleWakeup(
    ::std::uint64_t delay_ms,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<skus::WakeupContext>)> done,
    rust::cxxbridge1::Box<skus::WakeupContext> ctx) {
  VLOG(1) << "shim_scheduleWakeup " << delay_ms;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&OnScheduleWakeup, std::move(done), std::move(ctx)),
      base::TimeDelta::FromMilliseconds(delay_ms));
}

std::unique_ptr<SkusSdkFetcher> shim_executeRequest(
    const skus::SkusSdkContext& ctx,
    const skus::HttpRequest& req,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<skus::HttpRoundtripContext>,
             skus::HttpResponse)> done,
    rust::cxxbridge1::Box<skus::HttpRoundtripContext> rt_ctx) {
  auto fetcher = ctx.CreateFetcher();
  fetcher->BeginFetch(req, std::move(done), std::move(rt_ctx));
  return fetcher;
}

// static
void SkusSdkContextImpl::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kSkusState);
  registry->RegisterBooleanPref(prefs::kSkusVPNHasCredential, false);
}

SkusSdkContextImpl::SkusSdkContextImpl(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs), url_loader_factory_(url_loader_factory) {}

SkusSdkContextImpl::~SkusSdkContextImpl() {}

std::unique_ptr<skus::SkusSdkFetcher>
SkusSdkContextImpl::CreateFetcher() const {
  return std::make_unique<SkusSdkFetcherImpl>(url_loader_factory_);
}

std::string SkusSdkContextImpl::GetValueFromStore(std::string key) const {
  VLOG(1) << "shim_get: `" << key << "`";
  const base::Value* state = prefs_->GetDictionary(prefs::kSkusState);
  DCHECK(state);
  DCHECK(state->is_dict());
  const base::Value* value = state->FindKey(key);
  if (value) {
    return value->GetString();
  }
  return "";
}

void SkusSdkContextImpl::PurgeStore() const {
  VLOG(1) << "shim_purge";
  ::prefs::ScopedDictionaryPrefUpdate update(prefs_, prefs::kSkusState);
  std::unique_ptr<::prefs::DictionaryValueUpdate> state = update.Get();
  DCHECK(state);
  state->Clear();
}

void SkusSdkContextImpl::UpdateStoreValue(std::string key,
                                          std::string value) const {
  VLOG(1) << "shim_set: `" << key << "` = `" << value << "`";
  ::prefs::ScopedDictionaryPrefUpdate update(prefs_, prefs::kSkusState);
  std::unique_ptr<::prefs::DictionaryValueUpdate> state = update.Get();
  DCHECK(state);
  state->SetString(key, value);
}

}  // namespace skus
