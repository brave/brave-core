/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.developer;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

import androidx.preference.Preference;
import androidx.preference.Preference.OnPreferenceChangeListener;

import org.jni_zero.CalledByNative;

import org.chromium.base.ContextUtils;
import org.chromium.base.FileUtils;
import org.chromium.base.Log;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.billing.LinkSubscriptionUtils;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.rewards.BraveRewardsPanel;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.util.BraveDbUtil;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.user_prefs.UserPrefs;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

/** Settings fragment containing preferences for QA team. */
public class BraveQAPreferences extends BravePreferenceFragment
        implements OnPreferenceChangeListener, BraveRewardsObserver {
    private static final String PREF_USE_REWARDS_STAGING_SERVER = "use_rewards_staging_server";
    private static final String PREF_USE_SYNC_STAGING_SERVER = "use_sync_staging_server";
    private static final String PREF_QA_MAXIMIZE_INITIAL_ADS_NUMBER =
            "qa_maximize_initial_ads_number";
    private static final String PREF_QA_DEBUG_NTP = "qa_debug_ntp";
    private static final String PREF_QA_VLOG_REWARDS = "qa_vlog_rewards";
    private static final String PREF_QA_COMMAND_LINE = "qa_command_line";

    private static final String QA_ADS_PER_HOUR = "qa_ads_per_hour";
    private static final String QA_IMPORT_REWARDS_DB = "qa_import_rewards_db";
    private static final String QA_EXPORT_REWARDS_DB = "qa_export_rewards_db";

    private static final int CHOOSE_FILE_FOR_IMPORT_REQUEST_CODE = STORAGE_PERMISSION_IMPORT_REQUEST_CODE + 1;

    private static final int MAX_ADS = 10;
    private static final int DEFAULT_ADS_PER_HOUR = 2;

    private ChromeSwitchPreference mLinkSubscriptionOnStaging;
    private ChromeSwitchPreference mBraveDormantFeatureEngagement;
    private ChromeSwitchPreference mIsStagingServer;
    private ChromeSwitchPreference mIsSyncStagingServer;
    private ChromeSwitchPreference mMaximizeAdsNumber;
    private ChromeSwitchPreference mDebugNTP;
    private ChromeSwitchPreference mVlogRewards;
    private Preference mCommandLine;

    private Preference mImportRewardsDb;
    private Preference mExportRewardsDb;
    private BraveDbUtil mDbUtil;
    private String mFileToImport;
    private boolean mUseRewardsStagingServer;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        SettingsUtils.addPreferencesFromResource(this, R.xml.qa_preferences);

        // Hardcoded because it is for internal use only, hidden by access code, not translated
        mPageTitle.set("QA Preferences");

        mLinkSubscriptionOnStaging =
                (ChromeSwitchPreference)
                        findPreference(LinkSubscriptionUtils.PREF_LINK_SUBSCRIPTION_ON_STAGING);
        if (mLinkSubscriptionOnStaging != null) {
            mLinkSubscriptionOnStaging.setOnPreferenceChangeListener(this);
        }

        mBraveDormantFeatureEngagement =
                (ChromeSwitchPreference)
                        findPreference(OnboardingPrefManager.PREF_DORMANT_USERS_ENGAGEMENT);
        if (mBraveDormantFeatureEngagement != null) {
            mBraveDormantFeatureEngagement.setOnPreferenceChangeListener(this);
        }

        mIsStagingServer = (ChromeSwitchPreference) findPreference(PREF_USE_REWARDS_STAGING_SERVER);
        if (mIsStagingServer != null) {
            mIsStagingServer.setOnPreferenceChangeListener(this);
        }
        mIsStagingServer.setChecked(
                UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                        .getBoolean(BravePref.USE_REWARDS_STAGING_SERVER));

        mIsSyncStagingServer =
                (ChromeSwitchPreference) findPreference(PREF_USE_SYNC_STAGING_SERVER);
        if (mIsSyncStagingServer != null) {
            mIsSyncStagingServer.setOnPreferenceChangeListener(this);
        }
        mIsSyncStagingServer.setChecked(isSyncStagingUsed());

        mMaximizeAdsNumber =
                (ChromeSwitchPreference) findPreference(PREF_QA_MAXIMIZE_INITIAL_ADS_NUMBER);
        if (mMaximizeAdsNumber != null) {
            mMaximizeAdsNumber.setEnabled(mIsStagingServer.isChecked());
            mMaximizeAdsNumber.setOnPreferenceChangeListener(this);
        }

        mDebugNTP = (ChromeSwitchPreference) findPreference(PREF_QA_DEBUG_NTP);
        if (mDebugNTP != null) {
            mDebugNTP.setOnPreferenceChangeListener(this);
        }

        mVlogRewards = (ChromeSwitchPreference) findPreference(PREF_QA_VLOG_REWARDS);
        if (mVlogRewards != null) {
            mVlogRewards.setOnPreferenceChangeListener(this);
            mVlogRewards.setChecked(shouldVlogRewards());
        }

        mDbUtil = BraveDbUtil.getInstance();

        mImportRewardsDb = findPreference(QA_IMPORT_REWARDS_DB);
        mExportRewardsDb = findPreference(QA_EXPORT_REWARDS_DB);
        setRewardsDbClickListeners();

        mCommandLine = findPreference(PREF_QA_COMMAND_LINE);
        setCommandLineClickListener();

        checkQACode();
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    private void setRewardsDbClickListeners() {
        if (mImportRewardsDb != null) {
            mImportRewardsDb.setOnPreferenceClickListener(
                    preference -> {
                        Intent intent =
                                new Intent().setType("*/*").setAction(Intent.ACTION_GET_CONTENT);

                        startActivityForResult(
                                Intent.createChooser(intent, "Select a file"),
                                CHOOSE_FILE_FOR_IMPORT_REQUEST_CODE);
                        return true;
                    });
        }

        if (mExportRewardsDb != null) {
            mExportRewardsDb.setOnPreferenceClickListener( preference -> {
                if (isStoragePermissionGranted(true)) {
                    requestRestart(false);
                }
                return true;
            });
        }
    }

    private void setCommandLineClickListener() {
        if (mCommandLine == null) {
            return;
        }
        mCommandLine.setOnPreferenceClickListener(preference -> {
            LayoutInflater inflater = (LayoutInflater) getActivity().getSystemService(
                    Context.LAYOUT_INFLATER_SERVICE);
            View view = inflater.inflate(R.layout.qa_command_line, null);
            final EditText input = (EditText) view.findViewById(R.id.qa_command_line);

            input.setText(getPreferenceString(PREF_QA_COMMAND_LINE));

            DialogInterface.OnClickListener onClickListener =
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int button) {
                            if (button == AlertDialog.BUTTON_POSITIVE) {
                                // OK was pressed
                                String newCommandLine = input.getText().toString();
                                setPreferenceString(PREF_QA_COMMAND_LINE, newCommandLine);
                                BraveRelaunchUtils.askForRelaunch(getActivity());
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
                            inputMethodManager.showSoftInput(
                                    input, InputMethodManager.SHOW_IMPLICIT);
                        }
                    });
                }
            });
            input.requestFocus();

            AlertDialog.Builder alert = new AlertDialog.Builder(
                    getActivity(), R.style.ThemeOverlay_BrowserUI_AlertDialog);
            if (alert == null) {
                return true;
            }
            AlertDialog.Builder alertDialog =
                    alert.setTitle("Enter Command Line string")
                            .setView(view)
                            .setPositiveButton(R.string.ok, onClickListener)
                            .setNegativeButton(R.string.cancel, onClickListener)
                            .setCancelable(false);
            Dialog dialog = alertDialog.create();
            dialog.setCanceledOnTouchOutside(false);
            dialog.show();

            return true;
        });
    }

    @Override
    public void onStart() {
        BraveRewardsNativeWorker.getInstance().addObserver(this);
        super.onStart();
    }

    @Override
    public void onStop() {
        BraveRewardsNativeWorker.getInstance().removeObserver(this);
        super.onStop();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (PREF_USE_REWARDS_STAGING_SERVER.equals(preference.getKey())) {
            BraveRewardsNativeWorker.getInstance().resetTheWholeState();
            mUseRewardsStagingServer = (boolean) newValue;
            mMaximizeAdsNumber.setEnabled((boolean) newValue);
            enableMaximumAdsNumber(((boolean) newValue) && mMaximizeAdsNumber.isChecked());
        } else if (PREF_QA_MAXIMIZE_INITIAL_ADS_NUMBER.equals(preference.getKey())) {
            enableMaximumAdsNumber((boolean) newValue);
        } else if (PREF_QA_DEBUG_NTP.equals(preference.getKey())
                || PREF_USE_SYNC_STAGING_SERVER.equals(preference.getKey())
                || PREF_QA_VLOG_REWARDS.equals(preference.getKey())
                || LinkSubscriptionUtils.PREF_LINK_SUBSCRIPTION_ON_STAGING.equals(
                        preference.getKey())
                || OnboardingPrefManager.PREF_DORMANT_USERS_ENGAGEMENT.equals(
                        preference.getKey())) {
            setOnPreferenceValue(preference.getKey(), (boolean)newValue);
            BraveRelaunchUtils.askForRelaunch(getActivity());
        }
        return true;
    }

    private static void setOnPreferenceValue(String preferenceName, boolean newValue) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(preferenceName, newValue);
        sharedPreferencesEditor.apply();
    }

    private static boolean getPreferenceValue(String preferenceName) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(preferenceName, false);
    }

    private static void setPreferenceString(String preferenceName, String newValue) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putString(preferenceName, newValue);
        sharedPreferencesEditor.apply();
    }

    private static String getPreferenceString(String preferenceName) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getString(preferenceName, "");
    }

    @CalledByNative
    public static boolean isSyncStagingUsed() {
        return getPreferenceValue(PREF_USE_SYNC_STAGING_SERVER);
    }

    public static boolean shouldVlogRewards() {
        return getPreferenceValue(PREF_QA_VLOG_REWARDS);
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
                new AlertDialog.Builder(getActivity(), R.style.ThemeOverlay_BrowserUI_AlertDialog);
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
            int adsPerHour = BraveRewardsNativeWorker.getInstance().getAdsPerHour();
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putInt(QA_ADS_PER_HOUR, adsPerHour);
            sharedPreferencesEditor.apply();
            // Set max value
            BraveRewardsNativeWorker.getInstance().setAdsPerHour(MAX_ADS);
            return;
        }
        // Set saved values
        int adsPerHour = ContextUtils.getAppSharedPreferences().getInt(
                             QA_ADS_PER_HOUR, DEFAULT_ADS_PER_HOUR);
        BraveRewardsNativeWorker.getInstance().setAdsPerHour(adsPerHour);
    }

    @Override
    public void onResetTheWholeState(boolean success) {
        if (success) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(
                    BraveRewardsPanel.PREF_WAS_BRAVE_REWARDS_TURNED_ON, false);
            sharedPreferencesEditor.apply();

            BravePrefServiceBridge.getInstance().setSafetynetCheckFailed(false);
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(BravePref.USE_REWARDS_STAGING_SERVER, mUseRewardsStagingServer);
            BraveRewardsHelper.setRewardsEnvChange(true);

            BraveRelaunchUtils.askForRelaunch(getActivity());
        } else {
            BraveRelaunchUtils.askForRelaunchCustom(getActivity());
        }
    }

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {}

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            if (STORAGE_PERMISSION_EXPORT_REQUEST_CODE == requestCode) {
                requestRestart(false);
            } else if (STORAGE_PERMISSION_IMPORT_REQUEST_CODE == requestCode) {
                requestRestart(true);
            }
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == CHOOSE_FILE_FOR_IMPORT_REQUEST_CODE && resultCode == Activity.RESULT_OK
                && data != null) {
            try {
                InputStream in =
                    ContextUtils.getApplicationContext().getContentResolver().openInputStream(
                        data.getData());
                mFileToImport = mDbUtil.importDestinationPath() + ".prep";
                FileUtils.copyStreamToFile(in, new File(mFileToImport));
                in.close();
            } catch (IOException e) {
                Log.e(BraveDbUtil.getTag(), "Error on preparing database file: " + e);
                return;
            }
            requestRestart(true);
        }
    }

    private void requestRestart(boolean isImport) {
        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int button) {
                if (button == AlertDialog.BUTTON_POSITIVE) {
                    if (isImport) {
                        mDbUtil.setPerformDbImportOnStart(true);
                        mDbUtil.setDbImportFile(mFileToImport);
                    } else {
                        mDbUtil.setPerformDbExportOnStart(true);
                    }
                    BraveRelaunchUtils.restart();
                } else {
                    mDbUtil.cleanUpDbOperationRequest();
                }
            }
        };
        AlertDialog.Builder alertDialog =
                new AlertDialog.Builder(getActivity(), R.style.ThemeOverlay_BrowserUI_AlertDialog)
                        .setMessage(
                                "This operation requires restart. Would you like to restart"
                                        + " application and start operation?")
                        .setPositiveButton(R.string.ok, onClickListener)
                        .setNegativeButton(R.string.cancel, onClickListener);
        Dialog dialog = alertDialog.create();
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();
    }
}
