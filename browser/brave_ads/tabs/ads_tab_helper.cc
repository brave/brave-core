/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tabs/ads_tab_helper.h"

#include "base/check_is_test.h"
#include "base/containers/contains.h"
#include "base/strings/stringprintf.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"
#include "ui/base/page_transition_types.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#endif

namespace brave_ads {

namespace {

constexpr int kHtmlClientErrorResponseCodeClass = 4;
constexpr int kHtmlServerErrorResponseCodeClass = 5;

constexpr char16_t kSerializeDocumentToStringJavaScript[] =
    u"new XMLSerializer().serializeToString(document)";

constexpr char16_t kDocumentBodyInnerTextJavaScript[] =
    u"document?.body?.innerText";

std::string MediaPlayerUuid(const content::MediaPlayerId& id) {
  return base::StringPrintf("%d%d%d", id.frame_routing_id.child_id,
                            id.frame_routing_id.frame_routing_id,
                            id.delegate_id);
}

}  // namespace

AdsTabHelper::AdsTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AdsTabHelper>(*web_contents),
      session_id_(sessions::SessionTabHelper::IdForTab(web_contents)) {
  if (!session_id_.is_valid()) {
    // Invalid session id instance.
    return;
  }

  Profile* const profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  ads_service_ = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service_) {
    // No-op if the ads service is unavailable.
    return;
  }

#if !BUILDFLAG(IS_ANDROID)
  // See "background_helper_android.h" for Android.
  BrowserList::AddObserver(this);
#endif  // !BUILDFLAG(IS_ANDROID)

  MaybeNotifyBrowserDidBecomeActive();

  OnVisibilityChanged(web_contents->GetVisibility());
}

AdsTabHelper::~AdsTabHelper() {
#if !BUILDFLAG(IS_ANDROID)
  BrowserList::RemoveObserver(this);
#endif
}

void AdsTabHelper::SetAdsServiceForTesting(AdsService* ads_service) {
  CHECK_IS_TEST();

  ads_service_ = ads_service;
}

bool AdsTabHelper::UserHasJoinedBraveRewards() const {
  const PrefService* const prefs =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext())
          ->GetPrefs();

  return prefs->GetBoolean(brave_rewards::prefs::kEnabled);
}

bool AdsTabHelper::UserHasOptedInToNotificationAds() const {
  const PrefService* const prefs =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext())
          ->GetPrefs();

  return prefs->GetBoolean(brave_rewards::prefs::kEnabled) &&
         prefs->GetBoolean(prefs::kOptedInToNotificationAds);
}

bool AdsTabHelper::IsVisible() const {
  return is_web_contents_visible_ && is_browser_active_.value_or(false);
}

bool AdsTabHelper::IsNewNavigation(
    content::NavigationHandle* navigation_handle) {
  CHECK(navigation_handle);

  return ui::PageTransitionIsNewNavigation(
      navigation_handle->GetPageTransition());
}

bool AdsTabHelper::IsErrorPage(content::NavigationHandle* navigation_handle) {
  CHECK(navigation_handle);

  if (!navigation_handle->IsErrorPage()) {
    return false;
  }

  // Only consider client and server error responses as error pages.
  if (const net::HttpResponseHeaders* const response_headers =
          navigation_handle->GetResponseHeaders()) {
    const int response_code_class = response_headers->response_code() / 100;
    return response_code_class == kHtmlClientErrorResponseCodeClass ||
           response_code_class == kHtmlServerErrorResponseCodeClass;
  }

  return false;
}

void AdsTabHelper::ProcessNavigation() {
  MaybeNotifyTabHtmlContentDidChange();
  MaybeNotifyTabTextContentDidChange();

  // Set `was_restored_` to `false` so that listeners are notified of tab
  // changes after the tab is restored.
  was_restored_ = false;
}

void AdsTabHelper::ProcessSameDocumentNavigation() {
  MaybeNotifyTabHtmlContentDidChange();

  // Set `was_restored_` to `false` so that listeners are notified of tab
  // changes after the tab is restored.
  was_restored_ = false;
}

void AdsTabHelper::ResetNavigationState() {
  redirect_chain_.clear();
  redirect_chain_.shrink_to_fit();

  is_error_page_ = false;

  media_players_.clear();
}

void AdsTabHelper::MaybeNotifyBrowserDidBecomeActive() {
  if (is_browser_active_ && *is_browser_active_) {
    // Already active.
    return;
  }

  is_browser_active_ = true;

  if (ads_service_) {
    ads_service_->NotifyBrowserDidBecomeActive();
  }

  // Maybe notify of tab change after the browser's active state changes because
  // `OnVisibilityChanged` can be called before `OnBrowserSetLastActive`.
  MaybeNotifyTabDidChange();
}

void AdsTabHelper::MaybeNotifyBrowserDidResignActive() {
  if (is_browser_active_ && !*is_browser_active_) {
    // Already inactive.
    return;
  }

  is_browser_active_ = false;

  if (ads_service_) {
    ads_service_->NotifyBrowserDidResignActive();
  }

  // Maybe notify of tab change after the browser's active state changes because
  // `OnVisibilityChanged` can be called before `OnBrowserNoLongerActive`.
  MaybeNotifyTabDidChange();
}

void AdsTabHelper::MaybeNotifyUserGestureEventTriggered(
    content::NavigationHandle* navigation_handle) {
  CHECK(navigation_handle);

  if (!ads_service_) {
    // No-op if the ads service is unavailable.
    return;
  }

  if (was_restored_) {
    // Don't notify user gesture events for restored tabs.
    return;
  }

  if (!navigation_handle->HasUserGesture() &&
      navigation_handle->IsRendererInitiated()) {
    // Some browser initiated navigations return `false` for `HasUserGesture` so
    // we must also check `IsRendererInitiated`. See crbug.com/617904.
    return;
  }

  const ui::PageTransition page_transition =
      navigation_handle->GetPageTransition();
  ads_service_->NotifyUserGestureEventTriggered(page_transition);
}

void AdsTabHelper::MaybeNotifyTabDidChange() {
  if (!ads_service_) {
    // No-op if the ads service is unavailable.
    return;
  }

  if (redirect_chain_.empty()) {
    // Don't notify content changes if the tab redirect chain is empty, i.e.,
    // the web contents are still loading.
    return;
  }

  ads_service_->NotifyTabDidChange(/*tab_id=*/session_id_.id(), redirect_chain_,
                                   is_new_navigation_, was_restored_,
                                   is_error_page_, IsVisible());
}

bool AdsTabHelper::ShouldNotifyTabContentDidChange() const {
  // Don't notify about content changes if the ads service is not available, the
  // tab was restored, was a previously committed navigation, the web contents
  // are still loading, or an error page was displayed.
  return !was_restored_ && is_new_navigation_ && !redirect_chain_.empty() &&
         !is_error_page_;
}

void AdsTabHelper::MaybeNotifyTabHtmlContentDidChange() {
  if (!ads_service_) {
    // No-op if the ads service is unavailable.
    return;
  }

  if (!ShouldNotifyTabContentDidChange()) {
    return;
  }

  if (!UserHasJoinedBraveRewards()) {
    // HTML is not required because verifiable conversions are only supported
    // for Brave Rewards users. However, we must notify that the tab content has
    // changed with empty HTML to ensure that regular conversions are processed.
    return ads_service_->NotifyTabHtmlContentDidChange(
        /*tab_id=*/session_id_.id(), redirect_chain_,
        /*html=*/"");
  }

  // Only utilized for verifiable conversions, which requires the user to have
  // joined Brave Rewards.
  web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      kSerializeDocumentToStringJavaScript,
      base::BindOnce(&AdsTabHelper::OnMaybeNotifyTabHtmlContentDidChange,
                     weak_factory_.GetWeakPtr(), redirect_chain_),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void AdsTabHelper::OnMaybeNotifyTabHtmlContentDidChange(
    const std::vector<GURL>& redirect_chain,
    base::Value value) const {
  if (ads_service_ && value.is_string()) {
    ads_service_->NotifyTabHtmlContentDidChange(/*tab_id=*/session_id_.id(),
                                                redirect_chain,
                                                /*html=*/value.GetString());
  }
}

void AdsTabHelper::MaybeNotifyTabTextContentDidChange() {
  if (!ads_service_) {
    // No-op if the ads service is unavailable.
    return;
  }

  if (!ShouldNotifyTabContentDidChange()) {
    return;
  }

  if (UserHasOptedInToNotificationAds()) {
    // Only utilized for text classification, which requires the user to have
    // joined Brave Rewards and opted into notification ads.
    web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
        kDocumentBodyInnerTextJavaScript,
        base::BindOnce(&AdsTabHelper::OnMaybeNotifyTabTextContentDidChange,
                       weak_factory_.GetWeakPtr(), redirect_chain_),
        ISOLATED_WORLD_ID_BRAVE_INTERNAL);
  }
}

void AdsTabHelper::OnMaybeNotifyTabTextContentDidChange(
    const std::vector<GURL>& redirect_chain,
    base::Value value) const {
  if (ads_service_ && value.is_string()) {
    ads_service_->NotifyTabTextContentDidChange(/*tab_id=*/session_id_.id(),
                                                redirect_chain,
                                                /*text=*/value.GetString());
  }
}

void AdsTabHelper::MaybeNotifyTabDidStartPlayingMedia() {
  if (ads_service_) {
    ads_service_->NotifyTabDidStartPlayingMedia(/*tab_id=*/session_id_.id());
  }
}

void AdsTabHelper::MaybeNotifyTabDidStopPlayingMedia() {
  if (ads_service_) {
    ads_service_->NotifyTabDidStopPlayingMedia(/*tab_id=*/session_id_.id());
  }
}

void AdsTabHelper::MaybeNotifyTabdidClose() {
  if (ads_service_) {
    ads_service_->NotifyDidCloseTab(/*tab_id=*/session_id_.id());
  }
}

void AdsTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!ads_service_ || !navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }

  was_restored_ =
      navigation_handle->GetRestoreType() == content::RestoreType::kRestored;

  is_new_navigation_ = IsNewNavigation(navigation_handle);

  ResetNavigationState();
}

void AdsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!ads_service_) {
    // No-op if the ads service is unavailable.
    return;
  }

  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }

  redirect_chain_ = navigation_handle->GetRedirectChain();

  is_error_page_ = IsErrorPage(navigation_handle);

  MaybeNotifyUserGestureEventTriggered(navigation_handle);

  MaybeNotifyTabDidChange();

  // Process same document navigations only when a document load is completed.
  // For navigations that lead to a document change, `ProcessNavigation` is
  // called from `DocumentOnLoadCompletedInPrimaryMainFrame`.
  if (navigation_handle->IsSameDocument() &&
      web_contents()->IsDocumentOnLoadCompletedInPrimaryMainFrame()) {
    ProcessSameDocumentNavigation();
  }
}

void AdsTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  if (!ads_service_) {
    // No-op if the ads service is unavailable.
    return;
  }

  ProcessNavigation();
}

bool AdsTabHelper::IsPlayingMedia(const std::string& media_player_uuid) {
  return base::Contains(media_players_, media_player_uuid);
}

void AdsTabHelper::MediaStartedPlaying(const MediaPlayerInfo& /*video_type*/,
                                       const content::MediaPlayerId& id) {
  const std::string media_player_uuid = MediaPlayerUuid(id);

  if (IsPlayingMedia(media_player_uuid)) {
    // Already playing media.
    return;
  }

  media_players_.insert(media_player_uuid);
  if (media_players_.size() == 1) {
    // If this is the first media player that has started playing, notify that
    // the tab has started playing media.
    MaybeNotifyTabDidStartPlayingMedia();
  }
}

void AdsTabHelper::MediaStoppedPlaying(
    const MediaPlayerInfo& /*video_type*/,
    const content::MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason /*reason*/) {
  const std::string media_player_uuid = MediaPlayerUuid(id);

  if (!IsPlayingMedia(media_player_uuid)) {
    // Not playing media.
    return;
  }

  media_players_.erase(media_player_uuid);
  if (media_players_.empty()) {
    // If this is the last media player that has stopped playing, notify that
    // the tab has stopped playing media.
    MaybeNotifyTabDidStopPlayingMedia();
  }
}

void AdsTabHelper::OnVisibilityChanged(const content::Visibility visibility) {
  const bool last_is_web_contents_visible = is_web_contents_visible_;
  is_web_contents_visible_ = visibility == content::Visibility::VISIBLE;
  if (last_is_web_contents_visible != is_web_contents_visible_) {
    MaybeNotifyTabDidChange();
  }
}

void AdsTabHelper::WebContentsDestroyed() {
  MaybeNotifyTabdidClose();

  ads_service_ = nullptr;
}

#if !BUILDFLAG(IS_ANDROID)
// TODO(https://github.com/brave/brave-browser/issues/24970): Decouple
// BrowserListObserver.

void AdsTabHelper::OnBrowserSetLastActive(Browser* /*browser*/) {
  MaybeNotifyBrowserDidBecomeActive();
}

void AdsTabHelper::OnBrowserNoLongerActive(Browser* /*browser*/) {
  MaybeNotifyBrowserDidResignActive();
}
#endif

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdsTabHelper);

}  // namespace brave_ads
