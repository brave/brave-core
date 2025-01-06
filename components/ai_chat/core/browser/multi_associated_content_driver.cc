// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/multi_associated_content_driver.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>

#include "base/barrier_callback.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"

namespace ai_chat {

MultiAssociatedContentDriver::MultiAssociatedContentDriver(
    std::vector<AssociatedContentDriver*> content)
    : content_(std::move(content)) {
  for (AssociatedContentDriver* associated_content : content_) {
    content_observations_.AddObservation(associated_content);
  }
}

MultiAssociatedContentDriver::~MultiAssociatedContentDriver() = default;

void MultiAssociatedContentDriver::AddContent(
    AssociatedContentDriver* content) {
  if (base::Contains(content_, content)) {
    return;
  }

  content_observations_.AddObservation(content);
  content_.push_back(content);
}

void MultiAssociatedContentDriver::RemoveContent(
    AssociatedContentDriver* content) {
  std::vector<AssociatedContentDriver*> removed_items;
  std::erase_if(content_,
                [&content, &removed_items](AssociatedContentDriver* item) {
                  if (item == content) {
                    removed_items.push_back(content);
                    return true;
                  }
                  return false;
                });

  for (auto erased : removed_items) {
    content_observations_.RemoveObservation(erased);
  }
}

int MultiAssociatedContentDriver::GetContentCount() const {
  return content_.size();
}

void MultiAssociatedContentDriver::AddRelatedConversation(
    ConversationHandler* conversation) {
  associated_conversations_.insert(conversation);
}

void MultiAssociatedContentDriver::OnRelatedConversationDisassociated(
    ConversationHandler* conversation) {
  associated_conversations_.erase(conversation);
}

int MultiAssociatedContentDriver::GetContentId() const {
  // TODO: maybe static negative id?
  int result = 1;
  for (const auto* content : content_) {
    result *= content->GetContentId();
  }
  return result;
}

GURL MultiAssociatedContentDriver::GetURL() const {
  return GURL();
}

std::u16string MultiAssociatedContentDriver::GetTitle() const {
  return std::u16string();
}

std::vector<mojom::SiteInfoDetailPtr>
MultiAssociatedContentDriver::GetSiteInfoDetail() const {
  std::vector<mojom::SiteInfoDetailPtr> details;
  for (const auto* content : content_) {
    for (auto& detail : content->GetSiteInfoDetail()) {
      details.push_back(std::move(detail));
    }
  }
  return details;
}

void MultiAssociatedContentDriver::OnAssociatedContentDestroyed(
    AssociatedContentDriver* content) {
  std::erase(content_, content);
  content_observations_.RemoveObservation(content);
}

void MultiAssociatedContentDriver::GetContent(
    ConversationHandler::GetPageContentCallback callback) {
  if (!on_page_text_fetch_complete_) {
    on_page_text_fetch_complete_ = std::make_unique<base::OneShotEvent>();
    // wait for all GetPageContent to finish
    auto content_callback = base::BarrierCallback<std::string>(
        content_.size(),
        base::BindOnce(
            [](base::WeakPtr<MultiAssociatedContentDriver> self,
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
    for (const auto& content : content_) {
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
                     [](base::WeakPtr<MultiAssociatedContentDriver> self,
                        ConversationHandler::GetPageContentCallback callback) {
                       if (!self) {
                         return;
                       }
                       std::move(callback).Run(self->cached_text_content_,
                                               self->GetCachedIsVideo(), "");
                     },
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

std::string_view MultiAssociatedContentDriver::GetCachedTextContent() {
  return cached_text_content_;
}

bool MultiAssociatedContentDriver::GetCachedIsVideo() {
  return std::ranges::all_of(content_, [](AssociatedContentDriver* content) {
    return content->GetCachedIsVideo();
  });
}

void MultiAssociatedContentDriver::GetStagedEntriesFromContent(
    ConversationHandler::GetStagedEntriesCallback callback) {
  std::move(callback).Run(std::nullopt);
}

bool MultiAssociatedContentDriver::HasOpenAIChatPermission() const {
  return false;
}

}  // namespace ai_chat
