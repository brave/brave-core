/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.messages.snackbar;

import android.app.Activity;
import android.view.ViewGroup;

import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.ui.base.WindowAndroid;

/** Brave's extension of SnackbarManager. */
@NullMarked
public class BraveSnackbarManager extends SnackbarManager {
    private static final String TAG = "BraveSnackbarManager";

    // Will be deleted in bytecode. Variable from the parent class will be used instead.
    @SuppressWarnings({"UnusedVariable", "HidingField"})
    protected @Nullable SnackbarView mView;

    private @Nullable Runnable mPendingClickCallback;

    public BraveSnackbarManager(
            Activity activity,
            ViewGroup snackbarParentView,
            @Nullable WindowAndroid windowAndroid) {
        super(activity, snackbarParentView, windowAndroid);
    }

    @Override
    public void showSnackbar(Snackbar snackbar) {
        super.showSnackbar(snackbar);

        tryMakeSnackbarClickable();
    }

    /**
     * Stores the callback to be executed when the snackbar is clicked.
     *
     * @param clickCallback Callback to execute when the snackbar is clicked.
     */
    public void makeSnackbarClickable(Runnable clickCallback) {
        if (clickCallback == null) {
            Log.e(TAG, "makeSnackbarClickable: clickCallback is null");
            return;
        }

        mPendingClickCallback = clickCallback;
    }

    private void tryMakeSnackbarClickable() {
        if (!isShowing()) {
            return;
        }

        if (mView instanceof BraveSnackbarView) {
            ((BraveSnackbarView) mView).makeClickable(mPendingClickCallback);
        }
    }

    /**
     * Sets custom text on the snackbar with title, page title, and URL.
     *
     * @param title The title text (e.g., "Get back to your most recent tab")
     * @param pageTitle The page title
     * @param url The URL to display
     */
    public void setCustomText(String title, String pageTitle, String url) {
        if (!isShowing()) {
            return;
        }

        if (mView instanceof BraveSnackbarView) {
            ((BraveSnackbarView) mView).setCustomText(title, pageTitle, url);
        }
    }
}
