/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_SOURCE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_SOURCE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/url_data_source.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class FilePath;
}  // namespace base

namespace ntp_background_images {

class NTPBackgroundImagesService;

// This serves background image data.
class NTPSponsoredImagesSource : public content::URLDataSource {
 public:
  explicit NTPSponsoredImagesSource(NTPBackgroundImagesService* service);

  ~NTPSponsoredImagesSource() override;

  NTPSponsoredImagesSource(const NTPSponsoredImagesSource&) = delete;
  NTPSponsoredImagesSource& operator=(const NTPSponsoredImagesSource&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesSourceTest, SponsoredImagesTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesSourceTest,
                           BasicSuperReferralDataTest);

  // content::URLDataSource overrides:
  std::string GetSource() override;
  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;
  std::string GetMimeType(const std::string& path) override;
  bool AllowCaching() override;

  base::FilePath GetLocalFilePathFor(const std::string& path);
  void GetImageFile(const base::FilePath& image_file_path,
                    GotDataCallback callback);
  void OnGotImageFile(GotDataCallback callback,
                      absl::optional<std::string> input);
  bool IsValidPath(const std::string& path) const;

  raw_ptr<NTPBackgroundImagesService> service_ = nullptr;  // not owned
  base::WeakPtrFactory<NTPSponsoredImagesSource> weak_factory_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_SOURCE_H_
