/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ad_units/search_result_ad/search_result_ad_tab_helper.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/content/browser/ad_units/search_result_ad/search_result_ad_handler.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

AdsService* g_ads_service_for_testing = nullptr;

constexpr char kCheckForAdWithDataPlacementIdVisible[] =
    R"(
        (function () {
          const element = document.querySelector('div[data-placement-id="$1"]');
          if (!element) {
            return false;
          }
          const style = window.getComputedStyle(element);
          return style.display !== 'none' && style.visibility !== 'hidden';
        })()
    )";

}  // namespace

SearchResultAdTabHelper::SearchResultAdTabHelper(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<SearchResultAdTabHelper>(*web_contents) {}

SearchResultAdTabHelper::~SearchResultAdTabHelper() = default;

bool SearchResultAdTabHelper::ShouldHandleSearchResultAdEvents() const {
  if (!ShouldSupportSearchResultAds()) {
    return false;
  }

  if (ShouldAlwaysTriggerSearchResultAdEvents()) {
    return true;
  }

  const Profile* const profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return profile->GetPrefs()->GetBoolean(brave_rewards::prefs::kEnabled);
}

// static
void SearchResultAdTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  CHECK(web_contents);

  if (ShouldSupportSearchResultAds() && web_contents->GetBrowserContext() &&
      !web_contents->GetBrowserContext()->IsOffTheRecord()) {
    CreateForWebContents(web_contents);
  }
}

void SearchResultAdTabHelper::MaybeTriggerSearchResultAdClickedEvent(
    const GURL& navigation_url) {
  if (ShouldHandleSearchResultAdEvents() && search_result_ad_handler_) {
    search_result_ad_handler_->MaybeTriggerSearchResultAdClickedEvent(
        navigation_url);
  }
}

// static
void SearchResultAdTabHelper::SetAdsServiceForTesting(AdsService* ads_service) {
  CHECK(!g_ads_service_for_testing || !ads_service);
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

void SearchResultAdTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }

  MaybeProcessSearchResultAdClickedEvent(navigation_handle);
}

void SearchResultAdTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  if (!ShouldHandleSearchResultAdEvents()) {
    return;
  }

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
  if (!ShouldHandleSearchResultAdEvents() || !search_result_ad_handler_) {
    return;
  }

  content::RenderFrameHost* render_frame_host =
      web_contents()->GetPrimaryMainFrame();

  search_result_ad_handler_->MaybeRetrieveSearchResultAd(
      render_frame_host,
      base::BindOnce(&SearchResultAdTabHelper::OnRetrieveSearchResultAd,
                     weak_factory_.GetWeakPtr()));
}

void SearchResultAdTabHelper::WebContentsDestroyed() {
  search_result_ad_handler_.reset();
}

void SearchResultAdTabHelper::MaybeProcessSearchResultAdClickedEvent(
    content::NavigationHandle* navigation_handle) {
  CHECK(navigation_handle);

  if (!ShouldHandleSearchResultAdEvents()) {
    return;
  }

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

  CHECK(!navigation_handle->GetRedirectChain().empty());
  const GURL target_url = navigation_handle->GetRedirectChain()[0];
  search_result_ad_tab_helper->MaybeTriggerSearchResultAdClickedEvent(
      target_url);
}

void SearchResultAdTabHelper::OnRetrieveSearchResultAd(
    std::vector<std::string> placement_ids) {
  if (!search_result_ad_handler_ || placement_ids.empty()) {
    return;
  }

  for (const auto& placement_id : placement_ids) {
    if (placement_id.empty() || base::Contains(placement_id, '\"')) {
      continue;
    }
    const std::string script = base::ReplaceStringPlaceholders(
        kCheckForAdWithDataPlacementIdVisible, {placement_id}, nullptr);
    content::RenderFrameHost* render_frame_host =
        web_contents()->GetPrimaryMainFrame();
    render_frame_host->ExecuteJavaScriptInIsolatedWorld(
        base::ASCIIToUTF16(script),
        base::BindOnce(
            &SearchResultAdTabHelper::OnCheckForAdWithDataPlacementIdVisible,
            weak_factory_.GetWeakPtr(), placement_id),
        ISOLATED_WORLD_ID_BRAVE_INTERNAL);
  }
}

void SearchResultAdTabHelper::OnCheckForAdWithDataPlacementIdVisible(
    const std::string& placement_id,
    base::Value value) {
  if (ShouldHandleSearchResultAdEvents() && search_result_ad_handler_ &&
      value.is_bool() && value.GetBool()) {
    search_result_ad_handler_->MaybeTriggerSearchResultAdViewedEvent(
        placement_id);
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SearchResultAdTabHelper);

}  // namespace brave_ads
