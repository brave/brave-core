/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/utf_string_conversions.h"
#include "base/strings/string_util.h"
#include "brave/common/url_constants.h"
#include "chrome/common/webui_url_constants.h"

namespace brave {
  base::string16 ReplaceChromeSchemeWithBrave(const base::string16 url) {
    base::string16 replace = url;
    const base::string16 kChromeUISchemeU16 =
        base::ASCIIToUTF16(content::kChromeUIScheme);
    if (base::StartsWith(replace, kChromeUISchemeU16,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      base::ReplaceFirstSubstringAfterOffset(&replace, 0ul, kChromeUISchemeU16, base::ASCIIToUTF16(content::kBraveUIScheme));
    }
    return replace;
  }
}
