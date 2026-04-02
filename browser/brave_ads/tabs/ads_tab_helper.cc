/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tabs/ads_tab_helper.h"

#include "base/check.h"
#include "base/check_is_test.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "ui/base/page_transition_types.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#endif

namespace brave_ads {

namespace {

constexpr int kHttpClientErrorResponseStatusCodeClass = 4;
constexpr int kHttpServerErrorResponseStatusCodeClass = 5;

constexpr char16_t kDocumentBodyInnerTextJavaScript[] =
    u"document?.body?.innerText";

// Returns 'false' if the navigation was a back/forward navigation or a reload,
// otherwise 'true'.
bool IsNewNavigation(content::NavigationHandle* const navigation_handle) {
  CHECK(navigation_handle);

  return ui::PageTransitionIsNewNavigation(
      navigation_handle->GetPageTransition());
}

// NOTE: DO NOT use this method before the navigation commit as it will return
// null. It is safe to use from `WebContentsObserver::DidFinishNavigation()`.
std::optional<int> HttpStatusCode(
    content::NavigationHandle* const navigation_handle) {
  CHECK(navigation_handle);

  if (const net::HttpResponseHeaders* const response_headers =
          navigation_handle->GetResponseHeaders()) {
    return response_headers->response_code();
  }

  return std::nullopt;
}

bool IsErrorPage(int http_status_code) {
  const int http_status_code_class = http_status_code / 100;
  return http_status_code_class == kHttpClientErrorResponseStatusCodeClass ||
         http_status_code_class == kHttpServerErrorResponseStatusCodeClass;
}

}  // namespace

AdsTabHelper::AdsTabHelper(content::WebContents* const web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AdsTabHelper>(*web_contents),
      session_id_(sessions::SessionTabHelper::IdForTab(web_contents)) {
  if (!session_id_.is_valid()) {
    return;
  }

  Profile* const profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  ads_service_ = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service_) {
    return;
  }

#if !BUILDFLAG(IS_ANDROID)
  // See `application_state_monitor_android.h` for Android.
  BrowserList::AddObserver(this);
#endif  // !BUILDFLAG(IS_ANDROID)

  MaybeSetBrowserIsActive();

  OnVisibilityChanged(web_contents->GetVisibility());
}

AdsTabHelper::~AdsTabHelper() {
#if !BUILDFLAG(IS_ANDROID)
  BrowserList::RemoveObserver(this);
#endif
}

void AdsTabHelper::SetAdsServiceForTesting(AdsService* const ads_service) {
  CHECK_IS_TEST();

  ads_service_ = ads_service;
}

bool AdsTabHelper::UserHasOptedInToNotificationAds() const {
  const PrefService* const prefs =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext())
          ->GetPrefs();

  return prefs->GetBoolean(brave_rewards::prefs::kEnabled) &&
         prefs->GetBoolean(prefs::kOptedInToNotificationAds);
}

bool AdsTabHelper::IsVisible() const {
  // The web contents must be visible and the browser must be active.
  return is_web_contents_visible_ && is_browser_active_.value_or(false);
}

void AdsTabHelper::MaybeSetBrowserIsActive() {
  if (is_browser_active_.has_value() && *is_browser_active_) {
    // Already active.
    return;
  }

  is_browser_active_ = true;

  MaybeNotifyBrowserDidBecomeActive();

  // Maybe notify of tab change after the browser's active state changes because
  // `OnVisibilityChanged` can be called before `OnBrowserSetLastActive`.
  MaybeNotifyTabDidChange();
}

void AdsTabHelper::MaybeSetBrowserIsNoLongerActive() {
  if (is_browser_active_.has_value() && !*is_browser_active_) {
    // Already inactive.
    return;
  }

  is_browser_active_ = false;

  MaybeNotifyBrowserDidResignActive();

  // Maybe notify of tab change after the browser's active state changes because
  // `OnVisibilityChanged` can be called before `OnBrowserNoLongerActive`.
  MaybeNotifyTabDidChange();
}

void AdsTabHelper::ProcessNavigation() {
  MaybeNotifyTabTextContentDidChange();
}

void AdsTabHelper::ResetNavigationState() {
  redirect_chain_.clear();
  redirect_chain_.shrink_to_fit();

  http_status_code_.reset();

  // Clear player tracking on navigation because all players from the previous
  // document are torn down and their IDs become invalid.
  started_media_players_.clear();
  media_players_with_audio_.clear();
}

void AdsTabHelper::MaybeNotifyBrowserDidBecomeActive() {
  if (ads_service_) {
    ads_service_->NotifyBrowserDidBecomeActive();
  }
}

void AdsTabHelper::MaybeNotifyBrowserDidResignActive() {
  if (ads_service_) {
    ads_service_->NotifyBrowserDidResignActive();
  }
}

void AdsTabHelper::MaybeNotifyUserGestureEventTriggered(
    content::NavigationHandle* const navigation_handle) {
  CHECK(navigation_handle);

  if (!ads_service_) {
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
    return;
  }

  if (redirect_chain_.empty()) {
    // Don't notify content changes if the tab redirect chain is empty, i.e.,
    // the web contents are still loading.
    return;
  }

  ads_service_->NotifyTabDidChange(/*tab_id=*/session_id_.id(), redirect_chain_,
                                   is_new_navigation_, was_restored_,
                                   IsVisible());
}

void AdsTabHelper::MaybeNotifyTabDidLoad() {
  CHECK(http_status_code_);

  if (!ads_service_) {
    // No-op if the ads service is unavailable.
    return;
  }

  ads_service_->NotifyTabDidLoad(/*tab_id=*/session_id_.id(),
                                 *http_status_code_);
}

bool AdsTabHelper::ShouldNotifyTabContentDidChange() const {
  // Don't notify about content changes if the ads service is not available, the
  // tab was restored, was a previously committed navigation, the web contents
  // are still loading, or an error page was displayed. `http_status_code_` can
  // be `std::nullopt` if the navigation never finishes which can occur if the
  // user constantly refreshes the page.
  return ads_service_ && !was_restored_ && is_new_navigation_ &&
         !redirect_chain_.empty() && http_status_code_ &&
         !IsErrorPage(*http_status_code_);
}

void AdsTabHelper::MaybeNotifyTabTextContentDidChange() {
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
    base::Value value) {
  if (ads_service_ && value.is_string()) {
    ads_service_->NotifyTabTextContentDidChange(/*tab_id=*/session_id_.id(),
                                                redirect_chain,
                                                /*text=*/value.GetString());
  }
}

bool AdsTabHelper::IsPlayingMediaWithAudio(const content::MediaPlayerId& id) {
  return media_players_with_audio_.contains(id);
}

void AdsTabHelper::PlayerStartedPlayingWithAudio(
    const content::MediaPlayerId& id) {
  if (IsPlayingMediaWithAudio(id)) {
    return;
  }

  media_players_with_audio_.insert(id);
  if (media_players_with_audio_.size() == 1) {
    // If this is the first media player that has started playing, notify that
    // the tab has started playing media.
    MaybeNotifyTabDidStartPlayingMedia();
  }
}

void AdsTabHelper::PlayerStoppedPlayingWithAudio(
    const content::MediaPlayerId& id) {
  if (!IsPlayingMediaWithAudio(id)) {
    return;
  }

  media_players_with_audio_.erase(id);
  if (media_players_with_audio_.empty()) {
    // If this is the last media player that has stopped playing, notify that
    // the tab has stopped playing media.
    MaybeNotifyTabDidStopPlayingMedia();
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
  if (!ads_service_) {
    // No-op if the ads service is unavailable.
    return;
  }

  if (!navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }

  was_restored_ =
      navigation_handle->GetRestoreType() == content::RestoreType::kRestored;

  is_new_navigation_ = IsNewNavigation(navigation_handle);

  ResetNavigationState();
}

// This method is called when a navigation in the main frame or a subframe has
// completed. It indicates that the navigation has finished, but the document
// might still be loading resources.
void AdsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!ads_service_) {
    return;
  }

  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }

  redirect_chain_ = navigation_handle->GetRedirectChain();

  http_status_code_ = HttpStatusCode(navigation_handle).value_or(net::HTTP_OK);

  MaybeNotifyUserGestureEventTriggered(navigation_handle);

  // Notify of tab changes after navigation completes but before notifying that
  // the tab has loaded, so that any listeners can process the tab changes
  // before the tab is considered loaded.
  MaybeNotifyTabDidChange();

  MaybeNotifyTabDidLoad();

  if (navigation_handle->IsSameDocument() &&
      web_contents()->IsDocumentOnLoadCompletedInPrimaryMainFrame()) {
    // Set `was_restored_` to `false` so that listeners are notified of tab
    // changes after the tab is restored.
    was_restored_ = false;
  }
}

// This method is called when the document's onload event has fired in the
// primary main frame. This means that the document and all its subresources
// have finished loading.
void AdsTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  if (!ads_service_) {
    return;
  }

  ProcessNavigation();

  // Set `was_restored_` to `false` so that listeners are notified of tab
  // changes after the tab is restored.
  was_restored_ = false;
}

void AdsTabHelper::MediaStartedPlaying(const MediaPlayerInfo& video_type,
                                       const content::MediaPlayerId& id) {
  started_media_players_.insert(id);

  if (!video_type.has_audio) {
    // Media without an audio track should not suppress ads, as the user is not
    // actively engaged with audible content.
    return;
  }

  PlayerStartedPlayingWithAudio(id);
}

void AdsTabHelper::MediaStoppedPlaying(
    const MediaPlayerInfo& /*video_type*/,
    const content::MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason /*reason*/) {
  started_media_players_.erase(id);
  PlayerStoppedPlayingWithAudio(id);
}

void AdsTabHelper::MediaMutedStatusChanged(const content::MediaPlayerId& id,
                                           bool muted) {
  if (!started_media_players_.contains(id)) {
    // Only process mute transitions for players that have actually started, to
    // avoid spurious notifications from blocked autoplay or pre-start events.
    return;
  }

  if (muted) {
    PlayerStoppedPlayingWithAudio(id);
  } else {
    PlayerStartedPlayingWithAudio(id);
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
  MaybeSetBrowserIsActive();
}

void AdsTabHelper::OnBrowserNoLongerActive(Browser* /*browser*/) {
  MaybeSetBrowserIsNoLongerActive();
}
#endif

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdsTabHelper);

}  // namespace brave_ads
