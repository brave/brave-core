// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"

#include <utility>

#include "base/uuid.h"

namespace ai_chat {

PageContent::PageContent(const PageContent&) = default;
PageContent::PageContent(PageContent&&) = default;
PageContent& PageContent::operator=(const PageContent&) = default;
PageContent& PageContent::operator=(PageContent&&) = default;

PageContent::PageContent() = default;
PageContent::PageContent(std::string content, bool is_video)
    : content(std::move(content)), is_video(is_video) {}

AssociatedContentDelegate::AssociatedContentDelegate()
    : uuid_(base::Uuid::GenerateRandomV4().AsLowercaseString()) {}

AssociatedContentDelegate::~AssociatedContentDelegate() {
  for (auto& observer : observers_) {
    observer.OnDestroyed(this);
  }
}

void AssociatedContentDelegate::OnNewPage(int64_t navigation_id) {
  DVLOG(1) << __func__ << " navigation_id: " << navigation_id;

  // |content_id_| needs to be updated before we notify observers, so they know
  // that they're associated with a different tab now.
  content_id_ = navigation_id;

  // Note: We should request the Archive before updating any of the page details
  // so that the archive uses the old content.
  for (auto& observer : observers_) {
    observer.OnRequestArchive(this);
  }

  // Page content is reset to empty when a new page is navigated to.
  set_cached_page_content(PageContent());
  set_url(GURL());

  // Clear title directly so we don't notify observers.
  title_.clear();
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

void AssociatedContentDelegate::SetTitle(std::u16string title) {
  title_ = std::move(title);

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
