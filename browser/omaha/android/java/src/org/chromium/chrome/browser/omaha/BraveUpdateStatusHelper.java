/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omaha;

import org.chromium.base.ContextUtils;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.build.annotations.NullMarked;

/**
 * Feeds Play Store update availability into the prefs Omaha would write, so upstream's update menu
 * item and badge work. Applied by {@link UpdateStatusProvider} on the next launch.
 */
@NullMarked
public class BraveUpdateStatusHelper {
    public static void setLatestVersionCode(int versionCode) {
        PostTask.postTask(
                TaskTraits.BEST_EFFORT_MAY_BLOCK,
                () -> {
                    String marketUrl =
                            "market://details?id="
                                    + ContextUtils.getApplicationContext().getPackageName();
                    // Matches the patched VersionNumberGetter.getCurrentlyUsedVersion(). The
                    // "0.0.0." prefix is needed as VersionNumber only parses a.b.c.d.
                    OmahaPrefUtils.getSharedPreferences()
                            .edit()
                            .putString(OmahaPrefUtils.PREF_LATEST_VERSION, "0.0.0." + versionCode)
                            .putString(OmahaPrefUtils.PREF_MARKET_URL, marketUrl)
                            .apply();
                });
    }

    public static void clearLatestVersion() {
        PostTask.postTask(
                TaskTraits.BEST_EFFORT_MAY_BLOCK,
                () ->
                        OmahaPrefUtils.getSharedPreferences()
                                .edit()
                                .remove(OmahaPrefUtils.PREF_LATEST_VERSION)
                                .apply());
    }
}
