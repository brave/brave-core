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

LocalAIService::PendingEmbedRequest::PendingEmbedRequest() = default;
LocalAIService::PendingEmbedRequest::PendingEmbedRequest(std::string text,
                                                         EmbedCallback callback)
    : text(std::move(text)), callback(std::move(callback)) {}
LocalAIService::PendingEmbedRequest::~PendingEmbedRequest() = default;
LocalAIService::PendingEmbedRequest::PendingEmbedRequest(
    PendingEmbedRequest&&) = default;
LocalAIService::PendingEmbedRequest&
LocalAIService::PendingEmbedRequest::operator=(PendingEmbedRequest&&) = default;

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

void LocalAIService::BindEmbeddingGemma(
    mojo::PendingRemote<mojom::EmbeddingGemmaInterface> pending_remote) {
  if (embedding_gemma_remote_.is_bound()) {
    DVLOG(1) << "EmbeddingGemma already bound, resetting";
    embedding_gemma_remote_.reset();
  }
  embedding_gemma_remote_.Bind(std::move(pending_remote));

  embedding_gemma_remote_.set_disconnect_handler(base::BindOnce(
      [](LocalAIService* service) {
        DVLOG(1) << "EmbeddingGemma remote disconnected";
        service->CloseBackgroundContents();
      },
      base::Unretained(this)));

  DVLOG(3) << "BindEmbeddingGemma: Bound embedder remote";

  ProcessPendingEmbedRequests();
}

void LocalAIService::Embed(const std::string& text, EmbedCallback callback) {
  MaybeCreateBackgroundContents();

  if (!embedding_gemma_remote_.is_bound()) {
    DVLOG(3) << "Embedding not ready yet, queuing embed request";
    pending_embed_requests_.emplace_back(text, std::move(callback));
    return;
  }

  embedding_gemma_remote_->Embed(text, std::move(callback));
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

void LocalAIService::ProcessPendingEmbedRequests() {
  if (!embedding_gemma_remote_.is_bound()) {
    return;
  }

  DVLOG(3) << "Processing " << pending_embed_requests_.size()
           << " pending embed requests";

  std::vector<PendingEmbedRequest> requests;
  requests.swap(pending_embed_requests_);
  for (auto& request : requests) {
    embedding_gemma_remote_->Embed(request.text, std::move(request.callback));
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
  std::vector<PendingEmbedRequest> requests;
  requests.swap(pending_embed_requests_);
  for (auto& request : requests) {
    std::move(request.callback).Run({});
  }
}

void LocalAIService::CloseBackgroundContents() {
  DVLOG(3) << "LocalAIService: Closing background contents to free memory";

  embedding_gemma_remote_.reset();
  CancelPendingRequests();
  background_web_contents_.reset();
}

}  // namespace local_ai
