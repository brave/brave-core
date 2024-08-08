/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks;

import org.chromium.chrome.browser.flags.ChromeFeatureList;

public final class BraveReturnToChromeUtil {
    public static boolean shouldShowTabSwitcher(final long lastTimeMillis) {
        if (!ChromeFeatureList.sStartSurfaceReturnTime.isEnabled()) {
            return false;
        }

        return ReturnToChromeUtil.shouldShowTabSwitcher(lastTimeMillis);
    }
}
