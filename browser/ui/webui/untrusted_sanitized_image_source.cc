// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/untrusted_sanitized_image_source.h"

#include <string>
#include <utility>

#include "base/strings/strcat.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/common/webui_url_constants.h"

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#endif

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

std::string UntrustedSanitizedImageSource::GetAccessControlAllowOriginForOrigin(
    const std::string& origin) {
  const std::string origin_url = base::StrCat({origin, "/"});
  if (origin_url == kAIChatUntrustedConversationUIURL) {
    return origin;
  }
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  if (origin_url == kUntrustedNftURL || origin_url == kUntrustedMarketURL) {
    return origin;
  }
#endif
  return SanitizedImageSource::GetAccessControlAllowOriginForOrigin(origin);
}
