/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_UTILS_H_
#define BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_UTILS_H_

class BrowserWindowInterface;

// Returns a value indicating whether Focus Mode is supported for the specified
// browser window.
bool BrowserSupportsFocusMode(const BrowserWindowInterface* browser);

// Returns true if Focus Mode is both supported and currently active for the
// specified browser.
bool IsFocusModeEnabled(const BrowserWindowInterface* browser);

#endif  // BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_UTILS_H_
