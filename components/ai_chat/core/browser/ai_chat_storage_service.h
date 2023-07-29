/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_STORAGE_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_STORAGE_SERVICE_H_

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace ai_chat {
using ConversationCallback = base::OnceCallback<void(
    std::optional<mojom::ConversationPtr> conversation)>;
class AIChatStorageService : public KeyedService {
 public:
  explicit AIChatStorageService(content::BrowserContext* context);
  ~AIChatStorageService() override;
  AIChatStorageService(const AIChatStorageService&) = delete;
  AIChatStorageService& operator=(const AIChatStorageService&) = delete;

  void SyncConversation(mojom::ConversationPtr conversation,
                        ConversationCallback callback);
  void SyncConversationTurn(int64_t conversation_id,
                            mojom::ConversationTurnPtr turn);
  void GetConversationForGURL(const GURL& gurl, ConversationCallback callback);

 private:
  base::SequencedTaskRunner* GetTaskRunner();

  // KeyedService overrides:
  void Shutdown() override;

  const base::FilePath base_dir_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  base::SequenceBound<AIChatDatabase> ai_chat_db_;

  base::WeakPtrFactory<AIChatStorageService> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_STORAGE_SERVICE_H_
