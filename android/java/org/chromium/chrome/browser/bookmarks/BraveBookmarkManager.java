/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.Manifest;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.os.Environment;
import android.provider.OpenableColumns;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.base.WindowAndroid;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class BraveBookmarkManager extends BookmarkManager implements BraveBookmarkDelegate {
    private ActivityWindowAndroid mWindowAndroid;
    private BookmarkModel mBookmarkModel;
    private Context mContext;
    private static final String TAG = "BraveBookmarkManager";

    public BraveBookmarkManager(Context context, ComponentName openBookmarkComponentName,
            boolean isDialogUi, boolean isIncognito, SnackbarManager snackbarManager) {
        super(context, openBookmarkComponentName, isDialogUi, isIncognito, snackbarManager);
    }

    public void setWindow(ActivityWindowAndroid window) {
        mWindowAndroid = window;
    }

    @Override
    public void importBookmarks() {
        if (mWindowAndroid == null || mContext == null) {
            return;
        }

        if (DeviceFormFactor.isNonMultiDisplayContextOnTablet(mContext)) {
            doImportBookmarks();
        } else {
            if (mWindowAndroid.hasPermission(Manifest.permission.READ_EXTERNAL_STORAGE)) {
                doImportBookmarks();
            } else {
                mWindowAndroid.requestPermissions(
                        new String[] {Manifest.permission.READ_EXTERNAL_STORAGE},
                        (permissions, grantResults) -> {
                            if (grantResults.length >= 1
                                    && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                                doImportBookmarks();
                            }
                        });
            }
        }
    }

    private void doImportBookmarks() {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("text/html");
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        if (mWindowAndroid.showIntent(
                    Intent.createChooser(intent, "R.string.import_bookmarks_select_file"),
                    new WindowAndroid.IntentCallback() {
                        @Override
                        public void onIntentCompleted(int resultCode, Intent results) {
                            if (resultCode == Activity.RESULT_OK && results != null
                                    && results.getData() != null) {
                                try {
                                    Cursor cursor = mContext.getContentResolver().query(
                                            results.getData(), null, null, null, null);
                                    int nameIndex =
                                            cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                                    cursor.moveToFirst();
                                    String name = cursor.getString(nameIndex);
                                    File file = new File(mContext.getFilesDir(), name);
                                    InputStream inputStream =
                                            mContext.getContentResolver().openInputStream(
                                                    results.getData());
                                    FileOutputStream outputStream = new FileOutputStream(file);
                                    int read = 0;
                                    int maxBufferSize = 1 * 1024 * 1024;
                                    int bytesAvailable = inputStream.available();
                                    int bufferSize = Math.min(bytesAvailable, maxBufferSize);
                                    byte[] buffers = new byte[bufferSize];
                                    int k;
                                    while ((k = inputStream.read(buffers)) != -1) {
                                        read = k;
                                        outputStream.write(buffers, 0, read);
                                    }
                                    inputStream.close();
                                    outputStream.close();
                                    mBookmarkModel.importBookmarks(mWindowAndroid, file.getPath());

                                } catch (Exception e) {
                                    Log.e(TAG, "doImportBookmarks:" + e.getMessage());
                                }
                            }
                        }
                    },
                    null))
            return;
    }

    @Override
    public void exportBookmarks() {
        if (mWindowAndroid == null || mContext == null) {
            return;
        }

        if (DeviceFormFactor.isNonMultiDisplayContextOnTablet(mContext)) {
            doExportBookmarks();
        } else {
            if (mWindowAndroid.hasPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
                doExportBookmarks();
            } else {
                mWindowAndroid.requestPermissions(
                        new String[] {Manifest.permission.WRITE_EXTERNAL_STORAGE},
                        (permissions, grantResults) -> {
                            if (grantResults.length >= 1
                                    && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                                doExportBookmarks();
                            }
                        });
            }
        }
    }

    private void doExportBookmarks() {
        File downloadDir =
                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
        int num = 1;
        String exportFileName = "bookmarks.html";
        File file = new File(downloadDir, exportFileName);
        while (file.exists()) {
            exportFileName = "bookmarks (" + (num++) + ").html";
            file = new File(downloadDir, exportFileName);
        }
        mBookmarkModel.exportBookmarks(mWindowAndroid, file.getPath());
    }
}
