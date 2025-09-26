// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_SANITIZED_IMAGE_SOURCE_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_SANITIZED_IMAGE_SOURCE_H_

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "content/public/browser/url_data_source.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/cpp/decode_image.h"
#include "url/gurl.h"

class Profile;
class SkBitmap;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

// Brave versions of SanitizedImageSource available via
// chrome://brave-image/?url=<image_url>&target_size=<width>x<height>
// `target_size` is optional. If provided, the image will be downscaled using
// the same logic as for CSS "object-fit: cover". The aspect ratio is preserved,
// the resulted size is the smallest possible that fits the target size.
// The key differences are:
// * supporting .pad images (for Brave News);
// * supporting target_size. Resized images take less CPU to encode and
//   less memory to store;
// * using different encoding method: PNG FastEncodeBGRASkBitmap.
// * disabling caching (saving renderer memory);
// * USER_VISIBLE_PRIORITY.
class BraveSanitizedImageSource : public content::URLDataSource {
 public:
  explicit BraveSanitizedImageSource(Profile* profile);
  explicit BraveSanitizedImageSource(
      Profile* profile,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveSanitizedImageSource() override;

  void set_pcdn_domain_for_testing(std::string pcdn_domain) {
    pcdn_domain_ = std::move(pcdn_domain);
  }

  void StartDataRequest(
      const GURL& url,
      const content::WebContents::Getter& wc_getter,
      content::URLDataSource::GotDataCallback callback) override;

  bool AllowCaching() override;
  std::string GetMimeType(const GURL& url) override;
  std::string GetSource() override;

 private:
  struct RequestAttributes {
    GURL image_url;
    gfx::Size target_size;
  };
  void OnImageLoaded(std::unique_ptr<network::SimpleURLLoader> loader,
                     RequestAttributes request_attributes,
                     content::URLDataSource::GotDataCallback callback,
                     std::unique_ptr<std::string> body);
  void EncodeAndReplyStaticImage(
      RequestAttributes request_attributes,
      content::URLDataSource::GotDataCallback callback,
      const SkBitmap& bitmap);

  void StartImageDownload(RequestAttributes request_attributes,
                          content::URLDataSource::GotDataCallback callback);

  std::string pcdn_domain_;
  data_decoder::DataDecoder data_decoder_;
  const scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<BraveSanitizedImageSource> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_SANITIZED_IMAGE_SOURCE_H_
