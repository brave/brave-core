/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/search_result_ad/search_result_ad_tab_helper.h"

#include "base/feature_list.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_handler.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "url/gurl.h"

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

void SearchResultAdTabHelper::MaybeTriggerSearchResultAdClickedEvent(
    const GURL& navigation_url) {
  if (search_result_ad_handler_) {
    search_result_ad_handler_->MaybeTriggerSearchResultAdClickedEvent(
        navigation_url);
  }
}

// static
void SearchResultAdTabHelper::SetAdsServiceForTesting(AdsService* ads_service) {
  DCHECK(!g_ads_service_for_testing || !ads_service);
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
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  MaybeProcessSearchResultAdClickedEvent(navigation_handle);

  const bool should_trigger_viewed_event =
      navigation_handle->GetRestoreType() ==
          content::RestoreType::kNotRestored &&
      !(navigation_handle->GetPageTransition() &
        ui::PAGE_TRANSITION_FORWARD_BACK);
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

void SearchResultAdTabHelper::MaybeProcessSearchResultAdClickedEvent(
    content::NavigationHandle* navigation_handle) {
  DCHECK(navigation_handle);

  const auto& initiator_origin = navigation_handle->GetInitiatorOrigin();
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !ui::PageTransitionCoreTypeIs(navigation_handle->GetPageTransition(),
                                    ui::PAGE_TRANSITION_LINK) ||
      !initiator_origin ||
      !brave_search::IsAllowedHost(initiator_origin->GetURL())) {
    return;
  }

  content::WebContents* search_result_ad_web_contents =
      navigation_handle->GetWebContents();
  if (!search_result_ad_web_contents) {
    return;
  }

  if (content::WebContents* original_web_contents =
          search_result_ad_web_contents
              ->GetFirstWebContentsInLiveOriginalOpenerChain()) {
    search_result_ad_web_contents = original_web_contents;
  }

  SearchResultAdTabHelper* search_result_ad_tab_helper =
      SearchResultAdTabHelper::FromWebContents(search_result_ad_web_contents);
  if (!search_result_ad_tab_helper) {
    return;
  }

  DCHECK(!navigation_handle->GetRedirectChain().empty());
  const GURL target_url = navigation_handle->GetRedirectChain()[0];
  search_result_ad_tab_helper->MaybeTriggerSearchResultAdClickedEvent(
      target_url);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SearchResultAdTabHelper);

}  // namespace brave_ads
