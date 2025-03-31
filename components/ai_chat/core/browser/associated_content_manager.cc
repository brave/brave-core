// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_content_manager.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/barrier_callback.h"
#include "base/observer_list.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/associated_archive_content.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace ai_chat {

AssociatedContentManager::AssociatedContentManager(
    ConversationHandler* conversation)
    : conversation_(conversation) {}

AssociatedContentManager::~AssociatedContentManager() {
  DetachContent();
}

void AssociatedContentManager::SetContent(
    ConversationHandler::AssociatedContentDelegate* delegate) {
  DetachContent();

  if (delegate) {
    AddContent(delegate, /*notify_updated=*/false);
  }

  // Default to send page contents when we have a valid contents.
  // This class should only be provided with a delegate when
  // it is allowed to use it (e.g. not internal WebUI content).
  // The user can toggle this via the UI.
  should_send_ = features::IsPageContextEnabledInitially();
  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::LoadArchivedContent(
    const mojom::Conversation* metadata,
    const mojom::ConversationArchivePtr& archive) {
  // Remove all archived content - its been reloaded from the DB.
  for (int i = archive_content_.size() - 1; i >= 0; --i) {
    RemoveContent(archive_content_[i].get(), /*notify_updated=*/false);
  }

  CHECK_EQ(metadata->associated_content.size(),
           archive->associated_content.size());

  for (size_t i = 0; i < archive->associated_content.size(); ++i) {
    const auto& archive_content = archive->associated_content[i];
    const auto& content_it = std::ranges::find_if(
        metadata->associated_content, [&archive_content](const auto& content) {
          return archive_content->content_uuid == content->uuid;
        });
    if (content_it == metadata->associated_content.end()) {
      continue;
    }

    auto* content = content_it->get();
    bool is_video =
        (content->content_type == mojom::ContentType::VideoTranscript);
    archive_content_.push_back(std::make_unique<AssociatedArchiveContent>(
        content->url, archive_content->content,
        base::UTF8ToUTF16(content->title), is_video, content->uuid));
    AddContent(archive_content_.back().get(), /*notify_updated=*/false);
  }

  // If we restored content from an archive then it was used in the conversation
  // so we should send it.
  should_send_ = !archive->associated_content.empty();
  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::SetArchiveContent(int content_id,
                                                 std::string text_content,
                                                 bool is_video) {
  auto it =
      std::ranges::find(content_drivers_, content_id,
                        [](const auto& ptr) { return ptr->GetContentId(); });
  CHECK(it != content_drivers_.end()) << "Couldn't find |content_id|";

  auto* delegate = *it;
  content_observations_.RemoveObservation(delegate);

  // Construct a "content archive" implementation of AssociatedContentDelegate
  // with a duplicate of the article text.
  archive_content_.emplace_back(std::make_unique<AssociatedArchiveContent>(
      delegate->GetURL(), std::move(text_content), delegate->GetTitle(),
      is_video, delegate->uuid()));
  *it = archive_content_.back().get();

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::AddContent(
    ConversationHandler::AssociatedContentDelegate* delegate,
    bool notify_updated) {
  CHECK_EQ(conversation_->GetConversationHistorySize(), 0u)
      << "Cannot add content to an associated content manager with a "
         "conversation history.";

  // If we're adding content, we probably want to send it.
  should_send_ = true;

  content_drivers_.push_back(delegate);
  content_observations_.AddObservation(delegate);

  if (notify_updated) {
    conversation_->OnAssociatedContentUpdated();
  }
}

void AssociatedContentManager::RemoveContent(
    ConversationHandler::AssociatedContentDelegate* delegate,
    bool notify_updated) {
  CHECK_EQ(conversation_->GetConversationHistorySize(), 0u)
      << "Cannot remove content from an associated content manager with a "
         "conversation history.";

  auto it = std::ranges::find(content_drivers_, delegate,
                              [](const auto& ptr) { return ptr; });
  if (it != content_drivers_.end()) {
    // Let the content know it isn't associated with this conversation
    // anymore.
    content_observations_.RemoveObservation(delegate);
    content_drivers_.erase(it);
  }

  // If this is archived content, delete it.
  auto archive_it = std::ranges::find_if(
      archive_content_,
      [delegate](const auto& content) { return content.get() == delegate; });
  if (archive_it != archive_content_.end()) {
    archive_content_.erase(archive_it);
  }

  // If we're modifying the associated content, we probably want to send it.
  should_send_ = !content_drivers_.empty();

  if (notify_updated) {
    conversation_->OnAssociatedContentUpdated();
  }
}

std::vector<mojom::AssociatedContentPtr>
AssociatedContentManager::GetAssociatedContent() const {
  // For the <page></page> wrapper around the content.
  constexpr uint32_t kAdditionalCharsPerContent = 15;
  const uint32_t max_associated_content_length =
      ModelService::CalcuateMaxAssociatedContentLengthForModel(
          conversation_->GetCurrentModel());
  uint32_t total_consumed_chars = 0;

  std::vector<mojom::AssociatedContentPtr> result;
  for (auto* driver : content_drivers_) {
    // TODO(fallaciousreasoning): Work out how we get here and stop that
    // instead!
    if (!driver) {
      continue;
    }

    mojom::AssociatedContentPtr content = mojom::AssociatedContent::New();
    content->uuid = driver->uuid();
    content->content_id = driver->GetContentId();
    content->url = driver->GetURL();
    content->title = base::UTF16ToUTF8(driver->GetTitle());

    const uint32_t content_length =
        driver->GetCachedTextContent().length() + kAdditionalCharsPerContent;
    if (total_consumed_chars + content_length <=
        max_associated_content_length) {
      content->content_used_percentage = 100;
    } else if (total_consumed_chars >= max_associated_content_length) {
      content->content_used_percentage = 0;
    } else {
      // Convert to float to avoid integer division, which truncates towards
      // zero
      // and could lead to inaccurate results before multiplication.
      float pct = static_cast<float>(max_associated_content_length -
                                     total_consumed_chars) /
                  static_cast<float>(content_length) * 100;
      content->content_used_percentage = base::ClampRound(pct);
    }

    result.push_back(std::move(content));
    total_consumed_chars += content_length;
  }
  return result;
}

void AssociatedContentManager::GetContent(
    ConversationHandler::GetPageContentCallback callback) {
  if (!on_page_text_fetch_complete_) {
    on_page_text_fetch_complete_ = std::make_unique<base::OneShotEvent>();
    // wait for all GetPageContent to finish
    auto content_callback = base::BarrierCallback<std::string>(
        content_drivers_.size(),
        base::BindOnce(
            [](base::WeakPtr<AssociatedContentManager> self,
               std::vector<std::string> results) {
              if (!self) {
                return;
              }
              self->cached_text_content_ = base::StrCat(
                  {"<page>", base::JoinString(results, "</page><page>"),
                   "</page>"});
              self->on_page_text_fetch_complete_->Signal();
              self->on_page_text_fetch_complete_ = nullptr;
            },
            weak_ptr_factory_.GetWeakPtr()));
    for (const auto& content : content_drivers_) {
      if (!content) {
        continue;
      }
      content->GetContent(base::BindOnce(
          [](base::RepeatingCallback<void(std::string)> callback,
             std::string content, bool is_video,
             std::string invalidation_token) { callback.Run(content); },
          content_callback));
    }
  }
  on_page_text_fetch_complete_->Post(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<AssociatedContentManager> self,
                        ConversationHandler::GetPageContentCallback callback) {
                       if (!self) {
                         return;
                       }
                       self->conversation_->OnAssociatedContentUpdated();
                       std::move(callback).Run(self->cached_text_content_,
                                               false, "");
                     },
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AssociatedContentManager::GetStagedEntriesFromContent(
    ConversationHandler::GetStagedEntriesCallback callback) {
  if (content_drivers_.size() != 1) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  content_drivers_[0]->GetStagedEntriesFromContent(std::move(callback));
}

void AssociatedContentManager::GetTopSimilarityWithPromptTilContextLimit(
    const std::string& prompt,
    const std::string& text,
    uint32_t context_limit,
    TextEmbedder::TopSimilarityCallback callback) {
  if (content_drivers_.size() != 1) {
    // TODO: Not this.
    std::move(callback).Run(base::unexpected("No content driver"));
    return;
  }

  // TODO: I think this should work on our cached content from all pages.
  content_drivers_[0]->GetTopSimilarityWithPromptTilContextLimit(
      prompt, text, context_limit, std::move(callback));
}

std::string_view AssociatedContentManager::GetCachedTextContent() {
  return cached_text_content_;
}

bool AssociatedContentManager::HasOpenAIChatPermission() const {
  return content_drivers_.size() == 1 &&
         content_drivers_[0]->HasOpenAIChatPermission();
}

bool AssociatedContentManager::HasArchiveContent() const {
  return !archive_content_.empty();
}

bool AssociatedContentManager::HasContent() const {
  return !content_drivers_.empty();
}

bool AssociatedContentManager::IsVideo() const {
  return content_drivers_.size() == 1 &&
         content_drivers_[0]->GetCachedIsVideo();
}

void AssociatedContentManager::SetShouldSend(bool value) {
  should_send_ = value && HasContent();
  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::OnRequestArchive(
    ConversationHandler::AssociatedContentDelegate* delegate) {
  std::string text(delegate->GetCachedTextContent());
  SetArchiveContent(delegate->GetContentId(), std::move(text),
                    delegate->GetCachedIsVideo());

  // Note: We don't call conversation_->OnAssociatedContentUpdated() here
  // because the content should not have changed.
}

void AssociatedContentManager::OnTitleChanged(
    ConversationHandler::AssociatedContentDelegate* delegate) {
  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::OnContentChanged(
    ConversationHandler::AssociatedContentDelegate* delegate) {
  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::DetachContent() {
  content_observations_.RemoveAllObservations();
  content_drivers_.clear();
  archive_content_.clear();
}

}  // namespace ai_chat
