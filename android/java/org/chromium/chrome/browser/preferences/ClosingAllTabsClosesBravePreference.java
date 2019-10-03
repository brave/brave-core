package org.chromium.chrome.browser.preferences;

import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.ChromeSwitchPreferenceCompat;

public class ClosingAllTabsClosesBravePreference
        extends PreferenceFragmentCompat implements Preference.OnPreferenceChangeListener {
    private static final String CLOSING_ALL_TABS_CLOSES_BRAVE_KEY = "closing_all_tabs_closes_brave";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.prefs_closing_all_tabs_closes_brave);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.closing_all_tabs_closes_brave_preference);

        ChromeSwitchPreferenceCompat pref =
                (ChromeSwitchPreferenceCompat) findPreference(CLOSING_ALL_TABS_CLOSES_BRAVE_KEY);
        pref.setChecked(true);
        pref.setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        return true;
    }
}
