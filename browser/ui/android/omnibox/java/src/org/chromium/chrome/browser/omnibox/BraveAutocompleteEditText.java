/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox;

import android.content.Context;
import android.util.AttributeSet;
import android.view.accessibility.AccessibilityEvent;

public class BraveAutocompleteEditText extends AutocompleteEditText {
    public BraveAutocompleteEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void sendAccessibilityEventUnchecked(AccessibilityEvent event) {
        try {
            super.sendAccessibilityEventUnchecked(event);
        } catch (IllegalStateException ignored) {
            // Ignore the exception as it means Accessibility is off
            // Details could be found here https://github.com/brave/brave-browser/issues/38189
        }
    }
}
