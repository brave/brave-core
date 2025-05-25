// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"

#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/local_models_updater.h"

namespace ai_chat {

AssociatedContentDelegate::AssociatedContentDelegate()
    : uuid_(base::Uuid::GenerateRandomV4().AsLowercaseString()),
      text_embedder_(nullptr, base::OnTaskRunnerDeleter(nullptr)) {}

AssociatedContentDelegate::~AssociatedContentDelegate() = default;

void AssociatedContentDelegate::OnNewPage(int64_t navigation_id) {
  for (auto& observer : observers_) {
    observer.OnNavigated(this);
  }

  pending_top_similarity_requests_.clear();
  if (text_embedder_) {
    text_embedder_->CancelAllTasks();
    text_embedder_.reset();
  }
}

void AssociatedContentDelegate::GetStagedEntriesFromContent(
    GetStagedEntriesCallback callback) {
  std::move(callback).Run(std::nullopt);
}

bool AssociatedContentDelegate::HasOpenAIChatPermission() const {
  return false;
}

void AssociatedContentDelegate::GetScreenshots(
    mojom::ConversationHandler::GetScreenshotsCallback callback) {
  std::move(callback).Run(std::nullopt);
}

void AssociatedContentDelegate::GetTopSimilarityWithPromptTilContextLimit(
    const std::string& prompt,
    const std::string& text,
    uint32_t context_limit,
    TextEmbedder::TopSimilarityCallback callback) {
  // Create TextEmbedder
  if (!text_embedder_) {
    base::FilePath universal_qa_model_path =
        LocalModelsUpdaterState::GetInstance()->GetUniversalQAModel();
    // Tasks in TextEmbedder are run on |embedder_task_runner|. The
    // text_embedder_ must be deleted on that sequence to guarantee that pending
    // tasks can safely be executed.
    scoped_refptr<base::SequencedTaskRunner> embedder_task_runner =
        base::ThreadPool::CreateSequencedTaskRunner(
            {base::MayBlock(), base::TaskPriority::USER_BLOCKING});
    text_embedder_ = TextEmbedder::Create(
        base::FilePath(universal_qa_model_path), embedder_task_runner);
    if (!text_embedder_) {
      std::move(callback).Run(
          base::unexpected("Failed to create TextEmbedder"));
      pending_top_similarity_requests_.pop_back();
      return;
    }
  }

  if (!text_embedder_->IsInitialized()) {
    // Will have to wait for initialization to complete, store params for
    // calling later.
    pending_top_similarity_requests_.emplace_back(prompt, text, context_limit,
                                                  std::move(callback));

    text_embedder_->Initialize(
        base::BindOnce(&AssociatedContentDelegate::OnTextEmbedderInitialized,
                       weak_ptr_factory_.GetWeakPtr()));
  } else {
    // Run immediately if already initialized
    text_embedder_->GetTopSimilarityWithPromptTilContextLimit(
        prompt, text, context_limit, std::move(callback));
  }
}

void AssociatedContentDelegate::OnTextEmbedderInitialized(bool initialized) {
  if (!initialized) {
    VLOG(1) << "Failed to initialize TextEmbedder";
    for (auto& request_info : pending_top_similarity_requests_) {
      std::move(std::get<3>(request_info))
          .Run(base::unexpected<std::string>(
              "Failed to initialize TextEmbedder"));
    }
    pending_top_similarity_requests_.clear();
    return;
  }

  CHECK(text_embedder_);
  for (auto& request_info : pending_top_similarity_requests_) {
    text_embedder_->GetTopSimilarityWithPromptTilContextLimit(
        std::move(std::get<0>(request_info)),
        std::move(std::get<1>(request_info)), std::get<2>(request_info),
        std::move(std::get<3>(request_info)));
  }
  pending_top_similarity_requests_.clear();
}

void AssociatedContentDelegate::OnTitleChanged() {
  for (auto& observer : observers_) {
    observer.OnTitleChanged(this);
  }
}

void AssociatedContentDelegate::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AssociatedContentDelegate::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace ai_chat
