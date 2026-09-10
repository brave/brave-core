/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.customtabs;

import android.app.Activity;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.privacy.BraveBrowserLockManager;

import java.util.function.Supplier;

/**
 * Brave's extension for {@link IncognitoCustomTabSnapshotController}. Same conflict as {@link
 * org.chromium.chrome.browser.incognito.BraveIncognitoTabbedSnapshotController}, but for Custom Tab
 * activities — {@link BraveBrowserLockManager} applies FLAG_SECURE to every running activity, not
 * just {@code ChromeTabbedActivity}.
 */
@NullMarked
public class BraveIncognitoCustomTabSnapshotController
        extends IncognitoCustomTabSnapshotController {
    BraveIncognitoCustomTabSnapshotController(
            Activity activity, Supplier<Boolean> isShowingIncognitoSupplier) {
        super(activity, isShowingIncognitoSupplier);
    }

    @Override
    protected void updateIncognitoTabSnapshotState() {
        if (BraveBrowserLockManager.shouldForceSecureWindow()) return;
        super.updateIncognitoTabSnapshotState();
    }
}
