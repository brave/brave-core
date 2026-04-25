/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_WEB_IMAGE_IMAGE_DOWNLOADER_H_
#define BRAVE_IOS_BROWSER_API_WEB_IMAGE_IMAGE_DOWNLOADER_H_

#include <vector>
#import "components/image_fetcher/ios/ios_image_data_fetcher_wrapper.h"

namespace network {
class SharedURLLoaderFactory;
}

class SkBitmap;
class GURL;

namespace brave {
class ImageDownloader {
 public:
  using ImageDownloadCallback = base::OnceCallback<void(
      int id,
      int status_code,
      const GURL& image_url,
      const std::vector<SkBitmap>& bitmaps,
      const std::vector<gfx::Size>& original_bitmap_sizes)>;

  explicit ImageDownloader(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  virtual ~ImageDownloader();

  virtual int DownloadImage(const GURL& url,
                            std::size_t max_svg_width,
                            std::size_t max_svg_height,
                            ImageDownloadCallback callback);

 private:
  image_fetcher::IOSImageDataFetcherWrapper image_fetcher_;
};
}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_API_WEB_IMAGE_IMAGE_DOWNLOADER_H_
