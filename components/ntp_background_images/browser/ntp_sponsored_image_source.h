/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_IMAGE_SOURCE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_IMAGE_SOURCE_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/url_data_source.h"

class GURL;

namespace base {
class FilePath;
}  // namespace base

namespace ntp_background_images {

class NTPBackgroundImagesService;

// This class is responsible for provided sponsored image content from the file
// system to the new tab page.

class NTPSponsoredImageSource final : public content::URLDataSource {
 public:
  explicit NTPSponsoredImageSource(
      NTPBackgroundImagesService* background_images_service);

  NTPSponsoredImageSource(const NTPSponsoredImageSource&) = delete;
  NTPSponsoredImageSource& operator=(const NTPSponsoredImageSource&) = delete;

  ~NTPSponsoredImageSource() override;

  // content::URLDataSource:
  std::string GetSource() override;
  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;
  std::string GetMimeType(const GURL& url) override;
  bool AllowCaching() override;

 private:
  void ReadFileCallback(GotDataCallback callback,
                        std::optional<std::string> input);

  void AllowAccess(const base::FilePath& file_path, GotDataCallback callback);
  void DenyAccess(GotDataCallback callback);

  const raw_ptr<NTPBackgroundImagesService>
      background_images_service_;  // Not owned.

  base::WeakPtrFactory<NTPSponsoredImageSource> weak_factory_{this};
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_IMAGE_SOURCE_H_
