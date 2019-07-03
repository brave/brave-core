/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

import android.preference.Preference;
import android.preference.PreferenceFragment;
import java.util.HashMap;
import org.chromium.base.Log;
import org.chromium.chrome.browser.preferences.website.SiteSettingsCategory;
import org.chromium.chrome.browser.preferences.website.SiteSettingsPreferences;

/**
 * Base class for preferences.
 */
public class BravePreferenceFragment extends PreferenceFragment {
    private static final String PREF_SAFE_BROWSING = "safe_browsing";
    private static final String PREF_DO_NOT_TRACK = "do_not_track";
    private static final String PREF_USAGE_AND_CRASH_REPORTING = "usage_and_crash_reports";

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
