// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_RESTART_NAVIGATION_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_RESTART_NAVIGATION_H_

#include "ui/base/page_transition_types.h"

class GURL;

namespace content {
class NavigationHandle;
}  // namespace content

namespace brave_shields {

// Asynchronously starts a new navigation to `url` that copies the parameters
// of `navigation_handle`, using `transition` as its page transition. The
// caller is responsible for cancelling the navigation of `navigation_handle`.
void RestartNavigationWithURL(content::NavigationHandle& navigation_handle,
                              const GURL& url,
                              ui::PageTransition transition);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_RESTART_NAVIGATION_H_
