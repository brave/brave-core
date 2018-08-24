/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/payments/payments_helper.h"

#include "brave/browser/payments/payments_service.h"
#include "brave/browser/payments/payments_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/resource_load_info.mojom.h"
#include "content/public/common/resource_type.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"

using content::ResourceType;

namespace payments {

PaymentsHelper::PaymentsHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      tab_id_(SessionTabHelper::IdForTab(web_contents)) {
  if (!tab_id_.is_valid())
    return;

  BrowserList::AddObserver(this);
  Profile* profile = Profile::FromBrowserContext(
      web_contents->GetBrowserContext());
  payments_service_ = PaymentsServiceFactory::GetForProfile(profile);
}

PaymentsHelper::~PaymentsHelper() {
  BrowserList::RemoveObserver(this);
}

void PaymentsHelper::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                   const GURL& validated_url) {
  if (!payments_service_ || render_frame_host->GetParent())
    return;

  payments_service_->OnLoad(tab_id_, validated_url);
}

void PaymentsHelper::DidFinishNavigation(
    content::NavigationHandle* handle) {
  if (!payments_service_ ||
      !handle->IsInMainFrame() ||
      !handle->HasCommitted() ||
      handle->IsDownload())
    return;

  payments_service_->OnUnload(tab_id_);
}

void PaymentsHelper::ResourceLoadComplete(
    content::RenderFrameHost* render_frame_host,
    const content::GlobalRequestID& request_id,
    const content::mojom::ResourceLoadInfo& resource_load_info) {
  if (!payments_service_)
    return;

  if (resource_load_info.resource_type == content::RESOURCE_TYPE_MEDIA ||
      resource_load_info.resource_type == content::RESOURCE_TYPE_XHR) {
    payments_service_->OnXHRLoad(tab_id_, GURL(resource_load_info.url));
  }
}

void PaymentsHelper::DidAttachInterstitialPage() {
  if (payments_service_)
    payments_service_->OnUnload(tab_id_);
}

void PaymentsHelper::MediaStartedPlaying(const MediaPlayerInfo& video_type,
                         const MediaPlayerId& id) {
  if (payments_service_)
    payments_service_->OnMediaStart(tab_id_);
}

void PaymentsHelper::MediaStoppedPlaying(
    const MediaPlayerInfo& video_type,
    const MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason reason) {
  if (payments_service_)
    payments_service_->OnMediaStop(tab_id_);
}

void PaymentsHelper::OnVisibilityChanged(content::Visibility visibility) {
  if (!payments_service_)
    return;

  if (visibility == content::Visibility::HIDDEN) {
    payments_service_->OnHide(tab_id_);
  } else if (visibility == content::Visibility::OCCLUDED) {
    payments_service_->OnBackground(tab_id_);
  } else if (visibility == content::Visibility::VISIBLE) {
    payments_service_->OnShow(tab_id_);
  }
}

void PaymentsHelper::WebContentsDestroyed() {
  if (payments_service_)
    payments_service_->OnUnload(tab_id_);
}

void PaymentsHelper::OnBrowserSetLastActive(Browser* browser) {
  if (!payments_service_)
    return;

  if (browser->tab_strip_model()->GetIndexOfWebContents(web_contents()) !=
      TabStripModel::kNoTab) {
    payments_service_->OnForeground(tab_id_);
  }
}

void PaymentsHelper::OnBrowserNoLongerActive(Browser* browser) {
  if (!payments_service_)
    return;

  if (browser->tab_strip_model()->GetIndexOfWebContents(web_contents()) !=
      TabStripModel::kNoTab) {
    payments_service_->OnBackground(tab_id_);
  }
}

}  // namespace payments
