// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/core/local_ai_service_base.h"

#include <utility>

#include "base/barrier_closure.h"
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/logging.h"

namespace local_ai {

LocalAIServiceBase::PendingConsumer::PendingConsumer(
    base::OnceClosure on_ready,
    base::OnceClosure on_cancel)
    : on_ready(std::move(on_ready)), on_cancel(std::move(on_cancel)) {}
LocalAIServiceBase::PendingConsumer::~PendingConsumer() = default;
LocalAIServiceBase::PendingConsumer::PendingConsumer(PendingConsumer&&) =
    default;
LocalAIServiceBase::PendingConsumer&
LocalAIServiceBase::PendingConsumer::operator=(PendingConsumer&&) = default;

LocalAIServiceBase::LocalAIServiceBase(BackgroundWebContentsFactory bwc_factory,
                                       bool models_already_available)
    : bwc_factory_(std::move(bwc_factory)),
      model_files_ready_(models_already_available) {
  InitModelLoadBarrier();
}

LocalAIServiceBase::~LocalAIServiceBase() = default;

bool LocalAIServiceBase::ReadyToServe() {
  return model_ready_ && IsFactoryBound();
}

void LocalAIServiceBase::QueueConsumer(base::OnceClosure on_ready,
                                       base::OnceClosure on_cancel) {
  pending_consumers_.push_back({std::move(on_ready), std::move(on_cancel)});
}

void LocalAIServiceBase::HandleFactoryRegistered() {
  CHECK(bwc_);
  SetFactoryDisconnectHandler(base::BindOnce(
      &LocalAIServiceBase::OnFactoryDisconnected, weak_factory_.GetWeakPtr()));
  DVLOG(3) << "LocalAIServiceBase: Factory registered";
  if (model_load_barrier_) {
    model_load_barrier_.Run();
  }
}

void LocalAIServiceBase::CancelPendingAndResetFactory() {
  ResetFactory();
  CancelAllPending();
}

void LocalAIServiceBase::SignalModelsAvailable() {
  model_files_ready_ = true;
  if (model_load_barrier_) {
    model_load_barrier_.Run();
  }
}

void LocalAIServiceBase::NotifyIdle() {
  DVLOG(3) << "LocalAIServiceBase: "
              "All consumers disconnected";
  CloseBWC();
}

void LocalAIServiceBase::OnFactoryDisconnected() {
  model_ready_ = false;
  CancelPendingAndResetFactory();
  InitModelLoadBarrier();
}

void LocalAIServiceBase::OnBackgroundContentsDestroyed(
    BackgroundWebContents::DestroyReason reason) {
  DVLOG(1) << "LocalAIServiceBase: "
              "Background contents destroyed";
  CloseBWC();
}

void LocalAIServiceBase::Shutdown() {
  DVLOG(3) << "LocalAIServiceBase: Shutting down";
  OnShutdownExtra();
  weak_factory_.InvalidateWeakPtrs();
  CloseBWC();
}

void LocalAIServiceBase::MaybeCreateBWC() {
  if (bwc_) {
    return;
  }
  DVLOG(3) << "LocalAIServiceBase: "
              "Creating background contents";
  bwc_ = bwc_factory_.Run(this);
}

void LocalAIServiceBase::CloseBWC() {
  CancelPendingAndResetFactory();
  bwc_.reset();
  model_ready_ = false;
  InitModelLoadBarrier();
}

void LocalAIServiceBase::InitModelLoadBarrier() {
  model_load_barrier_ = base::BarrierClosure(
      2, base::BindOnce(&LocalAIServiceBase::OnModelLoadBarrierMet,
                        weak_factory_.GetWeakPtr()));
  if (model_files_ready_) {
    model_load_barrier_.Run();
  }
}

void LocalAIServiceBase::OnModelLoadBarrierMet() {
  LoadAndInitModel(base::BindOnce(&LocalAIServiceBase::OnLoadAndInitComplete,
                                  weak_factory_.GetWeakPtr()));
}

void LocalAIServiceBase::LoadAndInitModel(
    base::OnceCallback<void(bool)> on_complete) {
  LoadModelFiles(base::BindOnce(&LocalAIServiceBase::OnLoadDone,
                                weak_factory_.GetWeakPtr(),
                                std::move(on_complete)));
}

void LocalAIServiceBase::OnLoadDone(base::OnceCallback<void(bool)> on_complete,
                                    bool ok) {
  if (!ok) {
    std::move(on_complete).Run(false);
    return;
  }
  if (!IsFactoryBound()) {
    DVLOG(0) << "LocalAIServiceBase: "
                "Factory gone before model files loaded";
    std::move(on_complete).Run(false);
    return;
  }
  InitModelViaFactory(std::move(on_complete));
}

void LocalAIServiceBase::OnLoadAndInitComplete(bool success) {
  if (success) {
    model_ready_ = true;
    ProcessPendingCallbacks();
  } else {
    DVLOG(0) << "LocalAIServiceBase: Model init failed";
    CloseBWC();
  }
}

void LocalAIServiceBase::ProcessPendingCallbacks() {
  auto consumers = std::move(pending_consumers_);
  for (auto& c : consumers) {
    std::move(c.on_ready).Run();
  }
}

void LocalAIServiceBase::CancelAllPending() {
  auto consumers = std::move(pending_consumers_);
  for (auto& c : consumers) {
    std::move(c.on_cancel).Run();
  }
}

}  // namespace local_ai
