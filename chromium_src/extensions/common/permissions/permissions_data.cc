/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/string_piece.h"
#include "extensions/common/permissions/permissions_data.h"
#include "url/origin.h"

namespace extensions {

bool IsBraveProtectedUrl(const GURL& url) {
  const url::Origin origin = url::Origin::Create(url);
  const base::StringPiece path = url.path_piece();
  return ((origin.DomainIs("sandbox.uphold.com") ||
           origin.DomainIs("uphold.com")) &&
          base::StartsWith(path, "/authorize/",
                           base::CompareCase::INSENSITIVE_ASCII)) ||
      (origin.DomainIs("api.uphold.com") &&
       base::StartsWith(path, "/oauth2/token",
                        base::CompareCase::INSENSITIVE_ASCII));
}

}  // namespace extensions

#include "../../../../../extensions/common/permissions/permissions_data.cc"
