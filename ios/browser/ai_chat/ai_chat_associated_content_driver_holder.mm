// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ai_chat/ai_chat_associated_content_driver_holder.h"

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/ios/browser/ai_chat/ai_chat_associated_content_driver_bridge.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/navigation/navigation_item.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace ai_chat {

AssociatedContentDriverBridgeHolder::AssociatedContentDriverBridgeHolder(
    web::WebState* web_state)
    : AssociatedContentDriver(
          web_state->GetBrowserState()->GetSharedURLLoaderFactory()) {
  auto* item = web_state->GetNavigationManager()->GetLastCommittedItem();
  if (item) {
    SetTitle(GetPageTitle());
    set_url(GetPageURL());
    AssociatedContentDriver::OnNewPage(item->GetUniqueID());
  }
}

AssociatedContentDriverBridgeHolder::~AssociatedContentDriverBridgeHolder() =
    default;

std::u16string AssociatedContentDriverBridgeHolder::GetPageTitle() const {
  NSString* title = [bridge_ pageTitle];
  return title ? base::SysNSStringToUTF16(title) : std::u16string();
}

GURL AssociatedContentDriverBridgeHolder::GetPageURL() const {
  return net::GURLWithNSURL([bridge_ lastCommittedURL]);
}

void AssociatedContentDriverBridgeHolder::GetPageContent(
    FetchPageContentCallback callback,
    std::string_view invalidation_token) {
  SetTitle(GetPageTitle());
  set_url(GetPageURL());

  auto completionHandler = base::CallbackToBlock(base::BindOnce(
      &AssociatedContentDriverBridgeHolder::OnFinishedGetPageContent,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback),
      std::string(invalidation_token)));
  [bridge_ fetchPageContent:completionHandler];
}

void AssociatedContentDriverBridgeHolder::OnFinishedGetPageContent(
    FetchPageContentCallback callback,
    std::string invalidation_token,
    NSString* content,
    BOOL is_video) {
  std::move(callback).Run(
      content ? base::SysNSStringToUTF8(content) : std::string(), is_video,
      invalidation_token);
}

void AssociatedContentDriverBridgeHolder::GetSearchSummarizerKey(
    GetSearchSummarizerKeyCallback callback) {
  std::move(callback).Run(std::nullopt);
}

}  // namespace ai_chat
