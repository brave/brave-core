/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import androidx.appcompat.app.AppCompatActivity;

import org.chromium.ui.base.WindowAndroid;

class BraveBookmarkBridge extends BookmarkBridge {
    // Overridden Chromium's BookmarkBridge.mNativeBookmarkBridge
    private long mNativeBookmarkBridge;
    private WindowAndroid mWindowAndroid;

    BraveBookmarkBridge(long nativeBookmarkBridge) {
        super(nativeBookmarkBridge);
    }

    @Override
    public void bookmarksImported(boolean isSuccess) {
        if (mWindowAndroid != null && mWindowAndroid.getContext().get() != null
                && mWindowAndroid.getContext().get() instanceof AppCompatActivity) {
            ((AppCompatActivity) mWindowAndroid.getContext().get()).runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    BraveBookmarkUtils.showBookmarkImportExportDialog(
                            (AppCompatActivity) mWindowAndroid.getContext().get(), true, isSuccess);
                }
            });
        }
    }

    @Override
    public void bookmarksExported(boolean isSuccess) {
        if (mWindowAndroid != null && mWindowAndroid.getContext().get() != null
                && mWindowAndroid.getContext().get() instanceof AppCompatActivity) {
            ((AppCompatActivity) mWindowAndroid.getContext().get()).runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    BraveBookmarkUtils.showBookmarkImportExportDialog(
                            (AppCompatActivity) mWindowAndroid.getContext().get(), false,
                            isSuccess);
                }
            });
        }
    }

    public void importBookmarks(WindowAndroid windowAndroid, String importFilePath) {
        mWindowAndroid = windowAndroid;
        BookmarkBridgeJni.get().importBookmarks(
                mNativeBookmarkBridge, BraveBookmarkBridge.this, windowAndroid, importFilePath);
    }

    public void exportBookmarks(WindowAndroid windowAndroid, String exportFilePath) {
        mWindowAndroid = windowAndroid;
        BookmarkBridgeJni.get().exportBookmarks(
                mNativeBookmarkBridge, BraveBookmarkBridge.this, windowAndroid, exportFilePath);
    }
}
