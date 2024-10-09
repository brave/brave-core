/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/web_image/web_image.h"

#include <memory>

#include "brave/ios/browser/api/web_image/image_downloader.h"
#include "brave/ios/browser/svg/svg_image.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "net/base/apple/url_conversions.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "skia/ext/skia_utils_ios.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_rep_ios.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace WebImage {
// Max size of an SVG if the size cannot be determined intrinsically
static const int max_svg_size = 256;

// Default delay per frame of an animated image (we don't have a way to actually
// determine this intrinsically)
static const float animated_image_frame_delay = 2.0;
}  // namespace WebImage

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
           completion:(void (^)(UIImage* _Nullable image,
                                NSInteger httpResponseCode,
                                NSURL* url))completion {
  GURL url_ = net::GURLWithNSURL(url);
  if (!url_.is_valid() || url_.is_empty()) {
    completion(nullptr, -1, nullptr);
    return;
  }

  image_fetcher_->DownloadImage(
      url_, WebImage::max_svg_size, WebImage::max_svg_size,
      base::BindOnce(^(int download_id, int http_status_code,
                       const GURL& request_url,
                       const std::vector<SkBitmap>& bitmaps,
                       const std::vector<gfx::Size>& bitmap_sizes) {
        if (bitmaps.empty()) {
          completion(nullptr, http_status_code,
                     net::NSURLWithGURL(request_url));
          return;
        }

        completion([WebImageDownloader
                         decodeFrames:bitmaps
                                sizes:bitmap_sizes
                       animationDelay:WebImage::animated_image_frame_delay],
                   http_status_code, net::NSURLWithGURL(request_url));
      }));
}

+ (UIImage*)imageFromData:(NSData*)data {
  std::vector<gfx::Size> sizes;
  std::vector<SkBitmap> frames =
      skia::ImageDataToSkBitmapsWithMaxSize(data, CGFLOAT_MAX);

  for (const auto& frame : frames) {
    sizes.push_back(gfx::Size(frame.width(), frame.height()));
  }
  DCHECK_EQ(frames.size(), sizes.size());

  if (!frames.size()) {
    SkBitmap svg_image = SVGImage::MakeFromData(data, WebImage::max_svg_size,
                                                WebImage::max_svg_size);
    if (!svg_image.empty()) {
      frames.push_back(svg_image);
      sizes.push_back(gfx::Size(svg_image.width(), svg_image.height()));
    }
  }
  DCHECK_EQ(frames.size(), sizes.size());
  return [WebImageDownloader decodeFrames:frames
                                    sizes:sizes
                           animationDelay:WebImage::animated_image_frame_delay];
}

+ (nullable UIImage*)decodeFrames:(const std::vector<SkBitmap>&)frames
                            sizes:(const std::vector<gfx::Size>&)sizes
                   animationDelay:(CGFloat)animationDelay {
  DCHECK_EQ(frames.size(), sizes.size());

  if (frames.size() > 1) {
    NSMutableArray* frame_images = [[NSMutableArray alloc] init];
    for (const auto& bitmap : frames) {
      gfx::Image image = gfx::Image::CreateFrom1xBitmap(bitmap);
      [frame_images addObject:image.IsEmpty() ? [[UIImage alloc] init]
                                              : image.ToUIImage()];
    }

    return [UIImage animatedImageWithImages:frame_images
                                   duration:animationDelay];
  }

  gfx::ImageSkia image_skia;
  for (const auto& bitmap : frames) {
    image_skia.AddRepresentation(gfx::ImageSkiaRep(bitmap, 1.0));
  }

  gfx::Image image = gfx::Image(image_skia);
  return image.IsEmpty() ? nullptr : image.ToUIImage();
}
@end
