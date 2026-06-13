/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_customization.theme;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.ui.base.WindowAndroid;

/**
 * Brave override for NtpCustomizationPromoManager. Suppresses the NTP theme customization IPH,
 * which crashes in debug builds when the anchor view is not yet attached to a window at the time
 * the async IPH callback fires.
 */
@NullMarked
public class BraveNtpCustomizationPromoManager {
    public static boolean canShowCustomizationIph(
            Tab tab, WindowAndroid windowAndroid, boolean isTablet) {
        return false;
    }
}
