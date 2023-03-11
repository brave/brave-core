/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/web_image/web_image.h"
#include "brave/ios/browser/api/web_image/image_downloader.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "net/base/mac/url_conversions.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_rep_ios.h"
#include "url/gurl.h"

#include <memory>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface WebImageDownloader () {
  std::unique_ptr<brave::ImageDownloader> image_fetcher_;
}
@end

@implementation WebImageDownloader
- (instancetype)initWithBrowserState:(ChromeBrowserState*)browserState {
  if ((self = [super init])) {
    image_fetcher_ = std::make_unique<brave::ImageDownloader>(
        browserState->GetSharedURLLoaderFactory());
  }
  return self;
}

- (void)downloadImage:(NSURL*)url
         maxImageSize:(NSUInteger)maxImageSize
       animationDelay:(CGFloat)animationDelay
           completion:(void (^)(UIImage* image,
                                NSInteger httpResponseCode,
                                NSURL* url))completion {
  image_fetcher_->DownloadImage(
      net::GURLWithNSURL(url), maxImageSize,
      base::BindOnce(^(int download_id, int http_status_code,
                       const GURL& request_url,
                       const std::vector<SkBitmap>& bitmaps,
                       const std::vector<gfx::Size>& bitmap_sizes) {
        if (bitmaps.empty()) {
          completion(nullptr, http_status_code,
                     net::NSURLWithGURL(request_url));
          return;
        }

        if (bitmaps.size() > 1) {
          NSMutableArray* frame_images = [[NSMutableArray alloc] init];
          for (const auto& bitmap : bitmaps) {
            gfx::Image image = gfx::Image::CreateFrom1xBitmap(bitmap);
            [frame_images addObject:image.IsEmpty() ? [[UIImage alloc] init]
                                                    : image.ToUIImage()];
          }

          completion([UIImage animatedImageWithImages:frame_images
                                             duration:animationDelay],
                     http_status_code, net::NSURLWithGURL(request_url));
          return;
        }

        gfx::ImageSkia image_skia;
        for (const auto& bitmap : bitmaps) {
          image_skia.AddRepresentation(gfx::ImageSkiaRep(bitmap, 1.0));
        }

        gfx::Image image = gfx::Image(image_skia);
        completion(image.IsEmpty() ? nullptr : image.ToUIImage(),
                   http_status_code, net::NSURLWithGURL(request_url));
      }));
}
@end
