// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_context_impl.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/task/bind_post_task.h"
#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "brave/components/skus/browser/skus_url_loader_impl.h"

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

RustBoundPostTask::RustBoundPostTask(
    base::OnceCallback<void(const std::string&)> callback)
    : callback_(base::BindPostTaskToCurrentDefault(std::move(callback))) {}

RustBoundPostTask::~RustBoundPostTask() = default;

void RustBoundPostTask::Run(SkusResult result) {
  if (callback_) {
    // Call the bound callback with the result from Rust
    std::string error_message;
    if (result != skus::SkusResult::Ok) {
      error_message = std::string{skus::result_to_string(result)};
    }

    std::move(callback_).Run(error_message);
  }
}

void RustBoundPostTask::RunWithResponse(SkusResult result,
                                        rust::cxxbridge1::Str response) {
  if (callback_) {
    // Call the bound callback with the response from Rust
    std::string error_message;
    if (result != skus::SkusResult::Ok) {
      error_message = std::string{skus::result_to_string(result)};
      std::move(callback_).Run(error_message);
    } else {
      std::move(callback_).Run(static_cast<std::string>(response));
    }
  }
}

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

void shim_purge(
    skus::SkusContext& ctx,  // NOLINT
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StoragePurgeContext>,
                              bool success)> done,
    rust::cxxbridge1::Box<skus::StoragePurgeContext> st_ctx) {
  ctx.PurgeStore(std::move(done), std::move(st_ctx));
}

void shim_set(
    skus::SkusContext& ctx,  // NOLINT
    rust::cxxbridge1::Str key,
    rust::cxxbridge1::Str value,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageSetContext>,
                              bool success)> done,
    rust::cxxbridge1::Box<skus::StorageSetContext> st_ctx) {
  ctx.UpdateStoreValue(static_cast<std::string>(key),
                       static_cast<std::string>(value), std::move(done),
                       std::move(st_ctx));
}

void shim_get(
    skus::SkusContext& ctx,  // NOLINT
    rust::cxxbridge1::Str key,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageGetContext>,
                              rust::String value,
                              bool success)> done,
    rust::cxxbridge1::Box<skus::StorageGetContext> st_ctx) {
  ctx.GetValueFromStore(static_cast<std::string>(key), std::move(done),
                        std::move(st_ctx));
}

void shim_scheduleWakeup(
    ::std::uint64_t delay_ms,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::WakeupContext>)> done,
    rust::cxxbridge1::Box<skus::WakeupContext> ctx) {
  int buffer_ms = 10;
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
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_url_loader_factory,
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
    base::WeakPtr<SkusServiceImpl> skus_service)
    : pending_url_loader_factory_(std::move(pending_url_loader_factory)),
      ui_task_runner_(ui_task_runner),
      skus_service_(skus_service) {}
SkusContextImpl::~SkusContextImpl() = default;

std::unique_ptr<skus::SkusUrlLoader> SkusContextImpl::CreateFetcher() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto url_loader_factory = network::SharedURLLoaderFactory::Create(
      std::move(pending_url_loader_factory_));
  pending_url_loader_factory_ = url_loader_factory->Clone();
  return std::make_unique<SkusUrlLoaderImpl>(url_loader_factory);
}

void SkusContextImpl::GetValueFromStore(
    const std::string& key,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageGetContext>,
                              rust::String value,
                              bool success)> done,
    rust::cxxbridge1::Box<skus::StorageGetContext> st_ctx) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&SkusServiceImpl::GetValueFromStore, skus_service_, key,
                     std::move(done), std::move(st_ctx)));
}

void SkusContextImpl::PurgeStore(
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StoragePurgeContext>,
                              bool success)> done,
    rust::cxxbridge1::Box<skus::StoragePurgeContext> st_ctx) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ui_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&SkusServiceImpl::PurgeStore, skus_service_,
                                std::move(done), std::move(st_ctx)));
}

void SkusContextImpl::UpdateStoreValue(
    const std::string& key,
    const std::string& value,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageSetContext>,
                              bool success)> done,
    rust::cxxbridge1::Box<skus::StorageSetContext> st_ctx) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&SkusServiceImpl::UpdateStoreValue, skus_service_, key,
                     value, std::move(done), std::move(st_ctx)));
}

}  // namespace skus
