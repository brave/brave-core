/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.developer;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.util.BraveDbUtil;
import org.chromium.components.browser_ui.settings.SettingsUtils;

/**
 * Settings fragment containing preferences for QA team.
 */
public class BraveRewardsDebugPreferences extends BravePreferenceFragment {
    public static final String KEY = "brave_rewards_debug_preferences";
    private static final String QA_EXPORT_REWARDS_DB = "export_rewards_db";

    private Preference mExportRewardsDb;
    private BraveDbUtil mDbUtil;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_rewards_debug_preferences);

        // Hardcoded because it is for internal use only, not translated
        mPageTitle.set("Rewards Debug");

        mDbUtil = BraveDbUtil.getInstance();
        mExportRewardsDb = findPreference(QA_EXPORT_REWARDS_DB);
        setRewardsDbClickListeners();
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    private void setRewardsDbClickListeners() {
        if (mExportRewardsDb != null) {
            mExportRewardsDb.setOnPreferenceClickListener(
                    preference -> {
                        if (isStoragePermissionGranted(true)) {
                            requestRestart();
                        }
                        return true;
                    });
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            if (STORAGE_PERMISSION_EXPORT_REQUEST_CODE == requestCode) {
                requestRestart();
            }
        }
    }

    private void requestRestart() {
        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int button) {
                if (button == AlertDialog.BUTTON_POSITIVE) {
                    mDbUtil.setPerformDbExportOnStart(true);
                    BraveRelaunchUtils.restart();
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
