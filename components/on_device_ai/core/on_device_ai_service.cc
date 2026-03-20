// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/on_device_ai/core/on_device_ai_service.h"

#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "brave/components/on_device_ai/core/features.h"

namespace on_device_ai {

OnDeviceAIService::OnDeviceAIService(BackgroundWebContentsFactory factory)
    : background_web_contents_factory_(std::move(factory)) {
  CHECK(base::FeatureList::IsEnabled(features::kOnDeviceAIModels));
  DVLOG(3) << "OnDeviceAIService created";
}

OnDeviceAIService::~OnDeviceAIService() {
  CloseBackgroundContents();
}

mojo::PendingRemote<mojom::OnDeviceAIService> OnDeviceAIService::MakeRemote() {
  mojo::PendingRemote<mojom::OnDeviceAIService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void OnDeviceAIService::Bind(
    mojo::PendingReceiver<mojom::OnDeviceAIService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void OnDeviceAIService::RegisterPassageEmbedderFactory(
    mojo::PendingRemote<mojom::PassageEmbedderFactory> factory) {
  if (!background_web_contents_) {
    DVLOG(1) << "RegisterPassageEmbedderFactory: No background contents";
    return;
  }
  factory_.Bind(std::move(factory));
  factory_.set_disconnect_handler(
      base::BindOnce(&OnDeviceAIService::OnFactoryDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  ProcessPendingCallbacks();
}

void OnDeviceAIService::GetPassageEmbedder(
    GetPassageEmbedderCallback callback) {
  MaybeCreateBackgroundContents();
  if (factory_.is_bound()) {
    BindPassageEmbedder(std::move(callback));
  } else {
    pending_embedder_callbacks_.push_back(std::move(callback));
  }
}

void OnDeviceAIService::OnBackgroundContentsDestroyed(
    BackgroundWebContents::DestroyReason reason) {
  DVLOG(1) << "OnDeviceAIService: Background contents destroyed";
  CloseBackgroundContents();
}

void OnDeviceAIService::Shutdown() {
  DVLOG(3) << "OnDeviceAIService: Shutting down";
  receivers_.Clear();
  weak_ptr_factory_.InvalidateWeakPtrs();
  CloseBackgroundContents();
}

void OnDeviceAIService::MaybeCreateBackgroundContents() {
  if (background_web_contents_) {
    return;
  }

  DVLOG(3) << "OnDeviceAIService: Creating background contents";
  background_web_contents_ = background_web_contents_factory_.Run(this);
}

void OnDeviceAIService::CloseBackgroundContents() {
  DVLOG(3) << "OnDeviceAIService: Closing background contents to free memory";
  factory_.reset();
  CancelPendingCallbacks();
  background_web_contents_.reset();
}

void OnDeviceAIService::BindPassageEmbedder(
    GetPassageEmbedderCallback callback) {
  mojo::PendingRemote<mojom::PassageEmbedder> remote;
  factory_->Bind(remote.InitWithNewPipeAndPassReceiver());
  std::move(callback).Run(std::move(remote));
}

void OnDeviceAIService::ProcessPendingCallbacks() {
  auto callbacks = std::move(pending_embedder_callbacks_);
  for (auto& cb : callbacks) {
    BindPassageEmbedder(std::move(cb));
  }
}

void OnDeviceAIService::OnFactoryDisconnected() {
  factory_.reset();
  CancelPendingCallbacks();
}

void OnDeviceAIService::CancelPendingCallbacks() {
  auto callbacks = std::move(pending_embedder_callbacks_);
  for (auto& cb : callbacks) {
    std::move(cb).Run(mojo::PendingRemote<mojom::PassageEmbedder>());
  }
}

}  // namespace on_device_ai
