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
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
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
  DVLOG(1) << __func__;

  DetachContent();
}

void AssociatedContentManager::SetContent(
    ConversationHandler::AssociatedContentDelegate* delegate) {
  DVLOG(1) << __func__;

  DetachContent();

  if (delegate) {
    AddContent(delegate, /*notify_updated=*/false);
  }

  // Default to send page contents when we have a valid contents.
  // This class should only be provided with a delegate when
  // it is allowed to use it (e.g. not internal WebUI content).
  // The user can toggle this via the UI.
  should_send_ = features::IsPageContextEnabledInitially() && HasContent();
  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::LoadArchivedContent(
    const mojom::Conversation* metadata,
    const mojom::ConversationArchivePtr& archive) {
  DVLOG(2) << __func__
           << " metadata size: " << metadata->associated_content.size()
           << ", archive size: " << archive->associated_content.size();

  // Remove all archived content - its been reloaded from the DB.
  for (int i = archive_content_.size() - 1; i >= 0; --i) {
    RemoveContent(archive_content_[i].get(), /*notify_updated=*/false);
  }

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
  should_send_ = should_send_ || !archive->associated_content.empty();
  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::SetArchiveContent(int content_id,
                                                 std::string text_content,
                                                 bool is_video) {
  DVLOG(1) << __func__;

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
  content_observations_.AddObservation(*it);

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::AddContent(
    ConversationHandler::AssociatedContentDelegate* delegate,
    bool notify_updated) {
  DVLOG(1) << __func__;

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
  DVLOG(1) << __func__;

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
  DVLOG(1) << __func__;

  // For the <page></page> wrapper around the content.
  constexpr uint32_t kAdditionalCharsPerContent = 15;
  const uint32_t max_associated_content_length =
      ModelService::CalcuateMaxAssociatedContentLengthForModel(
          conversation_->GetCurrentModel());
  uint32_t total_consumed_chars = 0;

  std::vector<mojom::AssociatedContentPtr> result;
  for (auto* driver : content_drivers_) {
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
  DVLOG(1) << __func__;

  // Note: |GetContent| on the AssociatedContentDelegate's is sometimes sync,
  // sometimes async depending on whether it has already been run. This means we
  // need to make sure we don't destroy the signal before we post this callback.
  auto cb = base::BindOnce(
      [](base::WeakPtr<AssociatedContentManager> self,
         ConversationHandler::GetPageContentCallback callback) {
        if (!self) {
          return;
        }
        self->conversation_->OnAssociatedContentUpdated();
        std::move(callback).Run(self->cached_text_content_, false, "");
      },
      weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  if (!on_page_text_fetch_complete_) {
    on_page_text_fetch_complete_ = std::make_unique<base::OneShotEvent>();

    // Note: When we create the signal here its important we post the callback
    // before running the BarrierCallback, which will null out the signal when
    // it completes (if all the |GetContent| calls are synchronous).
    on_page_text_fetch_complete_->Post(FROM_HERE, std::move(cb));

    // wait for all GetPageContent to finish
    auto content_callback = base::BarrierCallback<std::string>(
        content_drivers_.size(),
        base::BindOnce(
            [](base::WeakPtr<AssociatedContentManager> self,
               std::vector<std::string> results) {
              if (!self) {
                return;
              }

              // If we only have one content driver directly return the content.
              // Otherwise, wrap each content in <page> tags.
              if (results.size() == 1) {
                self->cached_text_content_ = results[0];
              } else if (results.size() > 0) {
                self->cached_text_content_ = base::StrCat(
                    {"<page>", base::JoinString(results, "</page><page>"),
                     "</page>"});
              } else {
                self->cached_text_content_ = "";
              }
              self->on_page_text_fetch_complete_->Signal();
              self->on_page_text_fetch_complete_ = nullptr;
            },
            weak_ptr_factory_.GetWeakPtr()));
    for (auto* content : content_drivers_) {
      content->GetContent(base::BindOnce(
          [](base::RepeatingCallback<void(std::string)> callback,
             std::string content, bool is_video,
             std::string invalidation_token) { callback.Run(content); },
          content_callback));
    }
  } else {
    on_page_text_fetch_complete_->Post(FROM_HERE, std::move(cb));
  }
}

void AssociatedContentManager::GetStagedEntriesFromContent(
    ConversationHandler::GetStagedEntriesCallback callback) {
  DVLOG(1) << __func__;

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
  DVLOG(1) << __func__;

  if (content_drivers_.size() != 1) {
    // TODO(@fallaciousreasoning): Ask @petemill if this is correct.
    std::move(callback).Run(base::unexpected("No content driver"));
    return;
  }

  // TODO(@fallaciousreasoning): Ask @petemill if this is correct.
  content_drivers_[0]->GetTopSimilarityWithPromptTilContextLimit(
      prompt, text, context_limit, std::move(callback));
}

std::string_view AssociatedContentManager::GetCachedTextContent() {
  DVLOG(1) << __func__;

  return cached_text_content_;
}

bool AssociatedContentManager::HasOpenAIChatPermission() const {
  DVLOG(1) << __func__;

  return content_drivers_.size() == 1 &&
         content_drivers_[0]->HasOpenAIChatPermission();
}

bool AssociatedContentManager::HasNonArchiveContent() const {
  DVLOG(1) << __func__;

  return archive_content_.size() < content_drivers_.size();
}

bool AssociatedContentManager::HasContent() const {
  DVLOG(1) << __func__;

  return !content_drivers_.empty();
}

bool AssociatedContentManager::IsVideo() const {
  DVLOG(1) << __func__;

  return content_drivers_.size() == 1 &&
         content_drivers_[0]->GetCachedIsVideo();
}

void AssociatedContentManager::SetShouldSend(bool value) {
  DVLOG(1) << __func__ << " " << value;

  should_send_ = value && HasContent();
  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::OnRequestArchive(
    ConversationHandler::AssociatedContentDelegate* delegate) {
  DVLOG(1) << __func__;
  SetArchiveContent(delegate->GetContentId(),
                    std::string(delegate->GetCachedTextContent()),
                    delegate->GetCachedIsVideo());

  // Note: We don't call conversation_->OnAssociatedContentUpdated() here
  // because the content should not have changed.
}

void AssociatedContentManager::OnTitleChanged(
    ConversationHandler::AssociatedContentDelegate* delegate) {
  DVLOG(1) << __func__;

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::OnContentChanged(
    ConversationHandler::AssociatedContentDelegate* delegate) {
  DVLOG(1) << __func__;

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::DetachContent() {
  DVLOG(1) << __func__;

  content_observations_.RemoveAllObservations();
  content_drivers_.clear();
  archive_content_.clear();
}

}  // namespace ai_chat
