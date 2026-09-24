// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_UNTRUSTED_SANITIZED_IMAGE_SOURCE_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_UNTRUSTED_SANITIZED_IMAGE_SOURCE_H_

#include <string>

#include "brave/ios/browser/ui/webui/sanitized_image_source.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "url/gurl.h"

// Uses SanitizedImageSource for chrome-untrusted:// WebUIs
class UntrustedSanitizedImageSource : public SanitizedImageSource {
 public:
  using SanitizedImageSource::SanitizedImageSource;
  UntrustedSanitizedImageSource(const UntrustedSanitizedImageSource&) = delete;
  UntrustedSanitizedImageSource& operator=(const SanitizedImageSource&) =
      delete;

  // SanitizedImageSource:
  std::string GetSource() const override;
  void StartDataRequest(
      std::string_view path,
      web::URLDataSourceIOS::GotDataCallback callback) override;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_UNTRUSTED_SANITIZED_IMAGE_SOURCE_H_
