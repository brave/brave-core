// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_UNTRUSTED_BRAVE_NEWS_IMAGE_SOURCE_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_UNTRUSTED_BRAVE_NEWS_IMAGE_SOURCE_H_

#include <string>

#include "brave/browser/ui/webui/brave_sanitized_image_source.h"

// Exposes BraveSanitizedImageSource at chrome-untrusted://brave-image so it
// can be loaded as `//brave-image` from chrome-untrusted:// pages.
class UntrustedBraveNewsImageSource : public BraveSanitizedImageSource {
 public:
  using BraveSanitizedImageSource::BraveSanitizedImageSource;
  UntrustedBraveNewsImageSource(const UntrustedBraveNewsImageSource&) = delete;
  UntrustedBraveNewsImageSource& operator=(
      const UntrustedBraveNewsImageSource&) = delete;

  // BraveSanitizedImageSource:
  std::string GetSource() override;
  void StartDataRequest(
      const GURL& url,
      const content::WebContents::Getter& wc_getter,
      content::URLDataSource::GotDataCallback callback) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_UNTRUSTED_BRAVE_NEWS_IMAGE_SOURCE_H_
