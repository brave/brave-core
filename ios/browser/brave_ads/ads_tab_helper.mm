// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_ads/ads_tab_helper.h"

#include <optional>

#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/ios/browser/brave_ads/ads_service_factory_ios.h"
#include "brave/ios/browser/brave_ads/ads_service_impl_ios.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"
#include "ios/web/public/js_messaging/content_world.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr int kHttpClientErrorResponseStatusCodeClass = 4;
constexpr int kHttpServerErrorResponseStatusCodeClass = 5;

constexpr char16_t kSerializeDocumentToStringJavaScript[] =
    u"new XMLSerializer().serializeToString(document)";

constexpr char16_t kDocumentBodyInnerTextJavaScript[] =
    u"document?.body?.innerText";

// Returns 'false' if the navigation was a back/forward navigation or a reload,
// otherwise 'true'.
bool IsNewNavigation(web::NavigationContext* navigation_context) {
  return ui::PageTransitionIsNewNavigation(
      navigation_context->GetPageTransition());
}

bool IsErrorPage(int http_status_code) {
  const int http_status_code_class = http_status_code / 100;
  return http_status_code_class == kHttpClientErrorResponseStatusCodeClass ||
         http_status_code_class == kHttpServerErrorResponseStatusCodeClass;
}

}  // namespace

// static
void AdsTabHelper::MaybeCreateForWebState(web::WebState* web_state) {
  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(web_state->GetBrowserState());

  if (AdsServiceImplIOS* ads_service =
          AdsServiceFactoryIOS::GetForProfile(profile)) {
    AdsTabHelper::CreateForWebState(web_state, ads_service);
  }
}

AdsTabHelper::AdsTabHelper(web::WebState* web_state, AdsService* ads_service)
    : web_state_(web_state),
      tab_id_(web_state->GetUniqueIdentifier().identifier()),
      ads_service_(CHECK_DEREF(ads_service)) {
  web_state_->AddObserver(this);
}

AdsTabHelper::~AdsTabHelper() {
  if (web_state_) {
    web_state_->RemoveObserver(this);
  }
}

void AdsTabHelper::NotifyTabDidStartPlayingMedia() {
  ads_service_->NotifyTabDidStartPlayingMedia(tab_id_);
}

void AdsTabHelper::NotifyTabDidStopPlayingMedia() {
  ads_service_->NotifyTabDidStopPlayingMedia(tab_id_);
}

void AdsTabHelper::WasShown(web::WebState* web_state) {
  OnVisibilityChanged(true);
}

void AdsTabHelper::WasHidden(web::WebState* web_state) {
  OnVisibilityChanged(false);
}

void AdsTabHelper::DidStartNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  redirect_chain_.clear();
  http_status_code_.reset();

  if (web::NavigationManager* navigation_manager =
          web_state->GetNavigationManager()) {
    was_restored_ = navigation_manager->IsNativeRestoreInProgress();
  }

  is_new_navigation_ = IsNewNavigation(navigation_context);
  redirect_chain_.emplace_back(web_state->GetVisibleURL());
}

void AdsTabHelper::DidRedirectNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  redirect_chain_.emplace_back(web_state->GetVisibleURL());
}

void AdsTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  if (!navigation_context->HasCommitted()) {
    return;
  }

  if (net::HttpResponseHeaders* headers =
          navigation_context->GetResponseHeaders()) {
    http_status_code_ = headers->response_code();
  } else {
    http_status_code_ = net::HTTP_OK;
  }

  // Notify of tab changes after navigation completes but before notifying that
  // the tab has loaded, so that any listeners can process the tab changes
  // before the tab is considered loaded.
  MaybeNotifyTabDidChange();

  MaybeNotifyTabDidLoad();

  // Process same document navigations only when a document load is completed.
  // For navigations that lead to a document change, `ProcessNavigation` is
  // called from `DocumentOnLoadCompletedInPrimaryMainFrame`.
  if (navigation_context->IsSameDocument()) {
    MaybeNotifyTabHtmlContentDidChange();

    // Set `was_restored_` to `false` so that listeners are notified of tab
    // changes after the tab is restored.
    was_restored_ = false;
  }
}

void AdsTabHelper::PageLoaded(
    web::WebState* web_state,
    web::PageLoadCompletionStatus load_completion_status) {
  MaybeNotifyTabHtmlContentDidChange();
  MaybeNotifyTabTextContentDidChange();

  // Set `was_restored_` to `false` so that listeners are notified of tab
  // changes after the tab is restored.
  was_restored_ = false;
}

void AdsTabHelper::WebStateDestroyed(web::WebState* web_state) {
  ads_service_->NotifyDidCloseTab(tab_id_);

  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

bool AdsTabHelper::UserHasJoinedBraveRewards() const {
  const PrefService* const prefs =
      ProfileIOS::FromBrowserState(web_state_->GetBrowserState())->GetPrefs();

  return prefs->GetBoolean(brave_rewards::prefs::kEnabled);
}

bool AdsTabHelper::UserHasOptedInToNotificationAds() const {
  const PrefService* const prefs =
      ProfileIOS::FromBrowserState(web_state_->GetBrowserState())->GetPrefs();

  return prefs->GetBoolean(brave_rewards::prefs::kEnabled) &&
         prefs->GetBoolean(prefs::kOptedInToNotificationAds);
}
void AdsTabHelper::MaybeNotifyTabDidChange() {
  if (redirect_chain_.empty()) {
    // Don't notify content changes if the tab redirect chain is empty, i.e.,
    // the web contents are still loading.
    return;
  }

  ads_service_->NotifyTabDidChange(tab_id_, redirect_chain_, is_new_navigation_,
                                   was_restored_, web_state_->IsVisible());
}

void AdsTabHelper::MaybeNotifyTabDidLoad() {
  CHECK(http_status_code_);
  ads_service_->NotifyTabDidLoad(tab_id_, *http_status_code_);
}

void AdsTabHelper::OnVisibilityChanged(bool is_visible) {
  const bool last_is_web_contents_visible = is_web_state_visible_;
  is_web_state_visible_ = is_visible;
  if (last_is_web_contents_visible != is_web_state_visible_) {
    MaybeNotifyTabDidChange();
  }
}

bool AdsTabHelper::ShouldNotifyTabContentDidChange() const {
  // Don't notify about content changes if the ads service is not available, the
  // tab was restored, was a previously committed navigation, the web contents
  // are still loading, or an error page was displayed. `http_status_code_` can
  // be `std::nullopt` if the navigation never finishes which can occur if the
  // user constantly refreshes the page.
  return !was_restored_ && is_new_navigation_ && !redirect_chain_.empty() &&
         http_status_code_ && !IsErrorPage(*http_status_code_);
}

void AdsTabHelper::MaybeNotifyTabHtmlContentDidChange() {
  if (!ShouldNotifyTabContentDidChange()) {
    return;
  }
  if (!UserHasJoinedBraveRewards()) {
    // HTML is not required because verifiable conversions are only supported
    // for Brave Rewards users. However, we must notify that the tab content has
    // changed with empty HTML to ensure that regular conversions are processed.
    ads_service_->NotifyTabHtmlContentDidChange(tab_id_, redirect_chain_,
                                                /*html=*/"");
    return;
  }

  web::WebFrame* main_web_frame =
      web_state_->GetWebFramesManager(web::ContentWorld::kIsolatedWorld)
          ->GetMainWebFrame();
  if (!main_web_frame) {
    ads_service_->NotifyTabHtmlContentDidChange(tab_id_, redirect_chain_,
                                                /*html=*/"");
    return;
  }
  main_web_frame->ExecuteJavaScript(
      kSerializeDocumentToStringJavaScript,
      base::BindOnce(&AdsTabHelper::OnMaybeNotifyTabHtmlContentDidChange,
                     weak_factory_.GetWeakPtr(), redirect_chain_));
}

void AdsTabHelper::OnMaybeNotifyTabHtmlContentDidChange(
    const std::vector<GURL>& redirect_chain,
    const base::Value* value) {
  if (value && value->is_string()) {
    ads_service_->NotifyTabHtmlContentDidChange(tab_id_, redirect_chain,
                                                value->GetString());
  }
}

void AdsTabHelper::MaybeNotifyTabTextContentDidChange() {
  if (!ShouldNotifyTabContentDidChange() ||
      !UserHasOptedInToNotificationAds()) {
    // Only utilized for text classification, which requires the user to have
    // joined Brave Rewards and opted into notification ads.
    return;
  }
  web::WebFrame* main_web_frame =
      web_state_->GetWebFramesManager(web::ContentWorld::kIsolatedWorld)
          ->GetMainWebFrame();
  if (!main_web_frame) {
    return;
  }
  main_web_frame->ExecuteJavaScript(
      kDocumentBodyInnerTextJavaScript,
      base::BindOnce(&AdsTabHelper::OnMaybeNotifyTabTextContentDidChange,
                     weak_factory_.GetWeakPtr(), redirect_chain_));
}

void AdsTabHelper::OnMaybeNotifyTabTextContentDidChange(
    const std::vector<GURL>& redirect_chain,
    const base::Value* value) {
  if (value && value->is_string()) {
    ads_service_->NotifyTabTextContentDidChange(tab_id_, redirect_chain,
                                                value->GetString());
  }
}

}  // namespace brave_ads
