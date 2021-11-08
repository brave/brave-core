// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_sdk_context_impl.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/skus/browser/brave-rewards-cxx/src/lib.rs.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/skus_sdk_fetcher_impl.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "services/preferences/public/cpp/dictionary_value_update.h"
#include "services/preferences/public/cpp/scoped_pref_update.h"

namespace {

void OnScheduleWakeup(
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::WakeupContext>)> done,
    rust::cxxbridge1::Box<brave_rewards::WakeupContext> ctx) {
  done(std::move(ctx));
}

logging::LogSeverity getLogSeverity(brave_rewards::TracingLevel level) {
  switch (level) {
    case brave_rewards::TracingLevel::Trace:
      return -2;
    case brave_rewards::TracingLevel::Debug:
      return logging::LOGGING_VERBOSE;
    case brave_rewards::TracingLevel::Info:
      return logging::LOGGING_INFO;
    case brave_rewards::TracingLevel::Warn:
      return logging::LOGGING_WARNING;
    case brave_rewards::TracingLevel::Error:
      return logging::LOGGING_ERROR;
  }
  return logging::LOGGING_INFO;
}

}  // namespace

namespace brave_rewards {

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
                     brave_rewards::TracingLevel level,
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

void shim_purge(brave_rewards::SkusSdkContext& ctx) {
  ctx.PurgeStore();
}

void shim_set(brave_rewards::SkusSdkContext& ctx,
              rust::cxxbridge1::Str key,
              rust::cxxbridge1::Str value) {
  ctx.UpdateStoreValue(static_cast<std::string>(key),
                       static_cast<std::string>(value));
}

::rust::String shim_get(brave_rewards::SkusSdkContext& ctx,
                        rust::cxxbridge1::Str key) {
  return ::rust::String(ctx.GetValueFromStore(static_cast<std::string>(key)));
}

void shim_scheduleWakeup(
    ::std::uint64_t delay_ms,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::WakeupContext>)> done,
    rust::cxxbridge1::Box<brave_rewards::WakeupContext> ctx) {
  LOG(ERROR) << "shim_scheduleWakeup " << delay_ms;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&OnScheduleWakeup, std::move(done), std::move(ctx)),
      base::TimeDelta::FromMilliseconds(delay_ms));
}

std::unique_ptr<SkusSdkFetcher> shim_executeRequest(
    const brave_rewards::SkusSdkContext& ctx,
    const brave_rewards::HttpRequest& req,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
             brave_rewards::HttpResponse)> done,
    rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> rt_ctx) {
  auto fetcher = ctx.CreateFetcher();
  fetcher->BeginFetch(req, std::move(done), std::move(rt_ctx));
  return fetcher;
}

// static
void SkusSdkContextImpl::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kSkusDictionary);
  registry->RegisterStringPref(prefs::kSkusVPNCredential, "");
}

SkusSdkContextImpl::SkusSdkContextImpl(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs), url_loader_factory_(url_loader_factory) {}

SkusSdkContextImpl::~SkusSdkContextImpl() {}

std::unique_ptr<brave_rewards::SkusSdkFetcher>
SkusSdkContextImpl::CreateFetcher() const {
  return std::make_unique<SkusSdkFetcherImpl>(url_loader_factory_);
}

std::string SkusSdkContextImpl::GetValueFromStore(std::string key) const {
  LOG(ERROR) << "shim_get: `" << key << "`";
  const base::Value* dictionary = prefs_->GetDictionary(prefs::kSkusDictionary);
  DCHECK(dictionary);
  DCHECK(dictionary->is_dict());
  const base::Value* value = dictionary->FindKey(key);
  if (value) {
    return value->GetString();
  }
  return "{}";
}

void SkusSdkContextImpl::PurgeStore() const {
  LOG(ERROR) << "shim_purge";
  ::prefs::ScopedDictionaryPrefUpdate update(prefs_, prefs::kSkusDictionary);
  std::unique_ptr<::prefs::DictionaryValueUpdate> dictionary = update.Get();
  DCHECK(dictionary);
  dictionary->Clear();
}

void SkusSdkContextImpl::UpdateStoreValue(std::string key,
                                          std::string value) const {
  LOG(ERROR) << "shim_set: `" << key << "` = `" << value << "`";
  ::prefs::ScopedDictionaryPrefUpdate update(prefs_, prefs::kSkusDictionary);
  std::unique_ptr<::prefs::DictionaryValueUpdate> dictionary = update.Get();
  DCHECK(dictionary);
  dictionary->SetString(key, value);
}

}  // namespace brave_rewards
