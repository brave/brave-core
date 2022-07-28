/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/favicon/brave_ios_web_favicon_driver.h"
#import "brave/ios/browser/svg/svg_image.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "components/favicon/core/favicon_url.h"
#include "components/favicon/ios/favicon_url_util.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/web/navigation/navigation_manager_impl.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/favicon/favicon_status.h"
#include "ios/web/public/navigation/navigation_item.h"
#include "ios/web/public/thread/web_thread.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "skia/ext/skia_utils_ios.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace brave_favicon {

BraveIOSWebFaviconDriver* BraveIOSWebFaviconDriver::FromBrowserState(
    ChromeBrowserState* browser_state) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(browser_state);

  return static_cast<BraveIOSWebFaviconDriver*>(
      browser_state->GetUserData("kBraveIOSWebFaviconDriver"));
}

void BraveIOSWebFaviconDriver::CreateForBrowserState(
    ChromeBrowserState* browser_state,
    favicon::CoreFaviconService* favicon_service) {
  if (FromBrowserState(browser_state)) {
    return;
  }

  browser_state->SetUserData("kBraveIOSWebFaviconDriver",
                             base::WrapUnique(new BraveIOSWebFaviconDriver(
                                 browser_state, favicon_service)));
}

void BraveIOSWebFaviconDriver::SetMaximumFaviconImageSize(
    std::size_t max_image_size) {
  max_image_size_ = max_image_size;
}

// FaviconDriver implementation.
gfx::Image BraveIOSWebFaviconDriver::GetFavicon() const {
  static const web::FaviconStatus missing_favicon_status;
  DCHECK(current_item_.get());
  return (current_item_ ? current_item_->GetFaviconStatus()
                        : missing_favicon_status)
      .image;
}

bool BraveIOSWebFaviconDriver::FaviconIsValid() const {
  static const web::FaviconStatus missing_favicon_status;
  DCHECK(current_item_.get());
  return (current_item_ ? current_item_->GetFaviconStatus()
                        : missing_favicon_status)
      .valid;
}

GURL BraveIOSWebFaviconDriver::GetActiveURL() {
  DCHECK(current_item_.get());
  return current_item_ ? current_item_->GetURL() : GURL();
}

// FaviconHandler::Delegate implementation.

int BraveIOSWebFaviconDriver::DownloadImage(const GURL& url,
                                            int max_image_size,
                                            ImageDownloadCallback callback) {
  static int downloaded_image_count = 0;
  int local_download_id = ++downloaded_image_count;

  GURL local_url(url);
  __block ImageDownloadCallback local_callback = std::move(callback);

  image_fetcher::ImageDataFetcherBlock ios_callback =
      ^(NSData* data, const image_fetcher::RequestMetadata& metadata) {
        if (metadata.http_response_code ==
            image_fetcher::RequestMetadata::RESPONSE_CODE_INVALID)
          return;

        std::vector<SkBitmap> frames;
        std::vector<gfx::Size> sizes;
        if (data) {
          frames = skia::ImageDataToSkBitmapsWithMaxSize(
              data, max_image_size_ > 0 ? max_image_size_ : max_image_size);
          for (const auto& frame : frames) {
            sizes.push_back(gfx::Size(frame.width(), frame.height()));
          }
          DCHECK_EQ(frames.size(), sizes.size());

          // When there are no frames, attempt to parse the image as SVG
          // `skia::ImageDataToSkBitmapsWithMaxSize` parses all other formats
          // but not SVG
          if (!frames.size()) {
            SkBitmap svg_image =
                SVGImage::MakeFromData(data, max_image_size, max_image_size);
            if (!svg_image.empty()) {
              frames.push_back(svg_image);
              sizes.push_back(gfx::Size(svg_image.width(), svg_image.height()));
            }
          }
          DCHECK_EQ(frames.size(), sizes.size());
        }
        std::move(local_callback)
            .Run(local_download_id, metadata.http_response_code, local_url,
                 frames, sizes);
      };
  image_fetcher_.FetchImageDataWebpDecoded(url, ios_callback);

  return downloaded_image_count;
}

void BraveIOSWebFaviconDriver::DownloadManifest(
    const GURL& url,
    ManifestDownloadCallback callback) {
  NOTREACHED();
}

bool BraveIOSWebFaviconDriver::IsOffTheRecord() {
  DCHECK(browser_state_);
  return browser_state_->IsOffTheRecord();
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
    ChromeBrowserState* browser_state,
    favicon::CoreFaviconService* favicon_service)
    : FaviconDriverImpl(favicon_service),
      image_fetcher_(browser_state->GetSharedURLLoaderFactory()),
      browser_state_(browser_state),
      current_item_(new web::NavigationItemImpl()),
      max_image_size_(0) {}

BraveIOSWebFaviconDriver::~BraveIOSWebFaviconDriver() = default;

// Notifications

void BraveIOSWebFaviconDriver::DidStartNavigation(
    ChromeBrowserState* browser_state,
    const GURL& page_url) {
  DCHECK_EQ(browser_state_, browser_state);
  DCHECK(current_item_.get());

  current_item_->SetOriginalRequestURL(page_url);
  current_item_->SetURL(page_url);
  current_item_->SetTransitionType(ui::PageTransition::PAGE_TRANSITION_TYPED);
  current_item_->SetNavigationInitiationType(
      web::NavigationInitiationType::BROWSER_INITIATED);
}

void BraveIOSWebFaviconDriver::DidFinishNavigation(
    ChromeBrowserState* browser_state,
    const GURL& page_url) {
  DCHECK_EQ(browser_state_, browser_state);
  DCHECK(current_item_.get());

  // Fetch the fav-icon
  FetchFavicon(current_item_->GetURL(), /*IsSameDocument*/ false);
}

void BraveIOSWebFaviconDriver::FaviconUrlUpdated(
    const std::vector<web::FaviconURL>& candidates) {
  DCHECK(!candidates.empty());
  OnUpdateCandidates(GetActiveURL(),
                     favicon::FaviconURLsFromWebFaviconURLs(candidates),
                     GURL());
}

void BraveIOSWebFaviconDriver::SetFaviconStatus(
    const GURL& page_url,
    const web::FaviconStatus& favicon_status,
    favicon::FaviconDriverObserver::NotificationIconType notification_icon_type,
    bool icon_url_changed) {
  DCHECK(current_item_.get());

  if (!current_item_ || current_item_->GetURL() != page_url) {
    return;
  }

  current_item_->SetFaviconStatus(favicon_status);
  NotifyFaviconUpdatedObservers(notification_icon_type, favicon_status.url,
                                icon_url_changed, favicon_status.image);
}

}  // namespace brave_favicon
