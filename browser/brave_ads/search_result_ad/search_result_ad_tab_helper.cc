/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/search_result_ad/search_result_ad_tab_helper.h"

#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_handler.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"

namespace brave_ads {

namespace {

AdsService* g_ads_service_for_testing = nullptr;

}  // namespace

SearchResultAdTabHelper::SearchResultAdTabHelper(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<SearchResultAdTabHelper>(*web_contents) {}

SearchResultAdTabHelper::~SearchResultAdTabHelper() = default;

// static
void SearchResultAdTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  if (!base::FeatureList::IsEnabled(
          features::kSupportBraveSearchResultAdConfirmationEvents) ||
      !web_contents->GetBrowserContext() ||
      web_contents->GetBrowserContext()->IsOffTheRecord()) {
    return;
  }
  CreateForWebContents(web_contents);
}

absl::optional<GURL>
SearchResultAdTabHelper::MaybeTriggerSearchResultAdClickedEvent(
    const std::string& creative_instance_id) {
  if (!search_result_ad_handler_) {
    return absl::nullopt;
  }

  return search_result_ad_handler_->MaybeTriggerSearchResultAdClickedEvent(
      creative_instance_id);
}

// static
void SearchResultAdTabHelper::SetAdsServiceForTesting(AdsService* ads_service) {
  g_ads_service_for_testing = ads_service;
}

AdsService* SearchResultAdTabHelper::GetAdsService() {
  if (g_ads_service_for_testing) {
    return g_ads_service_for_testing;
  }

  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return AdsServiceFactory::GetForProfile(profile);
}

void SearchResultAdTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  const bool should_trigger_viewed_event =
      navigation_handle->GetRestoreType() == content::RestoreType::kNotRestored;
  search_result_ad_handler_ =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          GetAdsService(), navigation_handle->GetURL(),
          should_trigger_viewed_event);
}

void SearchResultAdTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  content::RenderFrameHost* render_frame_host =
      web_contents()->GetPrimaryMainFrame();
  if (search_result_ad_handler_) {
    search_result_ad_handler_->MaybeRetrieveSearchResultAd(render_frame_host);
  }
}

void SearchResultAdTabHelper::WebContentsDestroyed() {
  search_result_ad_handler_.reset();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SearchResultAdTabHelper);

}  // namespace brave_ads
