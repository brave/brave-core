// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/ai_chat/associated_multi_tab_content.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

namespace {

static const auto kAllowedSchemes = base::MakeFixedFlatSet<std::string_view>(
    {url::kHttpsScheme, url::kHttpScheme, url::kFileScheme, url::kDataScheme});

std::vector<AIChatTabHelper*> GetAIChatTabHelpers(Browser* browser) {
  std::vector<AIChatTabHelper*> helpers;
  TabStripModel* tab_strip_model = browser->tab_strip_model();
  for (int i = 0; i < tab_strip_model->count(); ++i) {
    content::WebContents* web_contents = tab_strip_model->GetWebContentsAt(i);
    AIChatTabHelper* associated_content =
        ai_chat::AIChatTabHelper::FromWebContents(web_contents);
    if (base::Contains(kAllowedSchemes,
                       associated_content->GetURL().scheme())) {
      DVLOG(2) << "Tab " << i
               << " URL: " << associated_content->GetURL().spec();
      helpers.push_back(associated_content);
    }
  }
  return helpers;
}

}  // namespace

AssociatedMultiTabContent::AssociatedMultiTabContent(Browser* browser)
    : AssociatedContentDriver(browser->profile()
                                  ->GetDefaultStoragePartition()
                                  ->GetURLLoaderFactoryForBrowserProcess()),
      browser_(browser) {
  DCHECK(browser_);
}

AssociatedMultiTabContent::~AssociatedMultiTabContent() = default;

mojom::AssociatedContentType AssociatedMultiTabContent::GetAssociatedContentType() const {
  return mojom::AssociatedContentType::Web;
}

mojom::SiteInfoDetailPtr AssociatedMultiTabContent::GetAssociatedContentDetail() const {
  auto tabs = GetAIChatTabHelpers(browser_);
  auto details = mojom::MultipleWebSiteInfoDetail::New();
  for (auto* tab : tabs) {
    if (tab->GetURL().SchemeIsHTTPOrHTTPS()) {
      auto detail = mojom::WebSiteInfoDetail::New();
      detail->title = base::UTF16ToUTF8(tab->GetTitle());
      detail->hostname = tab->GetURL().host();
      detail->url = tab->GetURL();
      details->sites.push_back(std::move(detail));
    }
  }

  return mojom::SiteInfoDetail::NewMultipleWebSiteInfo(std::move(details));
}

GURL AssociatedMultiTabContent::GetPageURL() const {
  return browser_->tab_strip_model()->GetActiveWebContents()->GetVisibleURL();
}

std::u16string AssociatedMultiTabContent::GetPageTitle() const {
  return browser_->tab_strip_model()->GetActiveWebContents()->GetTitle();
}

void AssociatedMultiTabContent::GetSearchSummarizerKey(
    GetSearchSummarizerKeyCallback callback) {
  std::move(callback).Run(std::nullopt);
}

void AssociatedMultiTabContent::GetPageContent(
    ConversationHandler::GetPageContentCallback callback,
    std::string_view invalidation_token) {
  auto tabs = GetAIChatTabHelpers(browser_);
  // TODO(petemill): BarrierCallback, wait for all GetPageContent to finish

}

}  // namespace ai_chat
