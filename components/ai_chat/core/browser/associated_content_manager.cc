// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_content_manager.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/barrier_closure.h"
#include "base/check.h"
#include "base/functional/callback_forward.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/core/browser/associated_archive_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "third_party/re2/src/re2/re2.h"

namespace ai_chat {

const char kPageTagRegex[] = "</?page>";

AssociatedContentManager::AssociatedContentManager(
    ConversationHandler* conversation)
    : conversation_(conversation) {}

AssociatedContentManager::~AssociatedContentManager() {
  DVLOG(1) << __func__;

  DetachContent();
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

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::SetArchiveContent(std::string content_uuid,
                                                 std::string text_content,
                                                 bool is_video) {
  DVLOG(1) << __func__;

  auto it = std::ranges::find(content_delegates_, content_uuid,
                              [](const auto& ptr) { return ptr->uuid(); });
  CHECK(it != content_delegates_.end()) << "Couldn't find |content_id|";

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

void AssociatedContentManager::AddContent(AssociatedContentDelegate* delegate,
                                          bool notify_updated,
                                          bool detach_existing_content) {
  DVLOG(1) << __func__;

  // Optionally, we can set |delegate| as the only content for this
  // conversation.
  if (detach_existing_content) {
    DetachContent();
  }

  if (delegate) {
    // If we've already added this delegate, don't add it again.
    // Note: We can get here if the user is clicking around quickly in the
    // attachments UI.
    if (std::ranges::find(content_delegates_, delegate, [](const auto& ptr) {
          return ptr;
        }) != content_delegates_.end()) {
      return;
    }

    content_delegates_.push_back(delegate);
    content_observations_.AddObservation(delegate);
  }

  if (notify_updated) {
    conversation_->OnAssociatedContentUpdated();
  }
}

void AssociatedContentManager::RemoveContent(
    AssociatedContentDelegate* delegate,
    bool notify_updated) {
  DVLOG(1) << __func__;

  auto it = std::ranges::find(content_delegates_, delegate,
                              [](const auto& ptr) { return ptr; });
  if (it != content_delegates_.end()) {
    // Let the content know it isn't associated with this conversation
    // anymore.
    content_observations_.RemoveObservation(delegate);
    content_delegates_.erase(it);
  }

  // If this is archived content, delete it.
  auto archive_it = std::ranges::find_if(
      archive_content_,
      [delegate](const auto& content) { return content.get() == delegate; });
  if (archive_it != archive_content_.end()) {
    archive_content_.erase(archive_it);
  }

  if (notify_updated) {
    conversation_->OnAssociatedContentUpdated();
  }
}

void AssociatedContentManager::RemoveContent(std::string_view content_uuid,
                                             bool notify_updated) {
  DVLOG(1) << __func__;

  auto it = std::ranges::find_if(content_delegates_,
                                 [&content_uuid](const auto& delegate) {
                                   return delegate->uuid() == content_uuid;
                                 });
  if (it != content_delegates_.end()) {
    RemoveContent(*it, notify_updated);
  }
}

void AssociatedContentManager::ClearContent() {
  DVLOG(1) << __func__;
  if (!HasAssociatedContent()) {
    return;
  }

  DetachContent();

  conversation_->OnAssociatedContentUpdated();
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
  for (auto* delegate : content_delegates_) {
    mojom::AssociatedContentPtr content = mojom::AssociatedContent::New();
    content->uuid = delegate->uuid();
    content->content_id = delegate->GetContentId();
    content->url = delegate->GetURL();
    content->title = base::UTF16ToUTF8(delegate->GetTitle());
    content->content_type = delegate->GetCachedIsVideo()
                                ? mojom::ContentType::VideoTranscript
                                : mojom::ContentType::PageContent;

    const uint32_t content_length =
        delegate->GetCachedTextContent().length() + kAdditionalCharsPerContent;
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

void AssociatedContentManager::GetContent(base::OnceClosure callback) {
  DVLOG(1) << __func__;

  // Note: |GetContent| on the AssociatedContentDelegate's is sometimes sync,
  // sometimes async depending on whether it has already been run. This means we
  // need to make sure we don't destroy the signal before we post this callback.
  if (!on_page_text_fetch_complete_) {
    on_page_text_fetch_complete_ = std::make_unique<base::OneShotEvent>();

    // Note: When we create the signal here its important we post the callback
    // before running the BarrierCallback, which will null out the signal when
    // it completes (if all the |GetContent| calls are synchronous).
    on_page_text_fetch_complete_->Post(FROM_HERE, std::move(callback));

    // wait for all GetPageContent to finish
    auto content_callback = base::BarrierClosure(
        content_delegates_.size(),
        base::BindOnce(
            [](base::WeakPtr<AssociatedContentManager> self) {
              if (!self) {
                return;
              }

              self->on_page_text_fetch_complete_->Signal();
              self->on_page_text_fetch_complete_ = nullptr;
            },
            weak_ptr_factory_.GetWeakPtr()));
    for (auto* content : content_delegates_) {
      content->GetContent(base::BindOnce(
          [](base::RepeatingClosure callback, std::string content,
             bool is_video, std::string invalidation_token) { callback.Run(); },
          content_callback));
    }
  } else {
    on_page_text_fetch_complete_->Post(FROM_HERE, std::move(callback));
  }
}

void AssociatedContentManager::GetScreenshots(
    ConversationHandler::GetScreenshotsCallback callback) {
  DVLOG(1) << __func__;

  if (content_delegates_.size() == 0) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  auto all_screenshots =
      base::BarrierCallback<std::optional<std::vector<mojom::UploadedFilePtr>>>(
          content_delegates_.size(),
          base::BindOnce(
              [](ConversationHandler::GetScreenshotsCallback callback,
                 std::vector<std::optional<std::vector<mojom::UploadedFilePtr>>>
                     screenshots) {
                bool all_nullopt = true;
                std::vector<mojom::UploadedFilePtr> all_screenshots;
                for (auto& set : screenshots) {
                  if (!set) {
                    continue;
                  }

                  all_nullopt = false;
                  std::ranges::move(set.value(),
                                    std::back_inserter(all_screenshots));
                }
                if (all_nullopt) {
                  std::move(callback).Run(std::nullopt);
                } else {
                  std::move(callback).Run(std::move(all_screenshots));
                }
              },
              std::move(callback)));

  for (auto* content : content_delegates_) {
    content->GetScreenshots(all_screenshots);
  }
}

void AssociatedContentManager::GetStagedEntriesFromContent(
    GetStagedEntriesCallback callback) {
  DVLOG(1) << __func__;

  if (content_delegates_.size() != 1) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  content_delegates_[0]->GetStagedEntriesFromContent(std::move(callback));
}

std::string AssociatedContentManager::GetCachedTextContent() const {
  DVLOG(1) << __func__;

  auto cached_content = GetCachedContent();
  std::vector<std::string> transformed_content;
  std::ranges::transform(cached_content,
                         std::back_inserter(transformed_content),
                         [](const auto& content) {
                           std::string text(content);
                           while (RE2::GlobalReplace(&text, kPageTagRegex, ""))
                             ;
                           return text;
                         });

  // If we only have one content delegate directly return the content.
  // Otherwise, wrap each content in <page> tags.
  if (transformed_content.size() == 1) {
    return std::string(transformed_content[0]);
  }

  if (transformed_content.size() == 0) {
    return "";
  }

  return base::StrCat({"<page>",
                       base::JoinString(transformed_content, "</page><page>"),
                       "</page>"});
}

std::vector<std::string_view> AssociatedContentManager::GetCachedContent()
    const {
  DVLOG(1) << __func__;

  std::vector<std::string_view> result;
  for (auto* delegate : content_delegates_) {
    result.push_back(delegate->GetCachedTextContent());
  }

  return result;
}

bool AssociatedContentManager::HasOpenAIChatPermission() const {
  DVLOG(1) << __func__;

  return content_delegates_.size() == 1 &&
         content_delegates_[0]->HasOpenAIChatPermission();
}

bool AssociatedContentManager::HasNonArchiveContent() const {
  DVLOG(1) << __func__;

  return archive_content_.size() < content_delegates_.size();
}

bool AssociatedContentManager::HasAssociatedContent() const {
  DVLOG(1) << __func__;

  return !content_delegates_.empty();
}

bool AssociatedContentManager::IsVideo() const {
  DVLOG(1) << __func__;

  return content_delegates_.size() == 1 &&
         content_delegates_[0]->GetCachedIsVideo();
}

void AssociatedContentManager::OnNavigated(
    AssociatedContentDelegate* delegate) {
  DVLOG(1) << __func__;
  SetArchiveContent(delegate->uuid(),
                    std::string(delegate->GetCachedTextContent()),
                    delegate->GetCachedIsVideo());

  // Note: We don't call conversation_->OnAssociatedContentUpdated() here
  // because the content should not have changed.
}

void AssociatedContentManager::OnTitleChanged(
    AssociatedContentDelegate* delegate) {
  DVLOG(1) << __func__;

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::DetachContent() {
  DVLOG(1) << __func__;

  content_observations_.RemoveAllObservations();
  content_delegates_.clear();
  archive_content_.clear();
}

}  // namespace ai_chat
