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
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.OpenableColumns;

import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.WindowAndroid;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class BraveBookmarkManager extends BookmarkManager implements BraveBookmarkDelegate {
    private ActivityWindowAndroid mWindowAndroid;

    // Overridden Chromium's BookmarkManager.mBookmarkModel
    private BookmarkModel mBookmarkModel;

    // Overridden Chromium's BookmarkManager.mContext
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

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
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
                            } else {
                                Log.e("tapan", "ask permission dialog");
                            }
                        });
            }
        }
    }

    private void doImportBookmarks() {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("text/html");
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        if (mWindowAndroid.showIntent(Intent.createChooser(intent,
                                              mContext.getResources().getString(
                                                      R.string.import_bookmarks_select_file)),
                    new WindowAndroid.IntentCallback() {
                        @Override
                        public void onIntentCompleted(int resultCode, Intent results) {
                            if (resultCode == Activity.RESULT_OK && results != null
                                    && results.getData() != null) {
                                PostTask.postTask(TaskTraits.USER_VISIBLE_MAY_BLOCK,
                                        () -> { importFileSelected(results.getData()); });
                            }
                        }
                    },
                    null))
            return;
    }

    private void importFileSelected(Uri resultData) {
        try {
            Cursor cursor = mContext.getContentResolver().query(resultData, null, null, null, null);
            int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
            cursor.moveToFirst();
            String name = cursor.getString(nameIndex);
            File file = new File(mContext.getFilesDir(), name);
            try (InputStream inputStream =
                            mContext.getContentResolver().openInputStream(resultData);
                    FileOutputStream outputStream = new FileOutputStream(file)) {
                int maxBufferSize = 1 * 1024 * 1024;
                int bytesAvailable = inputStream.available();
                int bufferSize = Math.min(bytesAvailable, maxBufferSize);
                byte[] buffers = new byte[bufferSize];
                int byteRead;
                while ((byteRead = inputStream.read(buffers)) != -1) {
                    outputStream.write(buffers, 0, byteRead);
                }
                ((AppCompatActivity) mWindowAndroid.getContext().get())
                        .runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if (mBookmarkModel instanceof BraveBookmarkModel) {
                                    ((BraveBookmarkModel) mBookmarkModel)
                                            .importBookmarks(mWindowAndroid, file.getPath());
                                }
                            }
                        });
            } catch (IOException e) {
                Log.e(TAG, "doImportBookmarks IOException:" + e.getMessage());
            }

        } catch (Exception e) {
            Log.e(TAG, "doImportBookmarks:" + e.getMessage());
        }
    }

    @Override
    public void exportBookmarks() {
        if (mWindowAndroid == null || mContext == null) {
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
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
        if (mBookmarkModel instanceof BraveBookmarkModel) {
            ((BraveBookmarkModel) mBookmarkModel).exportBookmarks(mWindowAndroid, file.getPath());
        }
    }
}
