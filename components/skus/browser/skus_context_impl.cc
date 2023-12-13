// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_context_impl.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "brave/components/skus/browser/skus_url_loader_impl.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace {

void OnScheduleWakeup(
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::WakeupContext>)> done,
    rust::cxxbridge1::Box<skus::WakeupContext> ctx) {
  done(std::move(ctx));
}

logging::LogSeverity GetLogSeverity(skus::TracingLevel level) {
  switch (level) {
    case skus::TracingLevel::Trace:
      // NOTE: since we have release_max_level_debug set this is equivalent to
      // DVLOG(4)
      return -4;
    case skus::TracingLevel::Debug:
      return -3;
    case skus::TracingLevel::Info:
      return -2;
    case skus::TracingLevel::Warn:
      return logging::LOGGING_VERBOSE;
    case skus::TracingLevel::Error:
      return logging::LOGGING_ERROR;
  }
  // this should never happen, set to WARNING level in this case so we notice
  // since it is otherwise unused
  return logging::LOGGING_WARNING;
}

}  // namespace

namespace skus {

FetchOrderCredentialsCallbackState::FetchOrderCredentialsCallbackState() =
    default;
FetchOrderCredentialsCallbackState::~FetchOrderCredentialsCallbackState() =
    default;

PrepareCredentialsPresentationCallbackState::
    PrepareCredentialsPresentationCallbackState() = default;
PrepareCredentialsPresentationCallbackState::
    ~PrepareCredentialsPresentationCallbackState() = default;

CredentialSummaryCallbackState::CredentialSummaryCallbackState() = default;
CredentialSummaryCallbackState::~CredentialSummaryCallbackState() = default;

RefreshOrderCallbackState::RefreshOrderCallbackState() = default;
RefreshOrderCallbackState::~RefreshOrderCallbackState() = default;

SubmitReceiptCallbackState::SubmitReceiptCallbackState() {}
SubmitReceiptCallbackState::~SubmitReceiptCallbackState() {}

void shim_logMessage(rust::cxxbridge1::Str file,
                     uint32_t line,
                     skus::TracingLevel level,
                     rust::cxxbridge1::Str message) {
  const logging::LogSeverity severity = GetLogSeverity(level);
  if (severity >= 0) {
    if (logging::ShouldCreateLogMessage(severity)) {
      const std::string file_str(file.data(), file.length());
      logging::LogMessage(file_str.c_str(), line, severity).stream()
          << std::string_view(message.data(), message.size());
    }
  } else {
    const std::string file_str(file.data(), file.length());
    if (-severity <= ::logging::GetVlogLevelHelper(file_str.c_str(),
                                                   file_str.length() + 1)) {
      logging::LogMessage(file_str.c_str(), line, severity).stream()
          << std::string_view(message.data(), message.size());
    }
  }
}

void shim_purge(skus::SkusContext& ctx) {  // NOLINT
  ctx.PurgeStore();
}

void shim_set(skus::SkusContext& ctx,  // NOLINT
              rust::cxxbridge1::Str key,
              rust::cxxbridge1::Str value) {
  ctx.UpdateStoreValue(static_cast<std::string>(key),
                       static_cast<std::string>(value));
}

::rust::String shim_get(skus::SkusContext& ctx,  // NOLINT
                        rust::cxxbridge1::Str key) {
  return ::rust::String(ctx.GetValueFromStore(static_cast<std::string>(key)));
}

void shim_scheduleWakeup(
    ::std::uint64_t delay_ms,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::WakeupContext>)> done,
    rust::cxxbridge1::Box<skus::WakeupContext> ctx) {
  int buffer_ms = 10;
  VLOG(1) << "shim_scheduleWakeup " << (delay_ms + buffer_ms) << " ("
          << delay_ms << "ms plus " << buffer_ms << "ms buffer)";
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&OnScheduleWakeup, std::move(done), std::move(ctx)),
      base::Milliseconds(delay_ms + buffer_ms));
}

std::unique_ptr<SkusUrlLoader> shim_executeRequest(
    const skus::SkusContext& ctx,
    const skus::HttpRequest& req,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::HttpRoundtripContext>,
                              skus::HttpResponse)> done,
    rust::cxxbridge1::Box<skus::HttpRoundtripContext> rt_ctx) {
  auto fetcher = ctx.CreateFetcher();
  fetcher->BeginFetch(req, std::move(done), std::move(rt_ctx));
  return fetcher;
}

SkusContextImpl::SkusContextImpl(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(*prefs), url_loader_factory_(url_loader_factory) {}

SkusContextImpl::~SkusContextImpl() = default;

std::unique_ptr<skus::SkusUrlLoader> SkusContextImpl::CreateFetcher() const {
  return std::make_unique<SkusUrlLoaderImpl>(url_loader_factory_);
}

std::string SkusContextImpl::GetValueFromStore(std::string key) const {
  VLOG(1) << "shim_get: `" << key << "`";
  const auto& state = prefs_->GetDict(prefs::kSkusState);
  const base::Value* value = state.Find(key);
  if (value) {
    return value->GetString();
  }
  return "";
}

void SkusContextImpl::PurgeStore() const {
  VLOG(1) << "shim_purge";
  ScopedDictPrefUpdate state(&*prefs_, prefs::kSkusState);
  state->clear();
}

void SkusContextImpl::UpdateStoreValue(std::string key,
                                       std::string value) const {
  VLOG(1) << "shim_set: `" << key << "` = `" << value << "`";
  ScopedDictPrefUpdate state(&*prefs_, prefs::kSkusState);
  state->Set(key, value);
}

}  // namespace skus
