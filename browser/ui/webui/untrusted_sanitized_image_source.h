// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_UNTRUSTED_SANITIZED_IMAGE_SOURCE_H_
#define BRAVE_BROWSER_UI_WEBUI_UNTRUSTED_SANITIZED_IMAGE_SOURCE_H_

#include <string>

#include "chrome/browser/ui/webui/sanitized_image_source.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

// Uses SanitizedImageSource for chrome-untrusted:// WebUIs
class UntrustedSanitizedImageSource : public SanitizedImageSource {
 public:
  using SanitizedImageSource::SanitizedImageSource;
  UntrustedSanitizedImageSource(const UntrustedSanitizedImageSource&) = delete;
  UntrustedSanitizedImageSource& operator=(const SanitizedImageSource&) =
      delete;

  // SanitizedImageSource:
  std::string GetSource() override;
  void StartDataRequest(
      const GURL& url,
      const content::WebContents::Getter& wc_getter,
      content::URLDataSource::GotDataCallback callback) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_UNTRUSTED_SANITIZED_IMAGE_SOURCE_H_
