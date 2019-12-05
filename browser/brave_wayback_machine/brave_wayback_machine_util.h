/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_UTIL_H_
#define BRAVE_BROWSER_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_UTIL_H_

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

void CheckWaybackMachineIfNeeded(content::WebContents* contents,
                                 int response_code);

bool IsWaybackMachineEnabled(content::BrowserContext* context);

#endif  // BRAVE_BROWSER_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_UTIL_H_
