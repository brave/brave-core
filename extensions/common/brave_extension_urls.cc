/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/extensions/common/brave_extension_urls.h"

#include "base/strings/string_util.h"
#include "url/origin.h"

namespace extension_urls {

bool IsBraveProtectedUrl(const url::Origin& origin, base::StringPiece path) {
  return ((origin.DomainIs("sandbox.uphold.com") ||
           origin.DomainIs("uphold.com")) &&
          base::StartsWith(path, "/authorize/",
                           base::CompareCase::SENSITIVE)) ||
      (origin.DomainIs("api.uphold.com") &&
       base::StartsWith(path, "/oauth2/token",
                        base::CompareCase::SENSITIVE));
}

}  // namespace extension_urls
