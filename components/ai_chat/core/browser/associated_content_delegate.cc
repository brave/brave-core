// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"

#include <memory>
#include <utility>

#include "base/uuid.h"

namespace ai_chat {

AssociatedContentDelegate::AssociatedContentDelegate()
    : uuid_(base::Uuid::GenerateRandomV4().AsLowercaseString()) {}

AssociatedContentDelegate::~AssociatedContentDelegate() = default;

void AssociatedContentDelegate::OnNewPage(int64_t navigation_id) {
  DVLOG(1) << __func__;

  for (auto& observer : observers_) {
    observer.OnNavigated(this);
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
