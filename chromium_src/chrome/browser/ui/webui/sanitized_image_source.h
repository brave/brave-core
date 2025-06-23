// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SANITIZED_IMAGE_SOURCE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SANITIZED_IMAGE_SOURCE_H_

// This juggling is required so we can still friend the Brave implementation
// without clashing with the
// #define SanitizedImageSource SanitizedImageSource_Chromium
class SanitizedImageSource;
using SanitizedImageSource_BraveImpl = SanitizedImageSource;

// We override SanitizedImageSource to add support for unpadding images from the
// Brave Private CDN.
#define SanitizedImageSource SanitizedImageSource_Chromium
#define OnImageLoaded                    \
  OnImageLoaded_Chromium() {}            \
  friend SanitizedImageSource_BraveImpl; \
  virtual void OnImageLoaded

#include <chrome/browser/ui/webui/sanitized_image_source.h>  // IWYU pragma: export

#undef OnImageLoaded
#undef SanitizedImageSource

class SanitizedImageSource : public SanitizedImageSource_Chromium {
 public:
  using SanitizedImageSource_Chromium::SanitizedImageSource_Chromium;
  ~SanitizedImageSource() override;

  void set_pcdn_domain_for_testing(std::string pcdn_domain) {
    pcdn_domain_ = std::move(pcdn_domain);
  }

 private:
  void OnImageLoaded(std::unique_ptr<network::SimpleURLLoader> loader,
                     RequestAttributes request_attributes,
                     content::URLDataSource::GotDataCallback callback,
                     std::unique_ptr<std::string> body) override;

  std::string pcdn_domain_;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SANITIZED_IMAGE_SOURCE_H_
