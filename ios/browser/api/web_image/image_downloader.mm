/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/web_image/image_downloader.h"

#include "base/check_op.h"
#import "brave/ios/browser/svg/svg_image.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/cross_thread_pending_shared_url_loader_factory.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "skia/ext/skia_utils_ios.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Injects local-network and cookie blocking options into every URLLoader
// created through the wrapped factory, covering loaders that upstream image
// fetching code creates internally.
class LocalNetworkBlockingURLLoaderFactory
    : public network::SharedURLLoaderFactory {
 public:
  explicit LocalNetworkBlockingURLLoaderFactory(
      scoped_refptr<network::SharedURLLoaderFactory> factory)
      : factory_(std::move(factory)) {}

  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override {
    options |= network::mojom::kURLLoadOptionBlockLocalRequest |
               network::mojom::kURLLoadOptionBlockAllCookies;
    factory_->CreateLoaderAndStart(std::move(loader), request_id, options,
                                   request, std::move(client),
                                   traffic_annotation);
  }

  void Clone(mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver)
      override {
    factory_->Clone(std::move(receiver));
  }

  std::unique_ptr<network::PendingSharedURLLoaderFactory> Clone() override {
    return network::CrossThreadPendingSharedURLLoaderFactory::Create(this);
  }

 private:
  ~LocalNetworkBlockingURLLoaderFactory() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> factory_;
};

}  // namespace

namespace brave {
ImageDownloader::ImageDownloader(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : image_fetcher_(base::MakeRefCounted<LocalNetworkBlockingURLLoaderFactory>(
          url_loader_factory)) {}

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
