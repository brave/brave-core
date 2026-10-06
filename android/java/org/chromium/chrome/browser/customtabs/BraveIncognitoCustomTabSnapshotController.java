/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.customtabs;

import android.app.Activity;
import android.os.Build;
import android.view.WindowManager;

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
        extends IncognitoCustomTabSnapshotController
        implements BraveBrowserLockManager.ScreenshotModeObserver {
    private final Activity mActivity;
    private final Supplier<Boolean> mIsShowingIncognitoSupplier;

    BraveIncognitoCustomTabSnapshotController(
            Activity activity, Supplier<Boolean> isShowingIncognitoSupplier) {
        // Upstream's own constructor calls updateIncognitoTabSnapshotState() synchronously before
        // this line returns, i.e. before the fields below are assigned — the null check at the
        // top of that override below guards against that one premature call.
        super(activity, isShowingIncognitoSupplier);
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
