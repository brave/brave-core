// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_TAB_UTILS_H_
#define BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_TAB_UTILS_H_

#include "ui/base/page_transition_types.h"

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

// True while |web_contents| is a newly created tab that has not committed a
// real page yet. Such a tab only inherits its container from the opener, so it
// is excluded from the same-site skip and rules apply to it. Cloned
// (duplicated) and restored tabs are not considered new: they keep their own
// container. Tabs that keep a live window.opener are also excluded, since
// re-routing them would sever script access.
bool IsNewTabNavigation(content::WebContents* web_contents);

}  // namespace traffic_control

#endif  // BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_TAB_UTILS_H_
