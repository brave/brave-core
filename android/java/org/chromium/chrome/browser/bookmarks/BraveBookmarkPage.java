/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.content.ComponentName;

import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.bookmarks.BookmarkManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.NativePageHost;

public class BraveBookmarkPage extends BookmarkPage {
    private BookmarkManager mManager;

    public BraveBookmarkPage(ComponentName componentName, SnackbarManager snackbarManager,
            boolean isIncognito, NativePageHost host) {
        super(componentName, snackbarManager, isIncognito, host);
        if (mManager != null && mManager instanceof BraveBookmarkManager) {
            ((BraveBookmarkManager) mManager)
                    .setWindow(BraveActivity.getChromeTabbedActivity().getWindowAndroid());
        }
    }
}
