/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.accessibility;

import android.content.Context;
import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.components.browser_ui.accessibility.AccessibilitySettings;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;
import org.chromium.components.browser_ui.settings.search.PreferenceParser;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;

@NullMarked
public class BraveAccessibilitySettings extends AccessibilitySettings {
    public static final String PREF_PULL_TO_REFRESH = "pull_to_refresh";

    private @Nullable ChromeSwitchPreference mPullToRefreshPref;

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        mPullToRefreshPref = new ChromeSwitchPreference(requireContext(), null);
        mPullToRefreshPref.setKey(PREF_PULL_TO_REFRESH);
        mPullToRefreshPref.setTitle(R.string.pull_to_refresh_title);
        mPullToRefreshPref.setSummary(R.string.pull_to_refresh_summary);
        mPullToRefreshPref.setChecked(
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.PREF_PULL_TO_REFRESH, true));
        mPullToRefreshPref.setOnPreferenceChangeListener(this);
        getPreferenceScreen().addPreference(mPullToRefreshPref);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (PREF_PULL_TO_REFRESH.equals(preference.getKey())) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.PREF_PULL_TO_REFRESH, (Boolean) newValue);
            return true;
        }
        return super.onPreferenceChange(preference, newValue);
    }

    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(BraveAccessibilitySettings.class.getName()) {
                @Override
                public void updateDynamicPreferences(Context context, SettingsIndexData indexData) {
                    String frag = BraveAccessibilitySettings.class.getName();
                    String id = PreferenceParser.createUniqueId(frag, PREF_PULL_TO_REFRESH);
                    Context appContext = ContextUtils.getApplicationContext();
                    indexData.addEntry(
                            id,
                            new SettingsIndexData.Entry.Builder(
                                            id,
                                            PREF_PULL_TO_REFRESH,
                                            appContext.getString(R.string.pull_to_refresh_title),
                                            frag)
                                    .setSummary(
                                            appContext.getString(R.string.pull_to_refresh_summary))
                                    .build());
                }
            };
}
