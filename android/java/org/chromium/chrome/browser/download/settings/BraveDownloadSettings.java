/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.download.settings;

import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveFeatureUtil;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BraveDownloadSettings extends DownloadSettings
        implements Preference.OnPreferenceChangeListener {
    private static final String PREF_AUTOMATICALLY_OPEN_WHEN_POSSIBLE =
            "automatically_open_when_possible";
    public static final String PREF_LOCATION_PROMPT_ENABLED = "location_prompt_enabled";
    private static final String PREF_DOWNLOAD_PROGRESS_NOTIFICATION_BUBBLE =
            "download_progress_notification_bubble";
    private static final String PREF_ENABLE_PARALLEL_DOWNLOADING = "enable_parallel_downloading";
    private static final String PARALLEL_DOWNLOADING = "ParallelDownloading";

    private ChromeSwitchPreference mAutomaticallyOpenWhenPossiblePref;
    private ChromeSwitchPreference mDownloadProgressNotificationBubblePref;
    private ChromeSwitchPreference mParallelDownloadingPref;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_download_preferences);

        mAutomaticallyOpenWhenPossiblePref =
                (ChromeSwitchPreference) findPreference(PREF_AUTOMATICALLY_OPEN_WHEN_POSSIBLE);
        mAutomaticallyOpenWhenPossiblePref.setOnPreferenceChangeListener(this);

        mDownloadProgressNotificationBubblePref =
                (ChromeSwitchPreference) findPreference(PREF_DOWNLOAD_PROGRESS_NOTIFICATION_BUBBLE);
        mDownloadProgressNotificationBubblePref.setOnPreferenceChangeListener(this);

        mParallelDownloadingPref =
                (ChromeSwitchPreference) findPreference(PREF_ENABLE_PARALLEL_DOWNLOADING);
        mParallelDownloadingPref.setOnPreferenceChangeListener(this);

        ChromeSwitchPreference locationPromptEnabledPref =
                (ChromeSwitchPreference) findPreference(PREF_LOCATION_PROMPT_ENABLED);
        if (locationPromptEnabledPref != null) {
            locationPromptEnabledPref.setTitle(
                    R.string.brave_download_location_prompt_enabled_title);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        updateDownloadSettings();
    }

    private void updateDownloadSettings() {
        boolean automaticallyOpenWhenPossible = ContextUtils.getAppSharedPreferences().getBoolean(
                BravePreferenceKeys.BRAVE_DOWNLOADS_AUTOMATICALLY_OPEN_WHEN_POSSIBLE, true);
        mAutomaticallyOpenWhenPossiblePref.setChecked(automaticallyOpenWhenPossible);

        boolean downloadProgressNotificationBubble =
                ContextUtils.getAppSharedPreferences()
                        .getBoolean(
                                BravePreferenceKeys
                                        .BRAVE_DOWNLOADS_DOWNLOAD_PROGRESS_NOTIFICATION_BUBBLE,
                                false);
        mDownloadProgressNotificationBubblePref.setChecked(downloadProgressNotificationBubble);

        boolean isParallelDownloadingEnabled = ChromeFeatureList.isEnabled(PARALLEL_DOWNLOADING);
        mParallelDownloadingPref.setChecked(isParallelDownloadingEnabled);
    }

    // Preference.OnPreferenceChangeListener implementation.
    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (PREF_AUTOMATICALLY_OPEN_WHEN_POSSIBLE.equals(preference.getKey())) {
            ContextUtils.getAppSharedPreferences()
                    .edit()
                    .putBoolean(
                            BravePreferenceKeys.BRAVE_DOWNLOADS_AUTOMATICALLY_OPEN_WHEN_POSSIBLE,
                            (boolean) newValue)
                    .apply();
        } else if (PREF_DOWNLOAD_PROGRESS_NOTIFICATION_BUBBLE.equals(preference.getKey())) {
            ContextUtils.getAppSharedPreferences()
                    .edit()
                    .putBoolean(
                            BravePreferenceKeys
                                    .BRAVE_DOWNLOADS_DOWNLOAD_PROGRESS_NOTIFICATION_BUBBLE,
                            (boolean) newValue)
                    .apply();
        } else if (PREF_ENABLE_PARALLEL_DOWNLOADING.equals(preference.getKey())) {
            BraveFeatureUtil.enableFeature(
                    BraveFeatureList.ENABLE_PARALLEL_DOWNLOADING, (boolean) newValue, false);
            BraveRelaunchUtils.askForRelaunch(getActivity());
        }
        return super.onPreferenceChange(preference, newValue);
    }
}
