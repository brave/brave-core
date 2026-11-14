/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.incognito;

import android.app.Activity;
import android.os.Build;
import android.view.WindowManager;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerChrome;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.privacy.BraveBrowserLockManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

import java.util.function.Supplier;

/**
 * Brave's extension for {@link IncognitoTabbedSnapshotController}. Upstream's own decision is
 * driven by {@link org.chromium.chrome.browser.flags.ChromeFeatureList#sIncognitoScreenshot}, which
 * is only refreshed at native init and so stays stale across a screenshot-mode change until the
 * next relaunch. Fully replace it with {@link
 * BraveBrowserLockManager#shouldSecureForIncognitoVisibility} instead of only overriding the
 * EVERYTHING case, so PRIVATE_TABS_ONLY also takes effect immediately.
 */
@NullMarked
public class BraveIncognitoTabbedSnapshotController extends IncognitoTabbedSnapshotController
        implements BraveBrowserLockManager.ScreenshotModeObserver {
    private final Activity mActivity;
    private final Supplier<Boolean> mIsShowingIncognitoSupplier;

    BraveIncognitoTabbedSnapshotController(
            Activity activity,
            LayoutManagerChrome layoutManager,
            TabModelSelector tabModelSelector,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            Supplier<Boolean> isShowingIncognitoSupplier) {
        // Upstream's own constructor registers a supplier observer with
        // addSyncObserverAndPostIfNonNull, which (per ObservableSupplierImpl) posts rather than
        // calls synchronously, so it cannot reach updateIncognitoTabSnapshotState() before the
        // fields below are assigned — the null check at the top of that override is just a cheap
        // defensive guard against that assumption ever changing upstream.
        super(
                activity,
                layoutManager,
                tabModelSelector,
                activityLifecycleDispatcher,
                isShowingIncognitoSupplier);
        mActivity = activity;
        mIsShowingIncognitoSupplier = isShowingIncognitoSupplier;
        BraveBrowserLockManager.addScreenshotModeObserver(activity, this);
        updateIncognitoTabSnapshotState();
    }

    @Override
    public void onScreenshotModeChanged() {
        updateIncognitoTabSnapshotState();
    }

    @Override
    protected void updateIncognitoTabSnapshotState() {
        if (mActivity == null) return;

        boolean secure =
                BraveBrowserLockManager.shouldSecureForIncognitoVisibility(
                        mIsShowingIncognitoSupplier.get());

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            mActivity.setRecentsScreenshotEnabled(!secure);
        }

        WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
        boolean currentlySecure =
                (attributes.flags & WindowManager.LayoutParams.FLAG_SECURE)
                        == WindowManager.LayoutParams.FLAG_SECURE;
        if (currentlySecure == secure) return;

        if (secure) {
            mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_SECURE);
        } else {
            mActivity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_SECURE);
        }
    }
}
