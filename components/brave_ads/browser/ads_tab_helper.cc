/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_tab_helper.h"

#include <memory>
#include <utility>

#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/dom_distiller/content/browser/distiller_page_web_contents.h"
#include "components/dom_distiller/content/browser/distiller_javascript_utils.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/resource/resource_bundle.h"

#if !defined(OS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#endif

namespace brave_ads {

AdsTabHelper::AdsTabHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      tab_id_(sessions::SessionTabHelper::IdForTab(web_contents)),
      ads_service_(nullptr),
      is_active_(false),
      is_browser_active_(true),
      run_distiller_(false),
      weak_factory_(this) {
  if (!tab_id_.is_valid()) {
    return;
  }

  Profile* profile = Profile::FromBrowserContext(
      web_contents->GetBrowserContext());
  ads_service_ = AdsServiceFactory::GetForProfile(profile);

#if !defined(OS_ANDROID)
  BrowserList::AddObserver(this);
  OnBrowserSetLastActive(BrowserList::GetInstance()->GetLastActive());
#endif
  OnVisibilityChanged(web_contents->GetVisibility());
}

AdsTabHelper::~AdsTabHelper() {
#if !defined(OS_ANDROID)
  BrowserList::RemoveObserver(this);
#endif
}

bool AdsTabHelper::IsAdsEnabled() const {
  if (!ads_service_ || !ads_service_->IsEnabled()) {
    return false;
  }

  return true;
}

void AdsTabHelper::TabUpdated() {
  if (!IsAdsEnabled()) {
    return;
  }

  ads_service_->OnTabUpdated(tab_id_, web_contents()->GetURL(),
      is_active_, is_browser_active_);
}

void AdsTabHelper::RunIsolatedJavaScript(
    content::RenderFrameHost* render_frame_host) {
  DCHECK(render_frame_host);

  dom_distiller::RunIsolatedJavaScript(render_frame_host,
      "document.body.innerText",
          base::BindOnce(&AdsTabHelper::OnJavaScriptResult,
              weak_factory_.GetWeakPtr()));
}

void AdsTabHelper::OnJavaScriptResult(
    base::Value value) {
  DCHECK(ads_service_ && ads_service_->IsEnabled());

  DCHECK(value.is_string());
  std::string content;
  value.GetAsString(&content);

  ads_service_->OnPageLoaded(tab_id_, redirect_chain_, content);
}

void AdsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() ||
      !tab_id_.is_valid()) {
    return;
  }

  redirect_chain_ = navigation_handle->GetRedirectChain();

  if (navigation_handle->IsSameDocument()) {
    if (!IsAdsEnabled()) {
      // Do not call the ads service if the ad service isn't enabled
      return;
    }

    content::RenderFrameHost* render_frame_host =
        navigation_handle->GetRenderFrameHost();

    RunIsolatedJavaScript(render_frame_host);

    return;
  }

  bool was_restored =
      navigation_handle->GetRestoreType() == content::RestoreType::kRestored;

  run_distiller_ = !was_restored;
}

void AdsTabHelper::DocumentOnLoadCompletedInMainFrame() {
  if (!IsAdsEnabled() || !run_distiller_) {
    // Do not start distilling if the ad service isn't enabled
    return;
  }

  std::unique_ptr<dom_distiller::SourcePageHandleWebContents> handle =
      std::make_unique<dom_distiller::SourcePageHandleWebContents>(
          web_contents(), false);

  content::RenderFrameHost* render_frame_host =
      handle->web_contents()->GetMainFrame();

  RunIsolatedJavaScript(render_frame_host);
}

void AdsTabHelper::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (render_frame_host->GetParent()) {
    return;
  }

  TabUpdated();
}

void AdsTabHelper::MediaStartedPlaying(
    const MediaPlayerInfo& video_type,
    const content::MediaPlayerId& id) {
  if (!IsAdsEnabled()) {
    return;
  }

  ads_service_->OnMediaStart(tab_id_);
}

void AdsTabHelper::MediaStoppedPlaying(
    const MediaPlayerInfo& video_type,
    const content::MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason reason) {
  if (!IsAdsEnabled()) {
    return;
  }

  ads_service_->OnMediaStop(tab_id_);
}

void AdsTabHelper::OnVisibilityChanged(content::Visibility visibility) {
  bool old_active = is_active_;
  if (visibility == content::Visibility::HIDDEN) {
    is_active_ = false;
  } else if (visibility == content::Visibility::OCCLUDED) {
    is_active_ = false;
  } else if (visibility == content::Visibility::VISIBLE) {
    is_active_ = true;
  }

  if (old_active == is_active_) {
    return;
  }

  TabUpdated();
}

void AdsTabHelper::WebContentsDestroyed() {
  if (!IsAdsEnabled()) {
    return;
  }

  ads_service_->OnTabClosed(tab_id_);
  ads_service_ = nullptr;
}

#if !defined(OS_ANDROID)
// components/brave_ads/browser/background_helper_android.cc handles Android
void AdsTabHelper::OnBrowserSetLastActive(Browser* browser) {
  if (!browser) {
    return;
  }

  bool old_active = is_browser_active_;
  if (browser->tab_strip_model()->GetIndexOfWebContents(web_contents()) !=
      TabStripModel::kNoTab) {
    is_browser_active_ = true;
  }

  if (old_active == is_browser_active_) {
    return;
  }

  TabUpdated();
}

void AdsTabHelper::OnBrowserNoLongerActive(Browser* browser) {
  bool old_active = is_browser_active_;
  if (browser->tab_strip_model()->GetIndexOfWebContents(web_contents()) !=
      TabStripModel::kNoTab) {
    is_browser_active_ = false;
  }

  if (old_active == is_browser_active_) {
    return;
  }

  TabUpdated();
}
#endif

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdsTabHelper)

}  // namespace brave_ads
