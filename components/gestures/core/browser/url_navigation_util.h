// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_URL_NAVIGATION_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_URL_NAVIGATION_UTIL_H_

class GURL;

namespace gestures {

// `url` is a gesture action's configured target, typed or synced by the
// user, so `brave:` pages and `about:blank` are allowed alongside http/https.
bool IsGestureAllowedToNavigateToUrl(const GURL& url);

// `url` was hit-tested out of untrusted page content, so only http/https
// is allowed; privileged schemes like `brave:` must stay unreachable.
bool IsPageSuppliedUrlAllowedToOpen(const GURL& url);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_URL_NAVIGATION_UTIL_H_
