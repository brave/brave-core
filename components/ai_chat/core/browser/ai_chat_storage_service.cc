/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_storage_service.h"

#include <utility>
#include <vector>

#include "base/task/thread_pool.h"
#include "content/public/browser/browser_context.h"

namespace ai_chat {
namespace {
constexpr base::FilePath::StringPieceType kBaseDirName =
    FILE_PATH_LITERAL("AIChat");
}  // namespace

AIChatStorageService::AIChatStorageService(content::BrowserContext* context)
    : base_dir_(context->GetPath().Append(kBaseDirName)) {
  ai_chat_db_ = base::SequenceBound<AIChatDatabase>(GetTaskRunner());

  auto on_response = base::BindOnce(
      [](bool success) { DVLOG(1) << "AIChatDB Init: " << success; });

  ai_chat_db_.AsyncCall(&AIChatDatabase::Init)
      .WithArgs(base_dir_)
      .Then(std::move(on_response));
}

AIChatStorageService::~AIChatStorageService() = default;

base::SequencedTaskRunner* AIChatStorageService::GetTaskRunner() {
  if (!task_runner_) {
    task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::WithBaseSyncPrimitives(),
         base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  }

  return task_runner_.get();
}

void AIChatStorageService::SyncConversation(mojom::ConversationPtr conversation,
                                          ConversationCallback callback) {
  auto on_added = [](mojom::ConversationPtr conversation,
                     ConversationCallback callback, int64_t id) {
    conversation->id = id;
    std::move(callback).Run(std::move(conversation));
  };

  ai_chat_db_.AsyncCall(&AIChatDatabase::AddConversation)
      .WithArgs(conversation.Clone())
      .Then(base::BindOnce(on_added, std::move(conversation),
                           std::move(callback)));
}

void AIChatStorageService::SyncConversationTurn(int64_t conversation_id,
                                              mojom::ConversationTurnPtr turn) {
  NOTIMPLEMENTED();
}

void AIChatStorageService::GetConversationForGURL(const GURL& gurl,
                                                ConversationCallback callback) {
  auto on_get = [](const GURL& gurl, ConversationCallback callback,
                   std::vector<mojom::ConversationPtr> conversations) {
    auto it = std::find_if(conversations.begin(), conversations.end(),
                           [&gurl](const auto& conversation) {
                             return conversation->page_url == gurl.spec();
                           });
    if (it != conversations.end()) {
      std::move(callback).Run(std::move(*it));
    } else {
      std::move(callback).Run(std::nullopt);
    }
  };

  ai_chat_db_.AsyncCall(&AIChatDatabase::GetAllConversations)
      .Then(base::BindOnce(on_get, gurl, std::move(callback)));
}

void AIChatStorageService::Shutdown() {
  task_runner_.reset();
}

}  // namespace ai_chat
