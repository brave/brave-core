/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_tab_helper.h"

#include <string>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"  // IWYU pragma: keep
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#endif

namespace brave_ads {

namespace {

constexpr char16_t kGetDocumentHTMLScript[] =
    u"new XMLSerializer().serializeToString(document)";

constexpr char16_t kGetInnerTextScript[] = u"document?.body?.innerText";

}  // namespace

AdsTabHelper::AdsTabHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<AdsTabHelper>(*web_contents),
      tab_id_(sessions::SessionTabHelper::IdForTab(web_contents)),
      weak_factory_(this) {
  if (!tab_id_.is_valid()) {
    return;
  }

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());

  is_incognito_ = !brave::IsRegularProfile(profile);

  ads_service_ = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service_) {
    return;
  }

#if !BUILDFLAG(IS_ANDROID)
  BrowserList::AddObserver(this);
  OnBrowserSetLastActive(BrowserList::GetInstance()->GetLastActive());
#endif
  OnVisibilityChanged(web_contents->GetVisibility());
}

AdsTabHelper::~AdsTabHelper() {
#if !BUILDFLAG(IS_ANDROID)
  BrowserList::RemoveObserver(this);
#endif
}

void AdsTabHelper::TabUpdated() {
  if (!ads_service_) {
    return;
  }

  const bool is_visible = is_active_ && is_browser_active_;

  ads_service_->NotifyTabDidChange(tab_id_.id(), redirect_chain_, is_visible,
                                   is_incognito_);
}

void AdsTabHelper::RunIsolatedJavaScript(
    content::RenderFrameHost* render_frame_host) {
  CHECK(render_frame_host);

  render_frame_host->ExecuteJavaScriptInIsolatedWorld(
      kGetDocumentHTMLScript,
      base::BindOnce(&AdsTabHelper::OnJavaScriptHtmlResult,
                     weak_factory_.GetWeakPtr()),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);

  render_frame_host->ExecuteJavaScriptInIsolatedWorld(
      kGetInnerTextScript,
      base::BindOnce(&AdsTabHelper::OnJavaScriptTextResult,
                     weak_factory_.GetWeakPtr()),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void AdsTabHelper::OnJavaScriptHtmlResult(base::Value value) {
  if (!ads_service_) {
    return;
  }

  if (!value.is_string()) {
    return;
  }

  const std::string& html = value.GetString();
  ads_service_->NotifyTabHtmlContentDidChange(tab_id_.id(), redirect_chain_,
                                              html);
}

void AdsTabHelper::OnJavaScriptTextResult(base::Value value) {
  if (!ads_service_) {
    return;
  }

  if (!value.is_string()) {
    return;
  }

  const std::string& text = value.GetString();
  ads_service_->NotifyTabTextContentDidChange(tab_id_.id(), redirect_chain_,
                                              text);
}

void AdsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  CHECK(navigation_handle);

  if (!ads_service_ || !navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() || !tab_id_.is_valid()) {
    return;
  }

  const bool tab_not_restored =
      navigation_handle->GetRestoreType() == content::RestoreType::kNotRestored;

  // Some browser initiated navigations have HasUserGesture set to false.
  // This should eventually be fixed in crbug.com/617904.
  if (tab_not_restored && (navigation_handle->HasUserGesture() ||
                           !navigation_handle->IsRendererInitiated())) {
    ads_service_->NotifyUserGestureEventTriggered(
        navigation_handle->GetPageTransition());
  }

  redirect_chain_ = navigation_handle->GetRedirectChain();

  if (!navigation_handle->IsSameDocument()) {
    should_process_ = tab_not_restored;
    return;
  }

  content::RenderFrameHost* render_frame_host =
      navigation_handle->GetRenderFrameHost();
  RunIsolatedJavaScript(render_frame_host);
}

void AdsTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  content::RenderFrameHost* render_frame_host =
      web_contents()->GetPrimaryMainFrame();
  if (should_process_) {
    RunIsolatedJavaScript(render_frame_host);
  }
}

void AdsTabHelper::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                 const GURL& validated_url) {
  CHECK(render_frame_host);

  if (render_frame_host->GetParent()) {
    return;
  }

  TabUpdated();
}

void AdsTabHelper::MediaStartedPlaying(const MediaPlayerInfo& video_type,
                                       const content::MediaPlayerId& id) {
  if (ads_service_) {
    ads_service_->NotifyTabDidStartPlayingMedia(tab_id_.id());
  }
}

void AdsTabHelper::MediaStoppedPlaying(
    const MediaPlayerInfo& video_type,
    const content::MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason reason) {
  if (ads_service_) {
    ads_service_->NotifyTabDidStopPlayingMedia(tab_id_.id());
  }
}

void AdsTabHelper::OnVisibilityChanged(content::Visibility visibility) {
  const bool old_is_active = is_active_;

  switch (visibility) {
    case content::Visibility::HIDDEN:
    case content::Visibility::OCCLUDED: {
      is_active_ = false;
      break;
    }

    case content::Visibility::VISIBLE: {
      is_active_ = true;
      break;
    }
  }

  if (old_is_active == is_active_) {
    return;
  }

  TabUpdated();
}

void AdsTabHelper::WebContentsDestroyed() {
  if (!ads_service_) {
    return;
  }

  ads_service_->NotifyDidCloseTab(tab_id_.id());
  ads_service_ = nullptr;
}

#if !BUILDFLAG(IS_ANDROID)
// components/brave_ads/browser/background_helper_android.cc handles Android
void AdsTabHelper::OnBrowserSetLastActive(Browser* browser) {
  if (!browser || !ads_service_) {
    return;
  }

  const bool old_is_browser_active = is_browser_active_;

  if (browser->tab_strip_model()->GetIndexOfWebContents(web_contents()) !=
      TabStripModel::kNoTab) {
    is_browser_active_ = true;
  }

  if (old_is_browser_active == is_browser_active_) {
    return;
  }

  ads_service_->NotifyBrowserDidBecomeActive();
}

void AdsTabHelper::OnBrowserNoLongerActive(Browser* browser) {
  CHECK(browser);

  if (!ads_service_) {
    return;
  }

  const bool old_is_browser_active = is_browser_active_;

  if (browser->tab_strip_model()->GetIndexOfWebContents(web_contents()) !=
      TabStripModel::kNoTab) {
    is_browser_active_ = false;
  }

  if (old_is_browser_active == is_browser_active_) {
    return;
  }

  ads_service_->NotifyBrowserDidResignActive();
}
#endif

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdsTabHelper);

}  // namespace brave_ads
