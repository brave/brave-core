/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tabs/ads_tab_helper.h"

#include "brave/browser/brave_ads/ads_service_factory.h"
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
#include "chrome/browser/ui/browser_list.h"  // IWYU pragma: keep
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#endif

namespace brave_ads {

namespace {

constexpr char16_t kSerializeDocumentToStringJavaScript[] =
    u"new XMLSerializer().serializeToString(document)";

constexpr char16_t kDocumentBodyInnerTextJavaScript[] =
    u"document?.body?.innerText";

}  // namespace

AdsTabHelper::AdsTabHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<AdsTabHelper>(*web_contents),
      tab_id_(sessions::SessionTabHelper::IdForTab(web_contents)) {
  if (!tab_id_.is_valid()) {
    // `WebContents` is null or has no `SessionTabHelper`.
    return;
  }

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  ads_service_ = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service_) {
    return;
  }

#if !BUILDFLAG(IS_ANDROID)
  // See "background_helper_android.h" for Android.
  BrowserList::AddObserver(this);

  OnBrowserSetLastActive(BrowserList::GetInstance()->GetLastActive());
#else
  // Set `is_browser_active_` to `true` because `BrowserList` class is not
  // available on Android.
  is_browser_active_ = true;
#endif  // !BUILDFLAG(IS_ANDROID)

  OnVisibilityChanged(web_contents->GetVisibility());
}

AdsTabHelper::~AdsTabHelper() {
#if !BUILDFLAG(IS_ANDROID)
  BrowserList::RemoveObserver(this);
#endif
}

PrefService* AdsTabHelper::GetPrefs() const {
  return Profile::FromBrowserContext(web_contents()->GetBrowserContext())
      ->GetPrefs();
}

bool AdsTabHelper::UserHasJoinedBraveRewards() const {
  return GetPrefs()->GetBoolean(brave_rewards::prefs::kEnabled);
}

bool AdsTabHelper::IsVisible() const {
  return is_web_contents_visible_ && is_browser_active_;
}

bool AdsTabHelper::IsNewNavigation(
    content::NavigationHandle* navigation_handle) {
  CHECK(navigation_handle);

  return ui::PageTransitionIsNewNavigation(
      navigation_handle->GetPageTransition());
}

bool AdsTabHelper::IsErrorPage(content::NavigationHandle* navigation_handle) {
  CHECK(navigation_handle);

  if (navigation_handle->IsErrorPage()) {
    return true;
  }

  const net::HttpResponseHeaders* const response_headers =
      navigation_handle->GetResponseHeaders();
  if (response_headers) {
    const int response_code_class = response_headers->response_code() / 100;
    return response_code_class == 4 /*client error*/ ||
           response_code_class == 5 /*server error*/;
  }

  return false;
}

void AdsTabHelper::ProcessNavigation() {
  MaybeNotifyTabContentDidChange();

  // Set `is_restoring_` to `false` so that listeners are notified of tab
  // changes after the tab is restored.
  is_restoring_ = false;
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
    content::NavigationHandle* navigation_handle) {
  CHECK(navigation_handle);

  if (!ads_service_) {
    return;
  }

  if (is_restoring_) {
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

  if (is_restoring_ || !is_new_navigation_) {
    // Don't notify content changes if the tab was restored or was a previously
    // committed navigation.
    return;
  }

  ads_service_->NotifyTabDidChange(tab_id_.id(), redirect_chain_,
                                   is_error_page_, IsVisible());
}

void AdsTabHelper::MaybeNotifyTabContentDidChange() {
  if (is_restoring_ || !is_new_navigation_ || redirect_chain_.empty() ||
      is_error_page_) {
    // Don't notify content changes if the tab was restored, was a previously
    // committed navigation, the web contents are still loading, or an error
    // page was displayed.
    return;
  }

  MaybeNotifyTabHtmlContentDidChange();
  MaybeNotifyTabTextContentDidChange();
}

void AdsTabHelper::MaybeNotifyTabHtmlContentDidChange() {
  CHECK(!redirect_chain_.empty());

  web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      kSerializeDocumentToStringJavaScript,
      base::BindOnce(&AdsTabHelper::OnMaybeNotifyTabHtmlContentDidChange,
                     weak_factory_.GetWeakPtr(), redirect_chain_),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void AdsTabHelper::OnMaybeNotifyTabHtmlContentDidChange(
    const std::vector<GURL>& redirect_chain,
    base::Value value) {
  if (ads_service_ && value.is_string()) {
    ads_service_->NotifyTabHtmlContentDidChange(tab_id_.id(), redirect_chain,
                                                /*html=*/value.GetString());
  }
}

void AdsTabHelper::MaybeNotifyTabTextContentDidChange() {
  CHECK(!redirect_chain_.empty());

  if (!UserHasJoinedBraveRewards()) {
    return;
  }

  web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      kDocumentBodyInnerTextJavaScript,
      base::BindOnce(&AdsTabHelper::OnMaybeNotifyTabTextContentDidChange,
                     weak_factory_.GetWeakPtr(), redirect_chain_),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void AdsTabHelper::OnMaybeNotifyTabTextContentDidChange(
    const std::vector<GURL>& redirect_chain,
    base::Value value) {
  if (ads_service_ && value.is_string()) {
    ads_service_->NotifyTabTextContentDidChange(tab_id_.id(), redirect_chain,
                                                /*text=*/value.GetString());
  }
}

void AdsTabHelper::MaybeNotifyTabDidStartPlayingMedia() {
  if (ads_service_) {
    ads_service_->NotifyTabDidStopPlayingMedia(tab_id_.id());
  }
}

void AdsTabHelper::MaybeNotifyTabDidStopPlayingMedia() {
  if (ads_service_) {
    ads_service_->NotifyTabDidStopPlayingMedia(tab_id_.id());
  }
}

void AdsTabHelper::MaybeNotifyTabdidClose() {
  if (ads_service_) {
    ads_service_->NotifyDidCloseTab(tab_id_.id());
  }
}

void AdsTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!ads_service_ || !navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }

  is_restoring_ =
      navigation_handle->GetRestoreType() == content::RestoreType::kRestored;

  is_new_navigation_ = IsNewNavigation(navigation_handle);

  redirect_chain_.clear();

  is_error_page_ = false;
}

void AdsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!ads_service_ || !navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }

  redirect_chain_ = navigation_handle->GetRedirectChain();

  is_error_page_ = IsErrorPage(navigation_handle);

  MaybeNotifyUserGestureEventTriggered(navigation_handle);

  MaybeNotifyTabDidChange();

  // For navigations that lead to a document change, `ProcessNavigation` is
  // called from `DocumentOnLoadCompletedInPrimaryMainFrame`.
  if (navigation_handle->IsSameDocument()) {
    ProcessNavigation();
  }
}

void AdsTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  ProcessNavigation();
}

void AdsTabHelper::MediaStartedPlaying(const MediaPlayerInfo& video_type,
                                       const content::MediaPlayerId& id) {
  MaybeNotifyTabDidStartPlayingMedia();
}

void AdsTabHelper::MediaStoppedPlaying(
    const MediaPlayerInfo& video_type,
    const content::MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason reason) {
  MaybeNotifyTabDidStopPlayingMedia();
}

void AdsTabHelper::OnVisibilityChanged(content::Visibility visibility) {
  const bool last_is_web_contents_visible = is_web_contents_visible_;
  is_web_contents_visible_ = visibility == content::Visibility::VISIBLE;

  if (redirect_chain_.empty()) {
    // Don't notify changes for tabs which have not finished loading.
    return;
  }

  if (last_is_web_contents_visible != is_web_contents_visible_) {
    MaybeNotifyTabDidChange();
  }
}

void AdsTabHelper::WebContentsDestroyed() {
  MaybeNotifyTabdidClose();

  ads_service_ = nullptr;
}

#if !BUILDFLAG(IS_ANDROID)
void AdsTabHelper::OnBrowserSetLastActive(Browser* browser) {
  const bool last_is_browser_active = is_browser_active_;
  is_browser_active_ = browser->tab_strip_model()->GetIndexOfWebContents(
                           web_contents()) != TabStripModel::kNoTab;
  if (last_is_browser_active != is_browser_active_) {
    MaybeNotifyBrowserDidBecomeActive();
  }
}

void AdsTabHelper::OnBrowserNoLongerActive(Browser* browser) {
  const bool last_is_browser_active = is_browser_active_;
  is_browser_active_ = browser->tab_strip_model()->GetIndexOfWebContents(
                           web_contents()) == TabStripModel::kNoTab;
  if (last_is_browser_active != is_browser_active_) {
    MaybeNotifyBrowserDidResignActive();
  }
}
#endif

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdsTabHelper);

}  // namespace brave_ads
