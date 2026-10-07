// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/memory_manager_delegate_impl.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/notreached.h"
#include "base/numerics/safe_conversions.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ai_chat/ai_chat_ui_semantic_search.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/dreaming_run.h"
#include "brave/components/ai_chat/core/browser/learned_memory_types.h"
#include "brave/components/ai_chat/core/browser/user_memory_manager.h"
#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom.h"

namespace ai_chat {

namespace {

mojom::LearnedMemoryType ToMojom(LearnedMemoryType type) {
  switch (type) {
    case LearnedMemoryType::kPermanent:
      return mojom::LearnedMemoryType::kPermanent;
    case LearnedMemoryType::kLongTerm:
      return mojom::LearnedMemoryType::kLongTerm;
    case LearnedMemoryType::kShortTerm:
      return mojom::LearnedMemoryType::kShortTerm;
  }
  NOTREACHED();
}

mojom::LearnedMemoryCategory ToMojom(LearnedMemoryCategory category) {
  switch (category) {
    case LearnedMemoryCategory::kPreference:
      return mojom::LearnedMemoryCategory::kPreference;
    case LearnedMemoryCategory::kPersonalFact:
      return mojom::LearnedMemoryCategory::kPersonalFact;
    case LearnedMemoryCategory::kTopic:
      return mojom::LearnedMemoryCategory::kTopic;
  }
  NOTREACHED();
}

mojom::DreamNowStatus ToMojom(DreamingStatus status) {
  switch (status) {
    case DreamingStatus::kCompleted:
      return mojom::DreamNowStatus::kCompleted;
    case DreamingStatus::kTimedOut:
      return mojom::DreamNowStatus::kTimedOut;
    case DreamingStatus::kFailed:
      return mojom::DreamNowStatus::kFailed;
    case DreamingStatus::kCanceled:
      return mojom::DreamNowStatus::kCanceled;
    case DreamingStatus::kBusy:
      return mojom::DreamNowStatus::kBusy;
    case DreamingStatus::kUnavailable:
      return mojom::DreamNowStatus::kUnavailable;
  }
  NOTREACHED();
}

void OnGotLearnedMemories(
    MemoryManagerDelegate::GetLearnedMemoriesCallback callback,
    std::vector<LearnedMemory> memories) {
  std::vector<mojom::LearnedMemoryItemPtr> items;
  items.reserve(memories.size());
  for (const auto& memory : memories) {
    std::optional<std::string> previous_text;
    if (memory.previous) {
      previous_text = memory.previous->text;
    }
    items.push_back(mojom::LearnedMemoryItem::New(
        memory.uuid, memory.text, ToMojom(memory.category),
        ToMojom(memory.type), memory.created_date, memory.updated_date,
        std::move(previous_text)));
  }
  // The newest memory is the first in the list.
  std::ranges::reverse(items);
  std::move(callback).Run(true, std::move(items));
}

void OnDreamNowDone(MemoryManagerDelegate::DreamNowCallback callback,
                    DreamingResult result) {
  std::move(callback).Run(mojom::DreamNowResult::New(
      ToMojom(result.status), base::saturated_cast<uint32_t>(result.turns_read),
      base::saturated_cast<uint32_t>(result.turns_kept),
      base::saturated_cast<uint32_t>(result.memories_added),
      base::saturated_cast<uint32_t>(result.memories_updated)));
}

}  // namespace

MemoryManagerDelegateImpl::MemoryManagerDelegateImpl(
    content::BrowserContext* context)
    : context_(context) {}

MemoryManagerDelegateImpl::~MemoryManagerDelegateImpl() = default;

void MemoryManagerDelegateImpl::SearchMemories(
    const std::string& query,
    SearchMemoriesCallback callback) {
  SearchMemoriesForUI(context_, query, std::move(callback));
}

void MemoryManagerDelegateImpl::GetLearnedMemories(
    GetLearnedMemoriesCallback callback) {
  AIChatService* service = AIChatServiceFactory::GetForBrowserContext(context_);
  UserMemoryManager* manager =
      service ? service->GetUserMemoryManager() : nullptr;
  if (!manager || !manager->is_storage_ready()) {
    std::move(callback).Run(false, {});
    return;
  }
  manager->GetLearnedMemories(
      base::BindOnce(&OnGotLearnedMemories, std::move(callback)));
}

void MemoryManagerDelegateImpl::DeleteLearnedMemory(
    const std::string& uuid,
    DeleteLearnedMemoryCallback callback) {
  AIChatService* service = AIChatServiceFactory::GetForBrowserContext(context_);
  UserMemoryManager* manager =
      service ? service->GetUserMemoryManager() : nullptr;
  if (!manager) {
    std::move(callback).Run(false);
    return;
  }
  manager->DeleteLearnedMemory(uuid, std::move(callback));
}

void MemoryManagerDelegateImpl::DeleteAllLearnedMemories(
    DeleteAllLearnedMemoriesCallback callback) {
  AIChatService* service = AIChatServiceFactory::GetForBrowserContext(context_);
  UserMemoryManager* manager =
      service ? service->GetUserMemoryManager() : nullptr;
  if (!manager) {
    std::move(callback).Run(false);
    return;
  }
  manager->DeleteAllLearnedMemories(std::move(callback));
}

void MemoryManagerDelegateImpl::DreamNow(DreamNowCallback callback) {
  AIChatService* service = AIChatServiceFactory::GetForBrowserContext(context_);
  UserMemoryManager* manager =
      service ? service->GetUserMemoryManager() : nullptr;
  if (!manager) {
    std::move(callback).Run(mojom::DreamNowResult::New(
        mojom::DreamNowStatus::kUnavailable, 0, 0, 0, 0));
    return;
  }
  manager->DreamNow(base::BindOnce(&OnDreamNowDone, std::move(callback)));
}

}  // namespace ai_chat
