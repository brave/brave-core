/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_RICH_MEDIA_SOURCE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_RICH_MEDIA_SOURCE_H_

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

// This serves Rich Media data.
class NTPSponsoredRichMediaSource : public content::URLDataSource {
 public:
  explicit NTPSponsoredRichMediaSource(NTPBackgroundImagesService* service);

  NTPSponsoredRichMediaSource(const NTPSponsoredRichMediaSource&) = delete;
  NTPSponsoredRichMediaSource& operator=(const NTPSponsoredRichMediaSource&) =
      delete;

  ~NTPSponsoredRichMediaSource() override;

 private:
  // content::URLDataSource:
  std::string GetSource() override;
  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;
  std::string GetMimeType(const GURL& url) override;
  bool AllowCaching() override;
  std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) override;

  std::optional<base::FilePath> MaybeGetFilePathForRequestPath(
      const std::string& request_path);
  void ReadFileCallback(GotDataCallback callback,
                        std::optional<std::string> input);

  raw_ptr<NTPBackgroundImagesService> service_ = nullptr;  // not owned.
  base::WeakPtrFactory<NTPSponsoredRichMediaSource> weak_factory_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_RICH_MEDIA_SOURCE_H_
