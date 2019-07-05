/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import java.util.HashMap;
import org.chromium.base.Log;
import org.chromium.chrome.browser.ChromeVersionInfo;
import org.chromium.chrome.browser.preferences.PrefServiceBridge.AboutVersionStrings;
import org.chromium.chrome.browser.preferences.website.SiteSettingsCategory;
import org.chromium.chrome.browser.preferences.website.SiteSettingsPreferences;

/**
 * Base class for preferences.
 */
public class BravePreferenceFragment extends PreferenceFragment {
    private static final String PREF_SAFE_BROWSING = "safe_browsing";
    private static final String PREF_DO_NOT_TRACK = "do_not_track";
    private static final String PREF_USAGE_AND_CRASH_REPORTING = "usage_and_crash_reports";
    private static final String PREF_APPLICATION_VERSION = "application_version";

    private static final String VERSION_MASK = "Brave %s, Chromium %s";

    private final HashMap<String, Preference> mRemovedPreferences = new HashMap<>();

    @Override
    public void onResume() {
        super.onResume();

        // Remove preferences that are not used in Brave
        removePreferenceIfPresent(MainPreferences.PREF_SIGN_IN);
        removePreferenceIfPresent(MainPreferences.PREF_AUTOFILL_ASSISTANT);
        removePreferenceIfPresent(MainPreferences.PREF_DATA_REDUCTION);
        removePreferenceIfPresent(SiteSettingsCategory.preferenceKey(SiteSettingsCategory.Type.ADS));
        removePreferenceIfPresent(SiteSettingsCategory.preferenceKey(SiteSettingsCategory.Type.BACKGROUND_SYNC));
        removePreferenceIfPresent(PREF_SAFE_BROWSING);
        removePreferenceIfPresent(PREF_DO_NOT_TRACK);
        removePreferenceIfPresent(PREF_USAGE_AND_CRASH_REPORTING);

        // Set proper Brave's version
        if (this instanceof AboutChromePreferences) {
            if (ChromeVersionInfo.isOfficialBuild()) {
                PrefServiceBridge prefServiceBridge = PrefServiceBridge.getInstance();
                AboutVersionStrings versionStrings = prefServiceBridge.getAboutVersionStrings();
                String version = versionStrings.getApplicationVersion();
                PackageInfo info;
                try {
                    info = getActivity().getPackageManager().getPackageInfo(
                        getActivity().getPackageName(), 0);
                    String versionName = info.versionName;
                    String[] versionSplitted = version.split(" ");
                    if (versionSplitted.length > 1) {
                        version = String.format(VERSION_MASK, versionName, versionSplitted[1]);
                    }
                } catch (NameNotFoundException e) {
                    // Nothing special here, just keep version as is
                }
                Preference p = findPreference(PREF_APPLICATION_VERSION);
                if (p != null) p.setSummary(version);
            }
        }
    }

    /**
     *  We need to override it to avoid NullPointerException in Chromium's child classes
     */
    @Override
    public Preference findPreference(CharSequence key) {
        Preference result = super.findPreference(key);
        if (result == null) {
            result = mRemovedPreferences.get(key);
        }
        return result;
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
            mRemovedPreferences.put(preference.getKey(), preference);
        }
    }
}
