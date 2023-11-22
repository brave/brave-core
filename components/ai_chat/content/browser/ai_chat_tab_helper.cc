/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/storage_partition.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

AIChatTabHelper::AIChatTabHelper(
    content::WebContents* web_contents,
    AIChatMetrics* ai_chat_metrics,
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter,
    PrefService* local_state_prefs)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AIChatTabHelper>(*web_contents),
      ConversationDriver(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext()),
          ai_chat_metrics,
          std::make_unique<ai_chat::AIChatCredentialManager>(
              skus_service_getter,
              local_state_prefs),
          web_contents->GetBrowserContext()
              ->GetDefaultStoragePartition()
              ->GetURLLoaderFactoryForBrowserProcess()),
      ai_chat_metrics_(ai_chat_metrics) {
  favicon::ContentFaviconDriver::FromWebContents(web_contents)
      ->AddObserver(this);
}

AIChatTabHelper::~AIChatTabHelper() = default;

// content::WebContentsObserver

void AIChatTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  // We might have content here, so check.
  // TODO(petemill): If there are other navigation events to also
  // check if content is available at, then start a queue and make
  // sure we don't have multiple async distills going on at the same time.
  MaybeGeneratePageText();
}

void AIChatTabHelper::WebContentsDestroyed() {
  CleanUp();
  favicon::ContentFaviconDriver::FromWebContents(web_contents())
      ->RemoveObserver(this);
}

void AIChatTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Store current navigation ID of the main document
  // so that we can ignore async responses against any navigated-away-from
  // documents.
  if (!navigation_handle->IsInMainFrame()) {
    DVLOG(3) << "FinishNavigation NOT in main frame";
    return;
  }
  DVLOG(2) << __func__ << navigation_handle->GetNavigationId()
           << " url: " << navigation_handle->GetURL().spec()
           << " same document? " << navigation_handle->IsSameDocument();
  SetNavigationId(navigation_handle->GetNavigationId());
  // Allow same-document navigation, as content often changes as a result
  // of framgment / pushState / replaceState navigations.
  // Content won't be retrieved immediately and we don't have a similar
  // "DOM Content Loaded" event, so let's wait for something else such as
  // page title changing, or a timer completing before calling
  // |MaybeGeneratePageText|.
  SetSameDocumentNavigation(navigation_handle->IsSameDocument());
  // Experimentally only call |CleanUp| _if_ a same-page navigation
  // results in a page title change (see |TtileWasSet|).
  if (!IsSameDocumentNavigation()) {
    CleanUp();
  }
}

void AIChatTabHelper::TitleWasSet(content::NavigationEntry* entry) {
  DVLOG(3) << __func__ << entry->GetTitle();
  if (is_same_document_navigation_) {
    // Seems as good a time as any to check for content after a same-document
    // navigation.
    // We only perform CleanUp here in case it was a minor pushState / fragment
    // navigation and didn't result in new content.
    CleanUp();
    MaybeGeneratePageText();
  }
}

// favicon::FaviconDriverObserver

void AIChatTabHelper::OnFaviconUpdated(
    favicon::FaviconDriver* favicon_driver,
    NotificationIconType notification_icon_type,
    const GURL& icon_url,
    bool icon_url_changed,
    const gfx::Image& image) {
  OnFaviconImageDataChanged();
}

// ai_chat::ConversationDriver

GURL AIChatTabHelper::GetPageURL() const {
  return web_contents()->GetLastCommittedURL();
}

void AIChatTabHelper::GetPageContent(
    base::OnceCallback<void(std::string, bool is_video)> callback) const {
  FetchPageContent(web_contents(), std::move(callback));
}

bool AIChatTabHelper::HasPrimaryMainFrame() const {
  return web_contents()->GetPrimaryMainFrame() != nullptr;
}

bool AIChatTabHelper::IsDocumentOnLoadCompletedInPrimaryMainFrame() const {
  return web_contents()->IsDocumentOnLoadCompletedInPrimaryMainFrame();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);

}  // namespace ai_chat
