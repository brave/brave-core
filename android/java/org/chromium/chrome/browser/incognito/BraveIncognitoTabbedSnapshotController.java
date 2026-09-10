/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.incognito;

import android.app.Activity;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerChrome;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.privacy.BraveBrowserLockManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

import java.util.function.Supplier;

/**
 * Brave's extension for {@link IncognitoTabbedSnapshotController}. Upstream unconditionally clears
 * FLAG_SECURE whenever no incognito tab is showing (on every tab-model switch and Hub show/hide),
 * which otherwise fights with {@link BraveBrowserLockManager}'s own FLAG_SECURE forcing for the
 * "Entire application" lock + "Prevent capture" combination — the window would go unprotected as
 * soon as the user is on a regular tab. Defer to Brave's own decision instead.
 */
@NullMarked
public class BraveIncognitoTabbedSnapshotController extends IncognitoTabbedSnapshotController {
    BraveIncognitoTabbedSnapshotController(
            Activity activity,
            LayoutManagerChrome layoutManager,
            TabModelSelector tabModelSelector,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            Supplier<Boolean> isShowingIncognitoSupplier) {
        super(
                activity,
                layoutManager,
                tabModelSelector,
                activityLifecycleDispatcher,
                isShowingIncognitoSupplier);
    }

    @Override
    protected void updateIncognitoTabSnapshotState() {
        if (BraveBrowserLockManager.shouldForceSecureWindow()) return;
        super.updateIncognitoTabSnapshotState();
    }
}
