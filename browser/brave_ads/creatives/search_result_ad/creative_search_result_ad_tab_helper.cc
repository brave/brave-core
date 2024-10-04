/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/creatives/search_result_ad/creative_search_result_ad_tab_helper.h"

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_handler.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

AdsService* g_ads_service_for_testing = nullptr;

constexpr char kDataPlacementIdVisibilityCheckJavaScript[] =
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

CreativeSearchResultAdTabHelper::CreativeSearchResultAdTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<CreativeSearchResultAdTabHelper>(
          *web_contents) {}

CreativeSearchResultAdTabHelper::~CreativeSearchResultAdTabHelper() = default;

// static
void CreativeSearchResultAdTabHelper::SetAdsServiceForTesting(
    AdsService* ads_service) {
  CHECK_IS_TEST();
  CHECK(!g_ads_service_for_testing || !ads_service);

  g_ads_service_for_testing = ads_service;
}

// static
void CreativeSearchResultAdTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  CHECK(web_contents);

  if (!web_contents->GetBrowserContext() ||
      web_contents->GetBrowserContext()->IsOffTheRecord()) {
    return;
  }

  if (!ShouldSupportSearchResultAds()) {
    return;
  }

  CreateForWebContents(web_contents);
}

bool CreativeSearchResultAdTabHelper::ShouldHandleCreativeAdEvents() const {
  if (!ShouldSupportSearchResultAds()) {
    // If the feature is disabled, we should not trigger creative ad events.
    return false;
  }

  if (ShouldAlwaysTriggerSearchResultAdEvents()) {
    // If the feature is enabled, we should always trigger creative ad events.
    return true;
  }

  // If the feature is enabled, we should only trigger creative ad events when
  // the user has joined Brave Rewards.
  const Profile* const profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return profile->GetPrefs()->GetBoolean(brave_rewards::prefs::kEnabled);
}

void CreativeSearchResultAdTabHelper::MaybeTriggerCreativeAdClickedEvent(
    const GURL& url) {
  if (creative_search_result_ad_handler_) {
    creative_search_result_ad_handler_->MaybeTriggerCreativeAdClickedEvent(url);
  }
}

///////////////////////////////////////////////////////////////////////////////

AdsService* CreativeSearchResultAdTabHelper::GetAdsService() const {
  if (g_ads_service_for_testing) {
    CHECK_IS_TEST();

    return g_ads_service_for_testing;
  }

  Profile* const profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return AdsServiceFactory::GetForProfile(profile);
}

void CreativeSearchResultAdTabHelper::MaybeCreateCreativeSearchResultAdHandler(
    content::NavigationHandle* const navigation_handle) {
  CHECK(navigation_handle);

  if (!ShouldHandleCreativeAdEvents()) {
    return;
  }

  // Do not trigger ad viewed events if the user navigates back or forward.
  const bool should_trigger_creative_ad_viewed_events =
      (navigation_handle->GetPageTransition() &
       ui::PAGE_TRANSITION_FORWARD_BACK) == 0;

  creative_search_result_ad_handler_ =
      CreativeSearchResultAdHandler::MaybeCreate(
          GetAdsService(), navigation_handle->GetURL(),
          should_trigger_creative_ad_viewed_events);
}

void CreativeSearchResultAdTabHelper::
    MaybeExtractCreativeAdPlacementIdsFromWebPageAndHandleViewedEvents() {
  if (!ShouldHandleCreativeAdEvents()) {
    return;
  }

  if (!creative_search_result_ad_handler_) {
    return;
  }

  creative_search_result_ad_handler_
      ->MaybeExtractCreativeAdPlacementIdsFromWebPage(
          web_contents()->GetPrimaryMainFrame(),
          base::BindOnce(&CreativeSearchResultAdTabHelper::
                             MaybeHandleCreativeAdViewedEvents,
                         weak_factory_.GetWeakPtr()));
}

void CreativeSearchResultAdTabHelper::MaybeHandleCreativeAdViewedEvents(
    const std::vector<std::string> placement_ids) {
  for (const auto& placement_id : placement_ids) {
    MaybeHandleCreativeAdViewedEvent(placement_id);
  }
}

void CreativeSearchResultAdTabHelper::MaybeHandleCreativeAdViewedEvent(
    const std::string& placement_id) {
  CHECK(!placement_id.empty());

  // It is safe to pass `placement_id` directly to the JavaScript function as it
  // has all characters except alphanumerics and -._~ escaped.
  const std::string javascript = base::ReplaceStringPlaceholders(
      kDataPlacementIdVisibilityCheckJavaScript, {placement_id}, nullptr);

  web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      base::UTF8ToUTF16(javascript),
      base::BindOnce(&CreativeSearchResultAdTabHelper::
                         MaybeHandleCreativeAdViewedEventCallback,
                     weak_factory_.GetWeakPtr(), placement_id),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void CreativeSearchResultAdTabHelper::MaybeHandleCreativeAdViewedEventCallback(
    const std::string& placement_id,
    const base::Value value) {
  const bool is_visible = value.is_bool() && value.GetBool();
  if (!is_visible) {
    // If the ad is not visible, we should not trigger the viewed event.
    return;
  }

  if (creative_search_result_ad_handler_) {
    creative_search_result_ad_handler_->MaybeTriggerCreativeAdViewedEvent(
        placement_id);
  }
}

void CreativeSearchResultAdTabHelper::MaybeHandleCreativeAdClickedEvent(
    content::NavigationHandle* const navigation_handle) {
  CHECK(navigation_handle);

  if (!ShouldHandleCreativeAdEvents()) {
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

  content::WebContents* web_contents = navigation_handle->GetWebContents();
  if (!web_contents) {
    return;
  }

  if (content::WebContents* original_opener_web_contents =
          web_contents->GetFirstWebContentsInLiveOriginalOpenerChain()) {
    web_contents = original_opener_web_contents;
  }

  CreativeSearchResultAdTabHelper* const creative_search_result_ad_tab_helper =
      CreativeSearchResultAdTabHelper::FromWebContents(web_contents);
  if (!creative_search_result_ad_tab_helper) {
    return;
  }

  CHECK(!navigation_handle->GetRedirectChain().empty());
  const GURL url = navigation_handle->GetRedirectChain().back();

  creative_search_result_ad_tab_helper->MaybeTriggerCreativeAdClickedEvent(url);
}

void CreativeSearchResultAdTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInPrimaryMainFrame()) {
    MaybeHandleCreativeAdClickedEvent(navigation_handle);
  }
}

void CreativeSearchResultAdTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  MaybeCreateCreativeSearchResultAdHandler(navigation_handle);
}

void CreativeSearchResultAdTabHelper::
    DocumentOnLoadCompletedInPrimaryMainFrame() {
  MaybeExtractCreativeAdPlacementIdsFromWebPageAndHandleViewedEvents();
}

void CreativeSearchResultAdTabHelper::WebContentsDestroyed() {
  creative_search_result_ad_handler_.reset();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(CreativeSearchResultAdTabHelper);

}  // namespace brave_ads
