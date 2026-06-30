/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar;

import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider.ControlsPosition;

/** Class responsible for managing the position (top, bottom) of the browsing mode toolbar. */
public class BraveToolbarPositionController {
    static int calculateStateTransition(
            boolean prefStateChanged,
            boolean ntpShowing,
            boolean tabSwitcherShowing,
            boolean isOmniboxFocused,
            boolean isFindInPageShowing,
            boolean isFormFieldFocusedWithKeyboardVisible,
            boolean doesUserPreferTopToolbar,
            @ControlsPosition int currentPosition) {
        // We want it to be shown on the NTP and tab switcher.
        return ToolbarPositionController.calculateStateTransition(
                prefStateChanged,
                false /*ntpShowing*/,
                false /*tabSwitcherShowing*/,
                isOmniboxFocused,
                isFindInPageShowing,
                isFormFieldFocusedWithKeyboardVisible,
                doesUserPreferTopToolbar,
                currentPosition);
    }
}
