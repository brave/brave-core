/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.util;

import android.app.Dialog;
import android.content.Context;
import android.os.Environment;
import android.text.TextUtils;
import android.widget.Toast;

import org.chromium.base.ContextUtils;
import org.chromium.base.FileUtils;
import org.chromium.base.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class BraveDbUtil {
    private static final String TAG = "BraveDbUtil";
    private static final String PUBLISHER_INFO_DB = "publisher_info_db";
    private static final String REWARDS_DB_SRC_DIR = "app_chrome/Default";
    private static final String REWARDS_DB_DST_DIR = "rewards";
    private static final String PREF_PERFORM_DB_EXPORT_ON_START = "perform_db_export_on_start";
    private static final String PREF_PERFORM_DB_IMPORT_ON_START = "perform_db_import_on_start";
    private static final String PREF_DB_IMPORT_FILE = "db_import_file";

    private String mRewardsSrc;
    private String mRewardsDst;
    private String mRewardsDstDir;

    private static BraveDbUtil sInstance;

    private BraveDbUtil() {
    }

    public static BraveDbUtil getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = new BraveDbUtil();
        return sInstance;
    }

    public void ExportRewardsDb(Dialog dlg) {
        Context context = ContextUtils.getApplicationContext();
        mRewardsSrc = context.getApplicationInfo().dataDir + File.separator + REWARDS_DB_SRC_DIR + File.separator
                + PUBLISHER_INFO_DB;

        mRewardsDstDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
                .getAbsolutePath() + File.separator + REWARDS_DB_DST_DIR;

        SimpleDateFormat dateFormat =
                new SimpleDateFormat("-yyyy-MM-dd-HHmmss", Locale.getDefault());
        mRewardsDst = mRewardsDstDir + File.separator + PUBLISHER_INFO_DB + dateFormat.format(new Date());

        copyRewardsDbThread(dlg, false);
    }

    public void ImportRewardsDb(Dialog dlg, String fileToImport) {
        mRewardsDst = importDestinationPath();

        mRewardsSrc = fileToImport.isEmpty()
                ? Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath()
                        + File.separator + REWARDS_DB_DST_DIR + File.separator + PUBLISHER_INFO_DB
                : fileToImport;

        mRewardsDstDir = "";

        copyRewardsDbThread(dlg, true);
    }

    private void copyRewardsDbThread(Dialog dlg, boolean isImport) {
        if (dlg != null)
            dlg.show();

        if (isImport) {
            File file = new File(mRewardsDst);
            boolean unused_file_deleted = file.delete();
            File file_journal = new File(mRewardsDst + "-journal");
            boolean unused_file_journal_deleted = file_journal.delete();
        }

        String erroMsg = "";

        // Create dest dir if necessary
        if (!TextUtils.isEmpty(mRewardsDstDir)) {
            if (!createDstDir(mRewardsDstDir)) {
                erroMsg = "Failed to create destination directory for database operation";
            }
        }

        // No errors so far: copy the file
        if (TextUtils.isEmpty(erroMsg)) {
            boolean succeeded = copyFile(mRewardsSrc, mRewardsDst);
            if (!succeeded) {
                erroMsg = "Failed to copy database file";
            }
        }

        // Update UI
        final String msg =
                !TextUtils.isEmpty(erroMsg)
                        ? erroMsg
                        : "Database successfully " + (isImport ? "imported" : "exported");
        if (dlg != null) dlg.dismiss();
        Context context = ContextUtils.getApplicationContext();
        Toast.makeText(context, msg, Toast.LENGTH_LONG).show();
        if (isImport) {
            File file = new File(mRewardsSrc);
            file.delete();
        }
    }

    private boolean copyFile(String src, String dst) {
        boolean succeeded = false;
        try {
            InputStream in = new FileInputStream(src);
            FileUtils.copyStreamToFile(in, new File(dst));
            succeeded = true;
            in.close();
        } catch (IOException e) {
            Log.e(TAG, "Error on copying database file (" + src + " -> " + dst +  "): " + e);
        }
        return succeeded;
    }

    private boolean createDstDir(String dir) {
        boolean succeeded = false;
        File dst = new File(dir);
        try {
            if (!dst.exists()) {
                succeeded = dst.mkdirs();
            } else {
                succeeded = true;
            }
        } catch (SecurityException e) {
            succeeded = false;
        }
        return succeeded;
    }

    public boolean performDbExportOnStart() {
        if (ContextUtils.getAppSharedPreferences() == null)
            return false;
        return ContextUtils.getAppSharedPreferences().getBoolean(PREF_PERFORM_DB_EXPORT_ON_START, false);
    }

    public void setPerformDbExportOnStart(boolean value) {
        if (ContextUtils.getAppSharedPreferences() == null)
            return;
        ContextUtils.getAppSharedPreferences().edit().putBoolean(PREF_PERFORM_DB_EXPORT_ON_START, value).apply();
    }

    public boolean performDbImportOnStart() {
        if (ContextUtils.getAppSharedPreferences() == null)
            return false;
        return ContextUtils.getAppSharedPreferences().getBoolean(PREF_PERFORM_DB_IMPORT_ON_START, false);
    }

    public void setPerformDbImportOnStart(boolean value) {
        if (ContextUtils.getAppSharedPreferences() == null)
            return;
        ContextUtils.getAppSharedPreferences().edit().putBoolean(PREF_PERFORM_DB_IMPORT_ON_START, value).apply();
    }

    public String dbImportFile() {
        if (ContextUtils.getAppSharedPreferences() == null)
            return "";
        return ContextUtils.getAppSharedPreferences().getString(PREF_DB_IMPORT_FILE, "");
    }

    public void setDbImportFile(String file) {
        if (ContextUtils.getAppSharedPreferences() == null)
            return;
        ContextUtils.getAppSharedPreferences().edit().putString(PREF_DB_IMPORT_FILE, file).apply();
    }

    public boolean dbOperationRequested() {
        return performDbExportOnStart() || (performDbImportOnStart() && !dbImportFile().isEmpty());
    }

    public void cleanUpDbOperationRequest() {
        setPerformDbExportOnStart(false);
        setPerformDbImportOnStart(false);
        if (!dbImportFile().isEmpty()) {
            File file = new File(dbImportFile());
            file.delete();
        }
        setDbImportFile("");
    }

    public String importDestinationPath() {
        return ContextUtils.getApplicationContext().getApplicationInfo().dataDir + File.separator
                + REWARDS_DB_SRC_DIR + File.separator + PUBLISHER_INFO_DB;
    }

    public static String getTag() {
        return TAG;
    }
}
