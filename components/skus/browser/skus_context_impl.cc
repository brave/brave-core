// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_context_impl.h"

#include <string>
#include <utility>

#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace {

void OnScheduleWakeup(
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::WakeupContext>)> done,
    rust::cxxbridge1::Box<skus::WakeupContext> ctx) {
  done(std::move(ctx));
}

const net::NetworkTrafficAnnotationTag& GetNetworkTrafficAnnotationTag() {
  static const net::NetworkTrafficAnnotationTag network_traffic_annotation_tag =
      net::DefineNetworkTrafficAnnotation("sku_sdk_execute_request", R"(
      semantics {
        sender: "Brave SKU SDK"
        description:
          "Call the SKU SDK implementation provided by the caller"
        trigger:
          "Any Brave webpage using SKU SDK where window.chrome.braveSkus.*"
          "methods are called; ex: fetch_order / fetch_order_credentials"
        data: "JSON data comprising an order."
        destination: OTHER
        destination_other: "Brave developers"
      }
      policy {
        cookies_allowed: NO
      })");
  return network_traffic_annotation_tag;
}

}  // namespace

namespace skus {

RustBoundPostTask::RustBoundPostTask(
    base::OnceCallback<void(skus::mojom::SkusResultPtr)> callback)
    : callback_(base::BindPostTaskToCurrentDefault(std::move(callback))) {}

RustBoundPostTask::~RustBoundPostTask() = default;

void RustBoundPostTask::Run(SkusResult result) {
  if (callback_) {
    // Call the bound callback with the result from Rust
    std::move(callback_).Run(skus::mojom::SkusResult::New(
        result.code, static_cast<std::string>(result.msg)));
  }
}

void RustBoundPostTask::RunWithResponse(SkusResult result,
                                        rust::cxxbridge1::Str response) {
  if (callback_) {
    // Call the bound callback with the response from Rust
    if (result.code == skus::mojom::SkusResultCode::Ok) {
      std::move(callback_).Run(skus::mojom::SkusResult::New(
          skus::mojom::SkusResultCode::Ok, static_cast<std::string>(response)));
    } else {
      std::move(callback_).Run(skus::mojom::SkusResult::New(
          result.code, static_cast<std::string>(result.msg)));
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

std::unique_ptr<mojo::rust::ScopedMessagePipeHandleWrapper>
shim_createUrlLoader(skus::SkusContext& ctx) {  // NOLINT
  return ctx.CreateUrlLoader();
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

std::unique_ptr<mojo::rust::ScopedMessagePipeHandleWrapper>
SkusContextImpl::CreateUrlLoader() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!url_loader_service_) {
    // SharedURLLoaderFactory must be created on the sequence that uses it,
    // which is why the factory is carried here in its pending form.
    url_loader_service_ = {new brave::network::SimpleUrlLoaderService(
                               network::SharedURLLoaderFactory::Create(
                                   std::move(pending_url_loader_factory_)),
                               GetNetworkTrafficAnnotationTag()),
                           base::OnTaskRunnerDeleter(
                               base::SequencedTaskRunner::GetCurrentDefault())};
  }

  mojo::PendingRemote<brave::network::mojom::SimpleUrlLoader> remote;
  url_loader_service_->Bind(remote.InitWithNewPipeAndPassReceiver());
  return std::make_unique<mojo::rust::ScopedMessagePipeHandleWrapper>(
      remote.PassPipe());
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
