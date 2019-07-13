/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/extensions/common/brave_extension_urls.h"

#include "base/strings/string_util.h"
#include "url/origin.h"

namespace extension_urls {

const char* BraveProtectedUrls::UpholdUrls[][2] = {
    {"https://sandbox.uphold.com", "/authorize/"},
    {"https://api.uphold.com", "/oauth2/token"},
};

bool BraveProtectedUrls::IsHiddenNetworkRequest(const url::Origin& origin,
                                                base::StringPiece path) {
  constexpr size_t url_count = base::size(UpholdUrls);
  for (size_t i=0; i < url_count; i++) {
    if (origin.DomainIs(UpholdUrls[i][0]) &&
        base::StartsWith(path, UpholdUrls[i][1],
                         base::CompareCase::SENSITIVE)) {
      return true;
    }
  }
  return false;
}

std::vector<const GURL> BraveProtectedUrls::ContentScriptWithheldUrls() {
  std::vector<const GURL> urls;

  constexpr size_t url_count = base::size(UpholdUrls);
  for (size_t i=0; i < url_count; i++) {
    std::string url(UpholdUrls[i][0]);
    url += UpholdUrls[i][1];
    urls.push_back(GURL(url));
  }

  return urls;
}

}  // namespace extension_urls
