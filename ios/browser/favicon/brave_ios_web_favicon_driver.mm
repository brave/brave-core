/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/favicon/brave_ios_web_favicon_driver.h"

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "components/favicon/core/favicon_url.h"
#include "components/favicon/ios/favicon_url_util.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/navigation/navigation_manager_impl.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/favicon/favicon_status.h"
#include "ios/web/public/navigation/navigation_item.h"
#include "ios/web/public/thread/web_thread.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace brave_favicon {

void BraveIOSWebFaviconDriver::SetMaximumFaviconImageSize(
    std::size_t max_image_width,
    std::size_t max_image_height) {
  max_image_width_ = max_image_width;
  max_image_height_ = max_image_height;
}

// FaviconDriver implementation.
gfx::Image BraveIOSWebFaviconDriver::GetFavicon() const {
  DCHECK(web_state_);
  return web_state_->GetFaviconStatus().image;
}

bool BraveIOSWebFaviconDriver::FaviconIsValid() const {
  DCHECK(web_state_);
  return web_state_->GetFaviconStatus().valid;
}

GURL BraveIOSWebFaviconDriver::GetActiveURL() {
  DCHECK(web_state_);
  return web_state_->GetLastCommittedURL();
}

// FaviconHandler::Delegate implementation.

int BraveIOSWebFaviconDriver::DownloadImage(const GURL& url,
                                            int max_image_size,
                                            ImageDownloadCallback callback) {
  return image_fetcher_.DownloadImage(
      url, max_image_width_ > 0 ? max_image_width_ : max_image_size,
      max_image_height_ > 0 ? max_image_height_ : max_image_size,
      std::move(callback));
}

void BraveIOSWebFaviconDriver::DownloadManifest(
    const GURL& url,
    ManifestDownloadCallback callback) {
  // We don't support downloading manifests
  NOTREACHED();
}

bool BraveIOSWebFaviconDriver::IsOffTheRecord() {
  DCHECK(web_state_);
  return web_state_->GetBrowserState()->IsOffTheRecord();
}

void BraveIOSWebFaviconDriver::OnFaviconUpdated(
    const GURL& page_url,
    favicon::FaviconDriverObserver::NotificationIconType notification_icon_type,
    const GURL& icon_url,
    bool icon_url_changed,
    const gfx::Image& image) {
  web::FaviconStatus favicon_status;
  favicon_status.valid = true;
  favicon_status.image = image;
  favicon_status.url = icon_url;

  SetFaviconStatus(page_url, favicon_status, notification_icon_type,
                   icon_url_changed);
}

void BraveIOSWebFaviconDriver::OnFaviconDeleted(
    const GURL& page_url,
    favicon::FaviconDriverObserver::NotificationIconType
        notification_icon_type) {
  SetFaviconStatus(page_url, web::FaviconStatus(), notification_icon_type,
                   /*icon_url_changed=*/true);
}

// Constructor / Destructor

BraveIOSWebFaviconDriver::BraveIOSWebFaviconDriver(
    web::WebState* web_state,
    favicon::CoreFaviconService* favicon_service)
    : FaviconDriverImpl(favicon_service),
      image_fetcher_(web_state->GetBrowserState()->GetSharedURLLoaderFactory()),
      max_image_width_(0),
      max_image_height_(0),
      web_state_(web_state) {
  web_state_->AddObserver(this);
}

BraveIOSWebFaviconDriver::~BraveIOSWebFaviconDriver() {
  // WebFaviconDriver is owned by WebState (as it is a WebStateUserData), so
  // the WebStateDestroyed will be called before the destructor and the member
  // field reset to null.
  DCHECK(!web_state_);
}

// Notifications

void BraveIOSWebFaviconDriver::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  // Fetch the favicon
  FetchFavicon(web_state->GetLastCommittedURL(), /*IsSameDocument*/ false);
  // navigation_context->IsSameDocument()
}

void BraveIOSWebFaviconDriver::FaviconUrlUpdated(
    web::WebState* web_state,
    const std::vector<web::FaviconURL>& candidates) {
  DCHECK_EQ(web_state_, web_state);
  DCHECK(!candidates.empty());
  OnUpdateCandidates(GetActiveURL(),
                     favicon::FaviconURLsFromWebFaviconURLs(candidates),
                     GURL());
}

void BraveIOSWebFaviconDriver::WebStateDestroyed(web::WebState* web_state) {
  DCHECK_EQ(web_state_, web_state);
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

void BraveIOSWebFaviconDriver::SetFaviconStatus(
    const GURL& page_url,
    const web::FaviconStatus& favicon_status,
    favicon::FaviconDriverObserver::NotificationIconType notification_icon_type,
    bool icon_url_changed) {
  DCHECK(web_state_);

  // Check whether the active URL has changed since FetchFavicon() was called.
  // On iOS, the active URL can change between calls to FetchFavicon(). For
  // instance, FetchFavicon() is not synchronously called when the active URL
  // changes as a result of CRWSessionController::goToEntry().
  if (web_state_->GetLastCommittedURL() != page_url) {
    return;
  }

  web_state_->SetFaviconStatus(favicon_status);
  NotifyFaviconUpdatedObservers(notification_icon_type, favicon_status.url,
                                icon_url_changed, favicon_status.image);
}

WEB_STATE_USER_DATA_KEY_IMPL(BraveIOSWebFaviconDriver)

}  // namespace brave_favicon
