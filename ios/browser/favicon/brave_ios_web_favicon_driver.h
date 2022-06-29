/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_WEB_FAVICON_DRIVER_H_
#define BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_WEB_FAVICON_DRIVER_H_

#include <memory>
#include <vector>

#include "base/supports_user_data.h"
#include "components/favicon/core/favicon_driver_impl.h"
#import "components/image_fetcher/ios/ios_image_data_fetcher_wrapper.h"
#include "ios/web/navigation/navigation_item_impl.h"
#include "ios/web/public/favicon/favicon_url.h"
#include "ios/web/public/thread/web_thread.h"

class ChromeBrowserState;

namespace web {
struct FaviconStatus;
}  // namespace web

namespace favicon {
class CoreFaviconService;
}  // namespace favicon

namespace brave_favicon {

class BraveIOSWebFaviconDriver : public favicon::FaviconDriverImpl,
                                 public base::SupportsUserData::Data {
 public:
  BraveIOSWebFaviconDriver(const BraveIOSWebFaviconDriver&) = delete;
  BraveIOSWebFaviconDriver& operator=(const BraveIOSWebFaviconDriver&) = delete;

  ~BraveIOSWebFaviconDriver() override;

  static void CreateForBrowserState(
      ChromeBrowserState* browser_state,
      favicon::CoreFaviconService* favicon_service);
  static BraveIOSWebFaviconDriver* FromBrowserState(
      ChromeBrowserState* browser_state);

  void SetMaximumFaviconImageSize(std::size_t max_image_size);

  // FaviconDriver implementation.
  gfx::Image GetFavicon() const override;
  bool FaviconIsValid() const override;
  GURL GetActiveURL() override;

  // FaviconHandler::Delegate implementation.
  int DownloadImage(const GURL& url,
                    int max_image_size,
                    ImageDownloadCallback callback) override;
  void DownloadManifest(const GURL& url,
                        ManifestDownloadCallback callback) override;
  bool IsOffTheRecord() override;
  void OnFaviconUpdated(const GURL& page_url,
                        favicon::FaviconDriverObserver::NotificationIconType
                            notification_icon_type,
                        const GURL& icon_url,
                        bool icon_url_changed,
                        const gfx::Image& image) override;
  void OnFaviconDeleted(const GURL& page_url,
                        favicon::FaviconDriverObserver::NotificationIconType
                            notification_icon_type) override;

  void DidStartNavigation(ChromeBrowserState* browser_state,
                          const GURL& page_url);
  void DidFinishNavigation(ChromeBrowserState* browser_state,
                           const GURL& page_url);
  void FaviconUrlUpdated(const std::vector<web::FaviconURL>& candidates);

 private:
  BraveIOSWebFaviconDriver(ChromeBrowserState* browser_state,
                           favicon::CoreFaviconService* favicon_service);
  void SetFaviconStatus(const GURL& page_url,
                        const web::FaviconStatus& favicon_status,
                        favicon::FaviconDriverObserver::NotificationIconType
                            notification_icon_type,
                        bool icon_url_changed);

  // Image Fetcher used to fetch favicon.
  image_fetcher::IOSImageDataFetcherWrapper image_fetcher_;
  ChromeBrowserState* browser_state_ = nullptr;
  std::unique_ptr<web::NavigationItemImpl> current_item_;
  std::size_t max_image_size_;
};

}  // namespace brave_favicon

#endif  // BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_WEB_FAVICON_DRIVER_H_
