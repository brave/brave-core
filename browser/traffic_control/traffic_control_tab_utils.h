// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_TAB_UTILS_H_
#define BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_TAB_UTILS_H_

#include "ui/base/page_transition_types.h"

class GURL;

namespace content {
class WebContents;
}  // namespace content

namespace traffic_control {

// Returns true if |web_contents| is an empty/new tab that can be closed after
// Traffic Control opens a replacement tab (about:blank or NTP, and no back
// history).
bool IsDiscardableEmptyTab(content::WebContents* web_contents);

// True for omnibox / address-bar navigations. These are excluded from the
// same-schemeful-site skip so typing a matching URL can still re-route.
bool IsOmniboxNavigation(ui::PageTransition transition);

// True when navigating from |from| into |to| crosses a schemeful-site boundary
// (or |from| is empty/NTP/about:blank). Same-site navigations return false.
bool CrossesSchemefulSiteBoundary(const GURL& from, const GURL& to);

}  // namespace traffic_control

#endif  // BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_TAB_UTILS_H_
