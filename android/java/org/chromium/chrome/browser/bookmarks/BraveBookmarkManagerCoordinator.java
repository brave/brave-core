/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.content.ComponentName;
import android.content.Context;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.ui.base.ActivityWindowAndroid;

public class BraveBookmarkManagerCoordinator extends BookmarkManagerCoordinator {
    // Overridden Chromium's BookmarkManagerCoordinator.mMediator
    private BookmarkManagerMediator mMediator;

    public BraveBookmarkManagerCoordinator(
            Context context,
            ComponentName openBookmarkComponentName,
            boolean isDialogUi,
            SnackbarManager snackbarManager,
            Profile profile,
            BookmarkUiPrefs bookmarkUiPrefs) {
        super(
                context,
                openBookmarkComponentName,
                isDialogUi,
                snackbarManager,
                profile,
                bookmarkUiPrefs);
    }

    public void setWindow(ActivityWindowAndroid window) {
        if (mMediator instanceof BraveBookmarkManagerMediator) {
            ((BraveBookmarkManagerMediator) mMediator).setWindow(window);
        }
    }
}
