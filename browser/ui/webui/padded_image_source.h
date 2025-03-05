// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_PADDED_IMAGE_SOURCE_H_
#define BRAVE_BROWSER_UI_WEBUI_PADDED_IMAGE_SOURCE_H_

#include <memory>
#include <string>

#include "chrome/browser/ui/webui/sanitized_image_source.h"

class Profile;

namespace network {
class SimpleURLLoader;
}  // namespace network

// This class is a drop in replacement for SanitizedImageSource that also allows
// use to download padded images from the Brave Private CDN.
class PaddedImageSource : public SanitizedImageSource {
 public:
  explicit PaddedImageSource(Profile* profile);

  // This constructor lets us pass mock dependencies for testing.
  PaddedImageSource(
      Profile* profile,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::unique_ptr<DataDecoderDelegate> delegate,
      std::string pcdn_domain);

  ~PaddedImageSource() override;

  // content::URLDataSource:
  std::string GetSource() override;
  void StartDataRequest(
      const GURL& url,
      const content::WebContents::Getter& wc_getter,
      content::URLDataSource::GotDataCallback callback) override;

 protected:
  void OnImageLoaded(std::unique_ptr<network::SimpleURLLoader> loader,
                     RequestAttributes request_attributes,
                     content::URLDataSource::GotDataCallback callback,
                     std::unique_ptr<std::string> body) override;

 private:
  const std::string pcdn_domain_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_PADDED_IMAGE_SOURCE_H_
