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
#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/core/browser/associated_archive_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/model_service.h"

namespace ai_chat {

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

  // Remove all owned content - it should be reloaded from the DB.
  for (int i = owned_content_.size() - 1; i >= 0; --i) {
    RemoveContent(owned_content_[i].get(), /*notify_updated=*/false);
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
    owned_content_.push_back(std::make_unique<AssociatedArchiveContent>(
        content->url, archive_content->content,
        base::UTF8ToUTF16(content->title), is_video, content->uuid));
    AddContent(owned_content_.back().get(), /*notify_updated=*/false);

    // Be sure to record the turn that this content is associated with.
    content_uuid_to_conversation_turns_[archive_content->content_uuid] =
        archive_content->conversation_turn_uuid;
  }

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::CreateArchiveContent(
    AssociatedContentDelegate* to_archive) {
  DVLOG(1) << __func__;
  auto content_uuid = to_archive->uuid();
  auto text_content = to_archive->cached_page_content().content;
  auto is_video = to_archive->cached_page_content().is_video;

  auto it = std::ranges::find(content_delegates_, content_uuid,
                              [](const auto& ptr) { return ptr->uuid(); });
  CHECK(it != content_delegates_.end()) << "Couldn't find |content_id|";

  auto* delegate = *it;
  content_observations_.RemoveObservation(delegate);

  // Construct a "content archive" implementation of AssociatedContentDelegate
  // with a duplicate of the article text.
  owned_content_.emplace_back(std::make_unique<AssociatedArchiveContent>(
      delegate->url(), std::move(text_content), delegate->title(), is_video,
      delegate->uuid()));
  *it = owned_content_.back().get();
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

    // Note: When we add a delegate to a conversation we should fetch the
    // content. Otherwise we can end up with a Snapshot with no content (i.e. if
    // the tab is closed).
    // We don't try and keep the content alive to force letting the content to
    // fetch because its a bit of an edge case, and there are no real
    // consequences of not having the content (except for the content not being
    // attached).
    // Additionally, we want to call GetContent even if `notify_updated` is
    // false so we cache the attached content.
    delegate->GetContent(base::BindOnce(
        [](base::WeakPtr<AssociatedContentManager> self, bool notify_updated,
           PageContent content) {
          if (!self || !notify_updated) {
            return;
          }

          // Note: |is_video| may have changed so we need to notify the
          // conversation.
          self->conversation_->OnAssociatedContentUpdated();
        },
        weak_ptr_factory_.GetWeakPtr(), notify_updated));

    content_delegates_.push_back(delegate);
    content_observations_.AddObservation(delegate);
  }

  if (notify_updated) {
    conversation_->OnAssociatedContentUpdated();
  }
}

void AssociatedContentManager::AddOwnedContent(
    std::unique_ptr<AssociatedContentDelegate> delegate,
    bool notify_updated) {
  owned_content_.push_back(std::move(delegate));
  AddContent(owned_content_.back().get(), notify_updated);
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

  // If this is owned content, delete it.
  auto owned_it = std::ranges::find_if(
      owned_content_,
      [delegate](const auto& content) { return content.get() == delegate; });
  if (owned_it != owned_content_.end()) {
    owned_content_.erase(owned_it);
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

void AssociatedContentManager::AssociateUnsentContentWithTurn(
    const mojom::ConversationTurnPtr& turn) {
  CHECK(turn->uuid.has_value());

  for (const auto& content : content_delegates_) {
    if (base::Contains(content_uuid_to_conversation_turns_, content->uuid())) {
      continue;
    }
    content_uuid_to_conversation_turns_[content->uuid()] = turn->uuid.value();
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
  for (auto* delegate : content_delegates_) {
    auto cached_page_content = delegate->cached_page_content();
    mojom::AssociatedContentPtr content = mojom::AssociatedContent::New();
    content->uuid = delegate->uuid();
    content->content_id = delegate->content_id();
    content->url = delegate->url();
    content->title = base::UTF16ToUTF8(delegate->title());
    content->content_type = cached_page_content.is_video
                                ? mojom::ContentType::VideoTranscript
                                : mojom::ContentType::PageContent;

    const uint32_t content_length =
        cached_page_content.content.length() + kAdditionalCharsPerContent;
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

    auto it = content_uuid_to_conversation_turns_.find(content->uuid);
    if (it != content_uuid_to_conversation_turns_.end()) {
      content->conversation_turn_uuid = it->second;
    }

    result.push_back(std::move(content));
    total_consumed_chars += content_length;
  }
  return result;
}

void AssociatedContentManager::HasContentUpdated(
    base::OnceCallback<void(bool)> callback) {
  DVLOG(1) << __func__;

  std::vector<PageContent> cached_content;
  std::ranges::copy(GetCachedContents(), std::back_inserter(cached_content));

  GetContent(base::BindOnce(
      [](base::WeakPtr<AssociatedContentManager> self,
         std::vector<PageContent> cached_content,
         base::OnceCallback<void(bool)> callback) {
        if (!self) {
          return;
        }

        auto new_contents = self->GetCachedContents();
        bool changed = cached_content.size() != new_contents.size();
        if (!changed) {
          for (size_t i = 0; i < cached_content.size(); ++i) {
            auto cached = cached_content[i];
            auto& new_content = new_contents[i].get();
            if (cached != new_content) {
              changed = true;
              break;
            }
          }
        }

        std::move(callback).Run(changed);
      },
      weak_ptr_factory_.GetWeakPtr(), std::move(cached_content),
      std::move(callback)));
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
          [](base::RepeatingClosure callback, PageContent) { callback.Run(); },
          content_callback));
    }
  } else {
    on_page_text_fetch_complete_->Post(FROM_HERE, std::move(callback));
  }
}

void AssociatedContentManager::GetScreenshots(
    mojom::ConversationHandler::GetScreenshotsCallback callback) {
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

PageContents AssociatedContentManager::GetCachedContents() const {
  DVLOG(1) << __func__;
  PageContents result;
  for (auto* delegate : content_delegates_) {
    result.push_back(delegate->cached_page_content());
  }

  return result;
}

PageContentsMap AssociatedContentManager::GetCachedContentsMap() const {
  PageContentsMap result;

  auto contents = GetCachedContents();
  auto meta = GetAssociatedContent();

  for (size_t i = 0; i < contents.size(); ++i) {
    auto turn_id = meta[i]->conversation_turn_uuid;
    DCHECK(turn_id)
        << "This method should only be called when all content has been "
           "associated with a turn (i.e. via AssociateUnsentContentWithTurn)";
    if (!turn_id) {
      continue;
    }
    result[turn_id.value()].push_back(contents[i]);
  }

  return result;
}

bool AssociatedContentManager::HasOpenAIChatPermission() const {
  DVLOG(1) << __func__;

  return content_delegates_.size() == 1 &&
         content_delegates_[0]->HasOpenAIChatPermission();
}

bool AssociatedContentManager::HasLiveContent() const {
  DVLOG(1) << __func__;

  return owned_content_.size() < content_delegates_.size();
}

bool AssociatedContentManager::HasAssociatedContent() const {
  DVLOG(1) << __func__;

  return !content_delegates_.empty();
}

bool AssociatedContentManager::IsVideo() const {
  DVLOG(1) << __func__;

  return content_delegates_.size() == 1 &&
         content_delegates_[0]->cached_page_content().is_video;
}

size_t AssociatedContentManager::GetContentDelegateCount() const {
  return content_delegates_.size();
}

void AssociatedContentManager::OnDestroyed(
    AssociatedContentDelegate* delegate) {
  DVLOG(1) << __func__;

  // Note: creating an archive removes the reference to |delegate| from
  // |content_delegates_| and replaces it with an archive.
  CreateArchiveContent(delegate);
}

void AssociatedContentManager::OnRequestArchive(
    AssociatedContentDelegate* delegate) {
  DVLOG(1) << __func__;

  CreateArchiveContent(delegate);
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
  owned_content_.clear();
}

}  // namespace ai_chat
