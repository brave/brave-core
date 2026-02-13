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

LocalAIService::PendingRequest::PendingRequest() = default;
LocalAIService::PendingRequest::PendingRequest(
    std::string text,
    GenerateEmbeddingsCallback callback)
    : text(std::move(text)), callback(std::move(callback)) {}
LocalAIService::PendingRequest::~PendingRequest() = default;
LocalAIService::PendingRequest::PendingRequest(PendingRequest&&) = default;
LocalAIService::PendingRequest& LocalAIService::PendingRequest::operator=(
    PendingRequest&&) = default;

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

void LocalAIService::RegisterOnDeviceModelWorker(
    mojo::PendingRemote<mojom::OnDeviceModelWorker> worker) {
  if (model_worker_remote_.is_bound()) {
    DVLOG(1) << "Model worker already bound, resetting";
    model_worker_remote_.reset();
  }
  model_worker_remote_.Bind(std::move(worker));

  model_worker_remote_.set_disconnect_handler(base::BindOnce(
      [](LocalAIService* service) {
        DVLOG(1) << "Model worker remote disconnected";
        service->CloseBackgroundContents();
      },
      base::Unretained(this)));

  DVLOG(3) << "RegisterOnDeviceModelWorker: Bound model worker";

  ProcessPendingRequests();
}

void LocalAIService::GenerateEmbeddings(
    const std::string& text,
    GenerateEmbeddingsCallback callback) {
  MaybeCreateBackgroundContents();

  if (!model_worker_remote_.is_bound()) {
    DVLOG(3) << "Model worker not ready yet, queuing request";
    pending_requests_.emplace_back(text, std::move(callback));
    return;
  }

  model_worker_remote_->GenerateEmbeddings(text, std::move(callback));
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

void LocalAIService::ProcessPendingRequests() {
  if (!model_worker_remote_.is_bound()) {
    return;
  }

  DVLOG(3) << "Processing " << pending_requests_.size()
           << " pending requests";

  std::vector<PendingRequest> requests;
  requests.swap(pending_requests_);
  for (auto& request : requests) {
    model_worker_remote_->GenerateEmbeddings(request.text,
                                             std::move(request.callback));
  }
}

void LocalAIService::MaybeCreateBackgroundContents() {
  if (background_web_contents_) {
    return;
  }

  DVLOG(3) << "LocalAIService: Creating background contents";
  background_web_contents_ = background_web_contents_factory_.Run(this);
}

void LocalAIService::CancelPendingRequests() {
  std::vector<PendingRequest> requests;
  requests.swap(pending_requests_);
  for (auto& request : requests) {
    std::move(request.callback).Run({});
  }
}

void LocalAIService::CloseBackgroundContents() {
  DVLOG(3) << "LocalAIService: Closing background contents to free memory";

  model_worker_remote_.reset();
  CancelPendingRequests();
  background_web_contents_.reset();
}

}  // namespace local_ai
