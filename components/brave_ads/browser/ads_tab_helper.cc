/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_tab_helper.h"

#include <memory>
#include <utility>

#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "chrome/browser/dom_distiller/dom_distiller_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "components/dom_distiller/content/browser/distiller_page_web_contents.h"
#include "components/dom_distiller/content/browser/web_contents_main_frame_observer.h"
#include "components/dom_distiller/core/distiller_page.h"
#include "components/dom_distiller/core/dom_distiller_service.h"
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
      tab_id_(SessionTabHelper::IdForTab(web_contents)),
      ads_service_(nullptr),
      is_active_(false),
      is_browser_active_(true),
      run_distiller_(false),
      weak_factory_(this) {
  if (!tab_id_.is_valid())
    return;

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

void AdsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame() &&
      navigation_handle->GetResponseHeaders()) {
    if (navigation_handle->GetResponseHeaders()->HasHeaderValue(
            "cache-control", "no-store")) {
      run_distiller_ = false;
    } else {
      bool was_restored =
          navigation_handle->GetRestoreType() != content::RestoreType::NONE;
      run_distiller_ = !was_restored;
    }
  }
}

void AdsTabHelper::DocumentOnLoadCompletedInMainFrame() {
  // don't start distilling is the ad service isn't enabled
  if (!ads_service_ || !ads_service_->IsAdsEnabled() || !run_distiller_)
    return;

  auto* dom_distiller_service =
      dom_distiller::DomDistillerServiceFactory::GetForBrowserContext(
          web_contents()->GetBrowserContext());

  if (!dom_distiller_service)
    return;

  auto source_page_handle =
      std::make_unique<dom_distiller::SourcePageHandleWebContents>(
          web_contents(), false);

  auto distiller_page =
      dom_distiller_service->CreateDefaultDistillerPageWithHandle(
          std::move(source_page_handle));

  auto options = dom_distiller::proto::DomDistillerOptions();
  // options.set_extract_text_only(true);
  // options.set_debug_level(1);

  auto* distiller_page_ptr = distiller_page.get();

  distiller_page_ptr->DistillPage(
      web_contents()->GetLastCommittedURL(),
      options,
      base::Bind(&AdsTabHelper::OnWebContentsDistillationDone,
          weak_factory_.GetWeakPtr(),
          web_contents()->GetLastCommittedURL(),
          base::Passed(std::move(distiller_page))));
}

void AdsTabHelper::OnWebContentsDistillationDone(
    const GURL& url,
    std::unique_ptr<dom_distiller::DistillerPage> distiller_page,
    std::unique_ptr<dom_distiller::proto::DomDistillerResult> distiller_result,
    bool distillation_successful) {
  if (!ads_service_ )
    return;

  if (distillation_successful &&
      distiller_result->has_distilled_content() &&
      distiller_result->has_markup_info() &&
      distiller_result->distilled_content().has_html()) {
    ads_service_->ClassifyPage(url.spec(),
                               distiller_result->distilled_content().html());
  } else {
    // TODO(bridiver) - fall back to web_contents()->GenerateMHTML or ignore?
  }
}

void AdsTabHelper::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (render_frame_host->GetParent())
    return;

  TabUpdated();
}

void AdsTabHelper::DidAttachInterstitialPage() {
  TabUpdated();
}

void AdsTabHelper::TabUpdated() {
  if (!ads_service_)
    return;

  ads_service_->TabUpdated(
      tab_id_,
      web_contents()->GetURL(),
      is_active_ && is_browser_active_);
}

void AdsTabHelper::MediaStartedPlaying(const MediaPlayerInfo& video_type,
                         const MediaPlayerId& id) {
  if (ads_service_)
    ads_service_->OnMediaStart(tab_id_);
}

void AdsTabHelper::MediaStoppedPlaying(
    const MediaPlayerInfo& video_type,
    const MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason reason) {
  if (ads_service_)
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

  if (old_active != is_active_)
    TabUpdated();
}

void AdsTabHelper::WebContentsDestroyed() {
  if (ads_service_) {
    ads_service_->TabClosed(tab_id_);
    ads_service_ = nullptr;
  }
}

// TODO(bridiver) - what is the android equivalent of this?
#if !defined(OS_ANDROID)
void AdsTabHelper::OnBrowserSetLastActive(Browser* browser) {
  if (!browser)
    return;

  bool old_active = is_browser_active_;
  if (browser->tab_strip_model()->GetIndexOfWebContents(web_contents()) !=
      TabStripModel::kNoTab) {
    is_browser_active_ = true;
  }

  if (old_active != is_browser_active_)
    TabUpdated();
}

void AdsTabHelper::OnBrowserNoLongerActive(Browser* browser) {
  bool old_active = is_browser_active_;
  if (browser->tab_strip_model()->GetIndexOfWebContents(web_contents()) !=
      TabStripModel::kNoTab) {
    is_browser_active_ = false;
  }

  if (old_active != is_browser_active_)
    TabUpdated();
}
#endif

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdsTabHelper)

}  // namespace brave_ads
