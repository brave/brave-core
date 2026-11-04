/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.webapps;

import org.chromium.base.BraveFeatureList;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.flags.ChromeFeatureList;

@NullMarked
public class BraveAppInstallMenuHandlerUtils {
    /** Whether to skip the Install/Create shortcut bottom sheet. */
    public static boolean shouldAddToHomescreenOnly() {
        return ChromeFeatureList.isEnabled(
                BraveFeatureList.BRAVE_ANDROID_MERGE_INSTALL_PWA_AND_CREATE_SHORTCUT);
    }
}
