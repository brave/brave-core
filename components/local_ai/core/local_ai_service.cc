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

void LocalAIService::RegisterPassageEmbedder(
    mojo::PendingRemote<mojom::PassageEmbedder> embedder) {
  if (!background_web_contents_) {
    DVLOG(1) << "RegisterPassageEmbedder: No background contents";
    return;
  }
  background_web_contents_->SetWorkerRemote(std::move(embedder));
}

void LocalAIService::GetPassageEmbedder(GetPassageEmbedderCallback callback) {
  MaybeCreateBackgroundContents();
  std::move(callback).Run(background_web_contents_->BindNewPassageEmbedder());
}

void LocalAIService::OnBackgroundContentsReady() {
  DVLOG(3) << "LocalAIService: Background contents ready";
}

void LocalAIService::OnBackgroundContentsDestroyed(
    BackgroundWebContents::DestroyReason reason) {
  DVLOG(1) << "LocalAIService: Background contents destroyed";
  CloseBackgroundContents();
}

void LocalAIService::Shutdown() {
  DVLOG(3) << "LocalAIService: Shutting down";
  receivers_.Clear();
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
  background_web_contents_.reset();
}

}  // namespace local_ai
