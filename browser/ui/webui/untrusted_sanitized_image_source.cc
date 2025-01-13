// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/untrusted_sanitized_image_source.h"

#include <string>
#include <utility>

#include "base/strings/strcat.h"
#include "chrome/common/webui_url_constants.h"

std::string UntrustedSanitizedImageSource::GetSource() {
  return base::StrCat({content::kChromeUIUntrustedScheme,
                       url::kStandardSchemeSeparator,
                       chrome::kChromeUIImageHost, "/"});
}

void UntrustedSanitizedImageSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    content::URLDataSource::GotDataCallback callback) {
  if (!url.is_valid() || !url.SchemeIs(content::kChromeUIUntrustedScheme)) {
    std::move(callback).Run(nullptr);
    return;
  }

  // Change scheme to ChromeUIScheme for base class
  GURL::Replacements replacements;
  replacements.SetSchemeStr(content::kChromeUIScheme);

  SanitizedImageSource::StartDataRequest(url.ReplaceComponents(replacements),
                                         wc_getter, std::move(callback));
}
