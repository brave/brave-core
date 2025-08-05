/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.MediaStore;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import org.jni_zero.CalledByNative;
import org.jni_zero.NativeMethods;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.ui.base.WindowAndroid;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

class BraveBookmarkBridge extends BookmarkBridge {
    // Overridden Chromium's BookmarkBridge.mNativeBookmarkBridge
    private static final String TAG = "BraveBookmarkBridge";
    private long mNativeBookmarkBridge;
    private WindowAndroid mWindowAndroid;
    private String mExportFilePath;

    BraveBookmarkBridge(long nativeBookmarkBridge, Profile profile) {
        super(nativeBookmarkBridge, profile);
    }

    @CalledByNative
    public void bookmarksImported(boolean isSuccess) {
        if (mWindowAndroid != null && mWindowAndroid.getContext().get() != null
                && mWindowAndroid.getContext().get() instanceof AppCompatActivity) {
            ((AppCompatActivity) mWindowAndroid.getContext().get()).runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    BraveBookmarkUtils.showBookmarkImportExportDialog(
                            (AppCompatActivity) mWindowAndroid.getContext().get(), true, isSuccess,
                            null);
                }
            });
        }
    }

    @CalledByNative
    public void bookmarksExported(final boolean isSuccess) {
        if (mWindowAndroid == null
                || mWindowAndroid.getContext().get() == null
                || !(mWindowAndroid.getContext().get() instanceof AppCompatActivity)) {
            return;
        }
        if (Build.VERSION.SDK_INT == Build.VERSION_CODES.Q && isSuccess) {
            PostTask.postTask(
                    TaskTraits.BEST_EFFORT_MAY_BLOCK,
                    () -> {
                        ThreadUtils.assertOnBackgroundThread();
                        boolean result = isSuccess;
                        try {
                            moveInternalFileToDownloads(
                                    mWindowAndroid.getContext().get(), mExportFilePath);
                        } catch (IOException e) {
                            Log.e(TAG, "Failed to move file to public Downloads", e);
                            result = false;
                        }
                        showExportedDialog(result);
                    });
        } else {
            showExportedDialog(isSuccess);
        }
    }

    private void moveInternalFileToDownloads(
            @NonNull Context context, @NonNull String internalFileName) throws IOException {
        ContentResolver resolver = context.getContentResolver();
        // Open internal file to copy to the MediaStore URI
        File internalFile = new File(internalFileName);

        ContentValues values = new ContentValues();
        values.put(MediaStore.MediaColumns.DISPLAY_NAME, internalFile.getName());
        values.put(MediaStore.MediaColumns.MIME_TYPE, "text/html");
        values.put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS);

        // Insert ContentValues into MediaStore to get a new URI
        Uri uri = resolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values);
        if (uri == null) {
            throw new IOException("Failed to create file in MediaStore");
        }

        try (InputStream inputStream = new FileInputStream(internalFile);
                OutputStream outputStream = resolver.openOutputStream(uri)) {

            if (outputStream == null) {
                resolver.delete(uri, null, null);
                throw new IOException("Failed to open output stream");
            }

            byte[] buffer = new byte[1024];
            int bytesRead;
            while ((bytesRead = inputStream.read(buffer)) != -1) {
                outputStream.write(buffer, 0, bytesRead);
            }

            if (internalFile.exists() && !internalFile.delete()) {
                throw new IOException("Failed to delete internal file after copying");
            }
            mExportFilePath =
                    new File(Environment.DIRECTORY_DOWNLOADS, internalFile.getName()).getPath();
        } catch (Exception e) {
            // Clean up the MediaStore entry on failure
            resolver.delete(uri, null, null);
            throw new IOException("Failed to copy or delete file", e);
        }
    }

    private void showExportedDialog(boolean isSuccess) {
        ((AppCompatActivity) mWindowAndroid.getContext().get())
                .runOnUiThread(
                        new Runnable() {
                            @Override
                            public void run() {
                                BraveBookmarkUtils.showBookmarkImportExportDialog(
                                        (AppCompatActivity) mWindowAndroid.getContext().get(),
                                        false,
                                        isSuccess,
                                        mExportFilePath);
                            }
                        });
    }

    public void importBookmarks(WindowAndroid windowAndroid, String importFilePath) {
        mWindowAndroid = windowAndroid;
        BraveBookmarkBridgeJni.get().importBookmarks(
                mNativeBookmarkBridge, BraveBookmarkBridge.this, windowAndroid, importFilePath);
    }

    public void exportBookmarks(WindowAndroid windowAndroid, String exportFilePath) {
        mWindowAndroid = windowAndroid;
        mExportFilePath = exportFilePath;
        BraveBookmarkBridgeJni.get().exportBookmarks(
                mNativeBookmarkBridge, BraveBookmarkBridge.this, windowAndroid, exportFilePath);
    }

    @NativeMethods
    public interface Natives {
        void importBookmarks(long nativeBraveBookmarkBridge, BraveBookmarkBridge caller,
                WindowAndroid window, String importFilePath);
        void exportBookmarks(long nativeBraveBookmarkBridge, BraveBookmarkBridge caller,
                WindowAndroid window, String exportFilePath);
    }
}
