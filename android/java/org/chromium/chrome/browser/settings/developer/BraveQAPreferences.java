/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.developer;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.preference.Preference;
import android.support.v7.preference.Preference.OnPreferenceChangeListener;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.lang.SecurityException;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.chromium.base.ContextUtils;
import org.chromium.base.FileUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsPanelPopup;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.settings.ChromeSwitchPreference;
import org.chromium.chrome.browser.settings.SettingsUtils;

import org.chromium.base.Log;

/**
 * Settings fragment containing preferences for QA team.
 */
public class BraveQAPreferences extends BravePreferenceFragment
        implements OnPreferenceChangeListener, BraveRewardsObserver {
    private static final String PREF_USE_REWARDS_STAGING_SERVER = "use_rewards_staging_server";
    private static final String PREF_QA_MAXIMIZE_INITIAL_ADS_NUMBER =
            "qa_maximize_initial_ads_number";
    private static final String PREF_QA_DEBUG_NTP= "qa_debug_ntp";

    private static final String QA_ADS_PER_HOUR = "qa_ads_per_hour";
    private static final String QA_IMPORT_REWARDS_DB = "qa_import_rewards_db";
    private static final String QA_EXPORT_REWARDS_DB = "qa_export_rewards_db";

    private static final String PUBLISHER_INFO_DB = "publisher_info_db";
    private static final String REWARDS_DB_SRC_DIR = "app_chrome/Default";
    private static final String REWARDS_DB_DST_DIR = "rewards";
    private static final int STORAGE_PERMISSION_REQUEST_CODE = 8000;

    private static final int MAX_ADS = 50;
    private static final int DEFAULT_ADS_PER_HOUR = 2;

    private ChromeSwitchPreference mIsStagingServer;
    private ChromeSwitchPreference mMaximizeAdsNumber;
    private ChromeSwitchPreference mDebugNTP;

    private Preference mImportRewardsDb;
    private Preference mExportRewardsDb;
    private String mRewardsSrc;
    private String mRewardsDst;
    private String mRewardsDstDir;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        SettingsUtils.addPreferencesFromResource(this, R.xml.qa_preferences);

        mIsStagingServer = (ChromeSwitchPreference) findPreference(PREF_USE_REWARDS_STAGING_SERVER);
        if (mIsStagingServer != null) {
            mIsStagingServer.setOnPreferenceChangeListener(this);
        }

        mMaximizeAdsNumber =
                (ChromeSwitchPreference) findPreference(PREF_QA_MAXIMIZE_INITIAL_ADS_NUMBER);
        if (mMaximizeAdsNumber != null) {
            mMaximizeAdsNumber.setEnabled(mIsStagingServer.isChecked());
            mMaximizeAdsNumber.setOnPreferenceChangeListener(this);
        }

        mDebugNTP = (ChromeSwitchPreference) findPreference(PREF_QA_DEBUG_NTP);
        if(mDebugNTP != null) {
            mDebugNTP.setOnPreferenceChangeListener(this);
        }

        mImportRewardsDb = findPreference(QA_IMPORT_REWARDS_DB);
        mExportRewardsDb = findPreference(QA_EXPORT_REWARDS_DB);
        setRewardsDbClickListeners();
    }

    private void setRewardsDbClickListeners() {
        if (mImportRewardsDb != null) {
            mImportRewardsDb.setOnPreferenceClickListener( preference -> {
                Context context = ContextUtils.getApplicationContext();
                mRewardsDst = context.getApplicationInfo().dataDir +
                        File.separator + REWARDS_DB_SRC_DIR +
                        File.separator + PUBLISHER_INFO_DB;

                mRewardsSrc = Environment.getExternalStoragePublicDirectory(
                        Environment.DIRECTORY_DOWNLOADS).getAbsolutePath() +
                        File.separator + REWARDS_DB_DST_DIR  + File.separator +
                        PUBLISHER_INFO_DB;

                mRewardsDstDir = "";

                if (isStoragePermissionGranted()) {
                    copyRewardsDbThread();
                }
                return true;
            });
        }

        if (mExportRewardsDb != null) {
            mExportRewardsDb.setOnPreferenceClickListener( preference -> {
                Context context = ContextUtils.getApplicationContext();
                mRewardsSrc = context.getApplicationInfo().dataDir +
                        File.separator + REWARDS_DB_SRC_DIR + File.separator +
                        PUBLISHER_INFO_DB;

                mRewardsDstDir = Environment.getExternalStoragePublicDirectory(
                        Environment.DIRECTORY_DOWNLOADS).getAbsolutePath() +
                        File.separator + REWARDS_DB_DST_DIR;

                SimpleDateFormat dateFormat =
                        new SimpleDateFormat("-yyyy-MM-dd-HHmmss");
                mRewardsDst = mRewardsDstDir + File.separator +
                        PUBLISHER_INFO_DB + dateFormat.format(new Date());

                if (isStoragePermissionGranted()) {
                    copyRewardsDbThread();
                }
                return true;
            });
        }
    }

    private void copyRewardsDbThread() {
        // disable UI before starting the thread
        mExportRewardsDb.setEnabled(false);
        new Thread( () -> {
            String erroMsg = "";

            // create dest dir if necessary
            if (!TextUtils.isEmpty(mRewardsDstDir)) {
                if ( !createDstDir (mRewardsDstDir)) {
                    erroMsg  = "Failed to create destination directory";
                }
            }

            // no errors so far: copy the file
            if (TextUtils.isEmpty(erroMsg)) {
                boolean succeeded = copyFile(mRewardsSrc, mRewardsDst);
                if (! succeeded) {
                    erroMsg = "Failed to copy db file";
                }
            }

            // update UI
            final String msg = (!TextUtils.isEmpty(erroMsg)) ? erroMsg : "Success";
            getActivity().runOnUiThread( () -> {
                Context context = ContextUtils.getApplicationContext();
                Toast.makeText(context , msg, Toast.LENGTH_SHORT).show();
                mExportRewardsDb.setEnabled(true);
            });
        }).start();
    }

    private boolean copyFile(String src, String dst) {
        boolean succeeded = false;
        try {
            InputStream in = new FileInputStream(src);
            FileUtils.copyStreamToFile(in, new File(dst));
            succeeded = true;
        }
        catch (IOException e) {
        }
        return succeeded;
    }

    private boolean createDstDir(String dir) {
        boolean succeeded = false;
        File dst = new File(dir);
        try {
            if (! dst.exists()) {
                succeeded = dst.mkdirs();
            }
            else {
                succeeded = true;
            }
        }
        catch (SecurityException e) {
            succeeded = false;
        }
        return succeeded;
    }

    private boolean isStoragePermissionGranted() {
        if (Build.VERSION.SDK_INT >=  Build.VERSION_CODES.M) {
            Context context = ContextUtils.getApplicationContext();
            if (context.checkSelfPermission(
                    android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    == PackageManager.PERMISSION_GRANTED) {
                return true;
            } else {
                ActivityCompat.requestPermissions(getActivity(),
                        new String[]
                        {android.Manifest.permission.WRITE_EXTERNAL_STORAGE},
                        STORAGE_PERMISSION_REQUEST_CODE);
                return false;
            }
        }
        else {
            return true;
        }
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, String[] permissions, int[] grantResults) {
        if (STORAGE_PERMISSION_REQUEST_CODE == requestCode &&
                grantResults.length > 0 &&
                grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            copyRewardsDbThread();
        }
    }

    @Override
    public void onStart() {
        BraveRewardsNativeWorker.getInstance().AddObserver(this);
        checkQACode();
        super.onStart();
    }

    @Override
    public void onStop() {
        BraveRewardsNativeWorker.getInstance().RemoveObserver(this);
        super.onStop();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (PREF_USE_REWARDS_STAGING_SERVER.equals(preference.getKey())) {
            BravePrefServiceBridge.getInstance().setUseRewardsStagingServer((boolean) newValue);
            BraveRewardsNativeWorker.getInstance().ResetTheWholeState();
            mMaximizeAdsNumber.setEnabled((boolean) newValue);
            enableMaximumAdsNumber(((boolean) newValue) && mMaximizeAdsNumber.isChecked());
            BraveRelaunchUtils.askForRelaunch(getActivity());
        } else if (PREF_QA_MAXIMIZE_INITIAL_ADS_NUMBER.equals(preference.getKey())) {
            enableMaximumAdsNumber((boolean) newValue);
        } else if (PREF_QA_DEBUG_NTP.equals(preference.getKey())) {
            setOnPreferenceValue(preference.getKey(), (boolean)newValue);
            BraveRelaunchUtils.askForRelaunch(getActivity());
        }
        return true;
    }

    private void setOnPreferenceValue(String preferenceName, boolean newValue) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(preferenceName, newValue);
        sharedPreferencesEditor.apply();
    }

    private void checkQACode() {
        LayoutInflater inflater =
                (LayoutInflater) getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.qa_code_check, null);
        final EditText input = (EditText) view.findViewById(R.id.qa_code);

        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int button) {
                if (button != AlertDialog.BUTTON_POSITIVE
                        || !input.getText().toString().equals(BraveConfig.DEVELOPER_OPTIONS_CODE)) {
                    getActivity().finish();
                }
            }
        };

        input.setOnFocusChangeListener(new OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                input.post(new Runnable() {
                    @Override
                    public void run() {
                        InputMethodManager inputMethodManager =
                                (InputMethodManager) getActivity().getSystemService(
                                        Context.INPUT_METHOD_SERVICE);
                        inputMethodManager.showSoftInput(input, InputMethodManager.SHOW_IMPLICIT);
                    }
                });
            }
        });
        input.requestFocus();

        AlertDialog.Builder alert =
                new AlertDialog.Builder(getActivity(), R.style.Theme_Chromium_AlertDialog);
        if (alert == null) {
            return;
        }
        AlertDialog.Builder alertDialog = alert
                .setTitle("Enter QA code")
                .setView(view)
                .setPositiveButton(R.string.ok, onClickListener)
                .setNegativeButton(R.string.cancel, onClickListener)
                .setCancelable(false);
        Dialog dialog = alertDialog.create();
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();
    }

    private void enableMaximumAdsNumber(boolean enable) {
        if (enable) {
            // Save current values
            int adsPerHour = BraveRewardsNativeWorker.getInstance().GetAdsPerHour();
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putInt(QA_ADS_PER_HOUR, adsPerHour);
            sharedPreferencesEditor.apply();
            // Set max value
            BraveRewardsNativeWorker.getInstance().SetAdsPerHour(MAX_ADS);
            return;
        }
        // Set saved values
        int adsPerHour = ContextUtils.getAppSharedPreferences().getInt(
                QA_ADS_PER_HOUR, DEFAULT_ADS_PER_HOUR);
        BraveRewardsNativeWorker.getInstance().SetAdsPerHour(adsPerHour);
    }

    @Override
    public void OnResetTheWholeState(boolean success) {
        if (success) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(BraveRewardsPanelPopup.PREF_GRANTS_NOTIFICATION_RECEIVED, false);
            sharedPreferencesEditor.putBoolean(BraveRewardsPanelPopup.PREF_WAS_BRAVE_REWARDS_TURNED_ON, false);
            sharedPreferencesEditor.apply();
            BravePrefServiceBridge.getInstance().setSafetynetCheckFailed(false);
            BraveRelaunchUtils.askForRelaunch(getActivity());
        } else {
            BraveRelaunchUtils.askForRelaunchCustom(getActivity());
        }
    }

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {}
}
