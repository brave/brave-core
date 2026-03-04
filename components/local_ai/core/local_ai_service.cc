// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/core/local_ai_service.h"

#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "brave/components/local_ai/core/features.h"

namespace local_ai {

LocalAIService::LocalAIService(BackgroundWebContentsFactory factory)
    : background_web_contents_factory_(std::move(factory)) {
  CHECK(base::FeatureList::IsEnabled(features::kLocalAIModels));
  DVLOG(3) << "LocalAIService created";
}

LocalAIService::~LocalAIService() {
  CloseBackgroundContents();
}

mojo::PendingRemote<mojom::LocalAIService> LocalAIService::MakeRemote() {
  mojo::PendingRemote<mojom::LocalAIService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void LocalAIService::Bind(
    mojo::PendingReceiver<mojom::LocalAIService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void LocalAIService::RegisterPassageEmbedderFactory(
    mojo::PendingRemote<mojom::PassageEmbedderFactory> factory) {
  if (!background_web_contents_) {
    DVLOG(1) << "RegisterPassageEmbedderFactory: No background contents";
    return;
  }
  factory_.Bind(std::move(factory));
  factory_.set_disconnect_handler(base::BindOnce(
      &LocalAIService::OnFactoryDisconnected, weak_ptr_factory_.GetWeakPtr()));
  ProcessPendingCallbacks();
}

void LocalAIService::GetPassageEmbedder(GetPassageEmbedderCallback callback) {
  MaybeCreateBackgroundContents();
  if (factory_.is_bound()) {
    BindPassageEmbedder(std::move(callback));
  } else {
    pending_embedder_callbacks_.push_back(std::move(callback));
  }
}

void LocalAIService::NotifyPassageEmbedderIdle() {
  DVLOG(3) << "LocalAIService: PassageEmbedder idle";
  CloseBackgroundContents();
}

void LocalAIService::OnBackgroundContentsDestroyed(
    BackgroundWebContents::DestroyReason reason) {
  DVLOG(1) << "LocalAIService: Background contents destroyed";
  CloseBackgroundContents();
}

void LocalAIService::Shutdown() {
  DVLOG(3) << "LocalAIService: Shutting down";
  receivers_.Clear();
  weak_ptr_factory_.InvalidateWeakPtrs();
  CloseBackgroundContents();
}

void LocalAIService::MaybeCreateBackgroundContents() {
  if (background_web_contents_) {
    return;
  }

  DVLOG(3) << "LocalAIService: Creating background contents";
  background_web_contents_ = background_web_contents_factory_.Run(this);
}

void LocalAIService::CloseBackgroundContents() {
  DVLOG(3) << "LocalAIService: Closing background contents to free memory";
  factory_.reset();
  CancelPendingCallbacks();
  background_web_contents_.reset();
}

void LocalAIService::BindPassageEmbedder(GetPassageEmbedderCallback callback) {
  mojo::PendingRemote<mojom::PassageEmbedder> remote;
  factory_->Bind(remote.InitWithNewPipeAndPassReceiver());
  std::move(callback).Run(std::move(remote));
}

void LocalAIService::ProcessPendingCallbacks() {
  auto callbacks = std::move(pending_embedder_callbacks_);
  for (auto& cb : callbacks) {
    BindPassageEmbedder(std::move(cb));
  }
}

void LocalAIService::OnFactoryDisconnected() {
  factory_.reset();
  CancelPendingCallbacks();
}

void LocalAIService::CancelPendingCallbacks() {
  auto callbacks = std::move(pending_embedder_callbacks_);
  for (auto& cb : callbacks) {
    std::move(cb).Run(mojo::PendingRemote<mojom::PassageEmbedder>());
  }
}

}  // namespace local_ai
