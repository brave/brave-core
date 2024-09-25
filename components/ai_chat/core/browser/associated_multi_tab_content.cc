// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_multi_tab_content.h"

#include "base/barrier_callback.h"
#include "base/containers/flat_map.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

AssociatedMultiTabContent::AssociatedMultiTabContent(
    std::vector<AssociatedContentDriver*> content,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : AssociatedContentDriver(url_loader_factory),
      content_(std::move(content)) {
  for (AssociatedContentDriver* associated_content : content_) {
    content_observations_.AddObservation(associated_content);
  }
}

AssociatedMultiTabContent::~AssociatedMultiTabContent() = default;

void AssociatedMultiTabContent::AddContent(AssociatedContentDriver* content) {
  content_.push_back(content);
}

void AssociatedMultiTabContent::RemoveContent(GURL url) {
  std::vector<AssociatedContentDriver*> removedItems;
  std::erase_if(content_,
                [&url, &removedItems](AssociatedContentDriver* content) {
                  if (content->GetURL() == url) {
                    removedItems.push_back(content);
                    return true;
                  }
                  return false;
                });

  for (auto erased : removedItems) {
    content_observations_.RemoveObservation(erased);
  }
}

void AssociatedMultiTabContent::OnAssociatedContentDestroyed(
    AssociatedContentDriver* content) {
  std::erase(content_, content);
  OnContentMetadataChanged();
}

mojom::AssociatedContentType
AssociatedMultiTabContent::GetAssociatedContentType() const {
  return mojom::AssociatedContentType::MultipleWeb;
}

mojom::SiteInfoDetailPtr AssociatedMultiTabContent::GetAssociatedContentDetail()
    const {
  auto details = mojom::MultipleWebSiteInfoDetail::New();
  for (AssociatedContentDriver* content : content_) {
    if (!content) {
      continue;
    }
    auto detail = mojom::WebSiteInfoDetail::New();
    detail->title = base::UTF16ToUTF8(content->GetTitle());
    detail->hostname = content->GetURL().host();
    detail->url = content->GetURL();
    details->sites.push_back(std::move(detail));
  }

  return mojom::SiteInfoDetail::NewMultipleWebSiteInfo(std::move(details));
}

GURL AssociatedMultiTabContent::GetPageURL() const {
  return GURL();
}

std::u16string AssociatedMultiTabContent::GetPageTitle() const {
  return u"AssociatedMultiTabContent::GetPageTitle";
}

void AssociatedMultiTabContent::GetSearchSummarizerKey(
    GetSearchSummarizerKeyCallback callback) {
  std::move(callback).Run(std::nullopt);
}

void AssociatedMultiTabContent::GetPageContent(
    ConversationHandler::GetPageContentCallback callback,
    std::string_view invalidation_token) {
  // wait for all GetPageContent to finish
  auto content_callback = base::BarrierCallback<std::string>(
      content_.size(),
      base::BindOnce(
          [](ConversationHandler::GetPageContentCallback callback,
             std::vector<std::string> results) {
            std::move(callback).Run(
                base::StrCat({"<page>",
                              base::JoinString(results, "</page><page>"),
                              "</page>"}),
                false, "");
          },
          std::move(callback)));
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

}  // namespace ai_chat
