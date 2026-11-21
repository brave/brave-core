// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/browser/url_navigation_util.h"

#include "content/public/common/url_constants.h"
#include "url/gurl.h"

namespace gestures {

bool IsGestureAllowedToNavigateToUrl(const GURL& url) {
  return url.is_valid() &&
         (url.SchemeIsHTTPOrHTTPS() || url.SchemeIs(content::kBraveUIScheme) ||
          url.IsAboutBlank());
}

bool IsPageSuppliedUrlAllowedToOpen(const GURL& url) {
  return url.is_valid() && url.SchemeIsHTTPOrHTTPS();
}

}  // namespace gestures
