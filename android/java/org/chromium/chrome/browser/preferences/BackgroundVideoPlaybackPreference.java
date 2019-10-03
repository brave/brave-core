package org.chromium.chrome.browser.preferences;

import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.ChromeSwitchPreferenceCompat;

public class BackgroundVideoPlaybackPreference
        extends PreferenceFragmentCompat implements Preference.OnPreferenceChangeListener {
    private static final String BACKGROUND_VIDEO_PLAYBACK_KEY = "background_video_playback";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.prefs_background_video_playback);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.background_video_playback_preference);

        ChromeSwitchPreferenceCompat playbackPref =
                (ChromeSwitchPreferenceCompat) findPreference(BACKGROUND_VIDEO_PLAYBACK_KEY);
        playbackPref.setChecked(true);
        playbackPref.setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        return true;
    }
}
