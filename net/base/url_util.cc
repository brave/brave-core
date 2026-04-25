/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/base/url_util.h"

#include "net/base/url_util.h"
#include "url/url_constants.h"

namespace net {

bool IsHTTPSOrLocalhostURL(const GURL& url) {
  return url.is_valid() &&
         (url.SchemeIs(url::kHttpsScheme) ||
          (IsLocalhost(url) && url.SchemeIs(url::kHttpScheme)));
}

}  // namespace net
