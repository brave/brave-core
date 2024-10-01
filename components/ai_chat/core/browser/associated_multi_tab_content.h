// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_MULTI_TAB_CONTENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_MULTI_TAB_CONTENT_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "content/public/browser/web_contents.h"

namespace network {
class SharedURLLoaderFactory;
}

namespace ai_chat {

class AssociatedMultiTabContent : public AssociatedContentDriver,
                                  public AssociatedContentDriver::Observer {
 public:
  AssociatedMultiTabContent(
      std::vector<AssociatedContentDriver*> content,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  AssociatedMultiTabContent(const AssociatedMultiTabContent&) = delete;
  AssociatedMultiTabContent& operator=(const AssociatedMultiTabContent&) =
      delete;
  ~AssociatedMultiTabContent() override;

  void AddContent(AssociatedContentDriver* content);
  // TODO(petemill): by ID
  void RemoveContent(GURL url);
  int GetContentCount() const;

  // AssociatedContentDriver::Observer
  void OnAssociatedContentDestroyed(AssociatedContentDriver* content) override;

  // AssociatedContentDriver
  mojom::AssociatedContentType GetAssociatedContentType() const override;
  mojom::SiteInfoDetailPtr GetAssociatedContentDetail() const override;
  GURL GetPageURL() const override;
  std::u16string GetPageTitle() const override;
  void GetSearchSummarizerKey(GetSearchSummarizerKeyCallback callback) override;
  void GetPageContent(ConversationHandler::GetPageContentCallback callback,
                      std::string_view invalidation_token) override;

 private:
  std::vector<AssociatedContentDriver*> content_;
  base::ScopedMultiSourceObservation<AssociatedContentDriver,
                                     AssociatedContentDriver::Observer>
      content_observations_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_MULTI_TAB_CONTENT_H_
