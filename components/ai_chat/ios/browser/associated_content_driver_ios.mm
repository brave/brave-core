// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/ios/browser/associated_content_driver_ios.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/base/apple/foundation_util.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/ios/ai_chat.mojom.objc+private.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/components/ai_chat/ios/browser/ai_chat_delegate.h"
#include "net/base/apple/url_conversions.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace ai_chat {

AssociatedContentDriverIOS::AssociatedContentDriverIOS(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    id<AIChatDelegate> delegate)
    : AssociatedContentDriver(url_loader_factory), bridge_(delegate) {
  SetTitle(GetPageTitle());
  set_url(GetPageURL());
}

AssociatedContentDriverIOS::~AssociatedContentDriverIOS() = default;

std::u16string AssociatedContentDriverIOS::GetPageTitle() const {
  NSString* title = [bridge_ getPageTitle];
  return title ? base::SysNSStringToUTF16(title) : std::u16string();
}

GURL AssociatedContentDriverIOS::GetPageURL() const {
  return net::GURLWithNSURL([bridge_ getLastCommittedURL]);
}

void AssociatedContentDriverIOS::GetPageContent(
    FetchPageContentCallback callback,
    std::string_view invalidation_token) {
  SetTitle(GetPageTitle());
  set_url(GetPageURL());

  [bridge_
      getPageContentWithCompletion:[callback =
                                        std::make_shared<decltype(callback)>(
                                            std::move(callback))](
                                       NSString* content, bool isVideo) {
        if (callback) {
          std::move(*callback).Run(
              content ? base::SysNSStringToUTF8(content) : std::string(),
              isVideo, std::string());
        }
      }];
}

void AssociatedContentDriverIOS::GetSearchSummarizerKey(
    GetSearchSummarizerKeyCallback callback) {
  std::move(callback).Run(std::nullopt);
}

}  // namespace ai_chat
