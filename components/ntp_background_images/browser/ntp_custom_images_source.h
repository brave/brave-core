// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_CUSTOM_IMAGES_SOURCE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_CUSTOM_IMAGES_SOURCE_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/url_data_source.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace gfx {
class Image;
}  // namespace gfx

namespace base {
class FilePath;
}  // namespace base

namespace image_fetcher {
class ImageDecoder;
}  // namespace image_fetcher

namespace ntp_background_images {

class NTPCustomBackgroundImagesService;

// This serves background image data.
class NTPCustomImagesSource : public content::URLDataSource {
 public:
  NTPCustomImagesSource(NTPCustomBackgroundImagesService* service,
      std::unique_ptr<image_fetcher::ImageDecoder> image_decoder);
  ~NTPCustomImagesSource() override;

  NTPCustomImagesSource(const NTPCustomImagesSource&) = delete;
  NTPCustomImagesSource& operator=(const NTPCustomImagesSource&) = delete;

 private:
  // content::URLDataSource overrides:
  std::string GetSource() override;
  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;
  std::string GetMimeType(const std::string& path) override;

  void GetImageFile(const base::FilePath& image_file_path,
                    GotDataCallback callback);
  void OnGotImageFile(GotDataCallback callback,
                      absl::optional<std::string> input);
  void OnImageDecoded(GotDataCallback callback,
                      const gfx::Image& image);

  raw_ptr<NTPCustomBackgroundImagesService> service_ = nullptr;  // not owned
  std::unique_ptr<image_fetcher::ImageDecoder> image_decoder_;
  base::WeakPtrFactory<NTPCustomImagesSource> weak_factory_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_CUSTOM_IMAGES_SOURCE_H_
