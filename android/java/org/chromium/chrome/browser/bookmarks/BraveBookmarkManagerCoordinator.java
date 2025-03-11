/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.chrome.browser.price_tracking.PriceDropNotificationManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.ui.base.ActivityWindowAndroid;

public class BraveBookmarkManagerCoordinator extends BookmarkManagerCoordinator {
    // Overridden Chromium's BookmarkManagerCoordinator.mMediator
    private BookmarkManagerMediator mMediator;

    public BraveBookmarkManagerCoordinator(
            @NonNull Context context,
            boolean isDialogUi,
            @NonNull SnackbarManager snackbarManager,
            @NonNull Profile profile,
            @NonNull BookmarkUiPrefs bookmarkUiPrefs,
            @NonNull BookmarkOpener bookmarkOpener,
            @Nullable BookmarkManagerOpener bookmarkManagerOpener,
            @NonNull PriceDropNotificationManager priceDropNotificationManager) {
        super(
                context,
                isDialogUi,
                snackbarManager,
                profile,
                bookmarkUiPrefs,
                bookmarkOpener,
                bookmarkManagerOpener,
                priceDropNotificationManager);
    }

    public void setWindow(ActivityWindowAndroid window) {
        if (mMediator instanceof BraveBookmarkManagerMediator) {
            ((BraveBookmarkManagerMediator) mMediator).setWindow(window);
        }
    }
}
