// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/untrusted_sanitized_image_source.h"

#include <string>
#include <utility>

#include "base/memory/ref_counted_memory.h"
#include "base/strings/strcat.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "ios/web/public/webui/url_data_source_ios.h"

namespace chrome {
inline constexpr char kChromeUIImageHost[] = "image";
inline constexpr char kChromeUntrustedUIImageURL[] =
    "chrome-untrusted://image/";

}  // namespace chrome

std::string UntrustedSanitizedImageSource::GetSource() const {
  return base::StrCat({kChromeUIUntrustedScheme, url::kStandardSchemeSeparator,
                       chrome::kChromeUIImageHost, "/"});
}

void UntrustedSanitizedImageSource::StartDataRequest(
    std::string_view path,
    web::URLDataSourceIOS::GotDataCallback callback) {
  GURL url =
      GURL(chrome::kChromeUntrustedUIImageURL).GetWithEmptyPath().Resolve(path);

  if (!url.is_valid() || !url.SchemeIs(kChromeUIUntrustedScheme)) {
    std::move(callback).Run(nullptr);
    return;
  }

  // Change scheme to ChromeUIScheme for base class
  GURL::Replacements replacements;
  replacements.SetSchemeStr(kChromeUIScheme);

  SanitizedImageSource::StartDataRequest(
      url.ReplaceComponents(replacements).path(), std::move(callback));
}
