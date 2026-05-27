// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_news/untrusted_brave_news_image_source.h"

#include <string>
#include <utility>

#include "base/strings/strcat.h"
#include "content/public/common/url_constants.h"
#include "url/url_constants.h"

namespace {
constexpr char kBraveImageHost[] = "brave-image";
}  // namespace

std::string UntrustedBraveNewsImageSource::GetSource() {
  return base::StrCat({content::kChromeUIUntrustedScheme,
                       url::kStandardSchemeSeparator, kBraveImageHost, "/"});
}

void UntrustedBraveNewsImageSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    content::URLDataSource::GotDataCallback callback) {
  if (!url.is_valid() || !url.SchemeIs(content::kChromeUIUntrustedScheme)) {
    std::move(callback).Run(nullptr);
    return;
  }

  // BraveSanitizedImageSource compares against a hard-coded chrome:// URL.
  // Rewrite the scheme before delegating to it.
  GURL::Replacements replacements;
  replacements.SetSchemeStr(content::kChromeUIScheme);
  BraveSanitizedImageSource::StartDataRequest(
      url.ReplaceComponents(replacements), wc_getter, std::move(callback));
}
