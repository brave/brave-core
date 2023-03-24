/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/web_image/image_downloader.h"
#import "brave/ios/browser/svg/svg_image.h"

#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "skia/ext/skia_utils_ios.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace brave {
ImageDownloader::ImageDownloader(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : image_fetcher_(url_loader_factory) {}

ImageDownloader::~ImageDownloader() {}

int ImageDownloader::DownloadImage(const GURL& url,
                                   std::size_t max_svg_width,
                                   std::size_t max_svg_height,
                                   ImageDownloadCallback callback) {
  static int downloaded_image_count = 0;
  int local_download_id = ++downloaded_image_count;

  GURL local_url(url);
  __block ImageDownloadCallback local_callback = std::move(callback);

  image_fetcher::ImageDataFetcherBlock ios_callback =
      ^(NSData* data, const image_fetcher::RequestMetadata& metadata) {
        if (metadata.http_response_code ==
            image_fetcher::RequestMetadata::RESPONSE_CODE_INVALID) {
          std::move(local_callback)
              .Run(local_download_id, metadata.http_response_code, local_url,
                   {}, {});
          return;
        }

        std::vector<SkBitmap> frames;
        std::vector<gfx::Size> sizes;
        if (data) {
          frames = skia::ImageDataToSkBitmapsWithMaxSize(data, CGFLOAT_MAX);

          for (const auto& frame : frames) {
            sizes.push_back(gfx::Size(frame.width(), frame.height()));
          }
          DCHECK_EQ(frames.size(), sizes.size());

          // When there are no frames, attempt to parse the image as SVG
          // `skia::ImageDataToSkBitmapsWithMaxSize` parses all other formats
          // but not SVG
          if (!frames.size()) {
            SkBitmap svg_image =
                SVGImage::MakeFromData(data, max_svg_width, max_svg_height);
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
}  // namespace brave
