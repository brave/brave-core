/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.screenshot_protection;

import android.app.Activity;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.NullableObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.privacy.BraveBrowserLockManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

/**
 * Brave's extension for {@link ScreenshotProtectionController} — the enterprise-policy-gated
 * counterpart to {@link
 * org.chromium.chrome.browser.incognito.BraveIncognitoTabbedSnapshotController}. Same conflict:
 * upstream's {@code updateScreenshotProtectionState()} unconditionally clears FLAG_SECURE whenever
 * its own tab/policy-based decision doesn't require it, which otherwise fights with {@link
 * BraveBrowserLockManager}'s own FLAG_SECURE forcing for the "Entire application" lock + "Prevent
 * capture" combination. This method is normally {@code private} upstream; a one-line patch makes it
 * {@code protected} so it can be overridden here.
 */
@NullMarked
public class BraveScreenshotProtectionController extends ScreenshotProtectionController {
    public BraveScreenshotProtectionController(
            Activity activity,
            NullableObservableSupplier<Tab> activityTabProvider,
            TabModelSelector tabModelSelector,
            boolean isCustomTab,
            MonotonicObservableSupplier<LayoutStateProvider> layoutStateProviderSupplier) {
        super(
                activity,
                activityTabProvider,
                tabModelSelector,
                isCustomTab,
                layoutStateProviderSupplier);
    }

    @Override
    protected void updateScreenshotProtectionState() {
        if (BraveBrowserLockManager.shouldForceSecureWindow()) return;
        super.updateScreenshotProtectionState();
    }
}
