/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.text.SpannableString;
import android.view.View;

import androidx.preference.Preference;
import androidx.preference.Preference.OnPreferenceChangeListener;
import androidx.preference.PreferenceCategory;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.ntp.BraveFreshNtpHelper;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.ClickableSpansTextMessagePreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.text.ChromeClickableSpan;

/** Fragment to keep track of all the display related preferences. */
@NullMarked
public class BackgroundImagesPreferences extends BravePreferenceFragment
        implements OnPreferenceChangeListener {
    private static final String TAG = "BackgroundImages";

    // deprecated preferences from browser-android-tabs
    public static final String PREF_SHOW_BACKGROUND_IMAGES = "show_background_images";
    public static final String PREF_SHOW_SPONSORED_IMAGES = "show_sponsored_images";
    public static final String PREF_SHOW_TOP_SITES = "show_top_sites";
    public static final String PREF_SHOW_BRAVE_STATS = "show_brave_stats";
    public static final String PREF_BACKGROUND_IMAGES_CATEGORY = "background_images";
    public static final String PREF_OPENING_SCREEN = "opening_screen_option";
    public static final String PREF_OPENING_SCREEN_CATEGORY = "opening_screen";

    public static final String PREF_SPONSORED_IMAGES_LEARN_MORE = "sponsored_images_learn_more";

    public static final String NEW_TAB_TAKEOVER_LEARN_MORE_LINK_URL =
            "https://support.brave.app/hc/en-us/articles/35182999599501";

    private ChromeSwitchPreference mShowBackgroundImagesPref;
    private ChromeSwitchPreference mShowSponsoredImagesPref;
    private ChromeSwitchPreference mShowBraveStatsPref;
    private ChromeSwitchPreference mShowTopSitesPref;
    private ClickableSpansTextMessagePreference mLearnMorePreference;
    private BraveRadioButtonGroupOpeningScreenPreference mOpeningScreenPref;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPageTitle.set(getString(R.string.prefs_new_tab_page));
        SettingsUtils.addPreferencesFromResource(this, R.xml.background_images_preferences);
    }

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mShowBackgroundImagesPref =
                (ChromeSwitchPreference) findPreference(PREF_SHOW_BACKGROUND_IMAGES);
        if (mShowBackgroundImagesPref != null) {
            mShowBackgroundImagesPref.setEnabled(true);
            mShowBackgroundImagesPref.setChecked(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE));
            mShowBackgroundImagesPref.setOnPreferenceChangeListener(this);
        }
        mShowSponsoredImagesPref =
                (ChromeSwitchPreference) findPreference(PREF_SHOW_SPONSORED_IMAGES);
        if (mShowSponsoredImagesPref != null) {
            mShowSponsoredImagesPref.setEnabled(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE));
            mShowSponsoredImagesPref.setChecked(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(
                                    BravePref.NEW_TAB_PAGE_SHOW_SPONSORED_IMAGES_BACKGROUND_IMAGE));
            mShowSponsoredImagesPref.setOnPreferenceChangeListener(this);
        }
        mLearnMorePreference =
                (ClickableSpansTextMessagePreference)
                        findPreference(PREF_SPONSORED_IMAGES_LEARN_MORE);
        if (mLearnMorePreference != null) {
            ChromeClickableSpan chromeClickableSpan =
                    new ChromeClickableSpan(
                            getContext().getColor(R.color.brave_link),
                            sponsoredImagesLearnMoreClickedCallback());
            SpannableString spannableString =
                    new SpannableString(
                            getContext().getString(R.string.sponsored_images_learn_more));
            spannableString.setSpan(chromeClickableSpan, 0, spannableString.length(), 0);
            mLearnMorePreference.setSummary(spannableString);
        }
        if (!NTPBackgroundImagesBridge.enableSponsoredImages()) {
            PreferenceCategory preferenceCategory =
                    (PreferenceCategory) findPreference(PREF_BACKGROUND_IMAGES_CATEGORY);
            preferenceCategory.removePreference(mShowSponsoredImagesPref);
            preferenceCategory.removePreference(mLearnMorePreference);
        }

        mShowTopSitesPref = (ChromeSwitchPreference) findPreference(PREF_SHOW_TOP_SITES);
        if (mShowTopSitesPref != null) {
            mShowTopSitesPref.setEnabled(true);
            mShowTopSitesPref.setChecked(NtpUtil.shouldDisplayTopSites());
            mShowTopSitesPref.setOnPreferenceChangeListener(this);
        }
        mShowBraveStatsPref = (ChromeSwitchPreference) findPreference(PREF_SHOW_BRAVE_STATS);
        if (mShowBraveStatsPref != null) {
            mShowBraveStatsPref.setEnabled(true);
            mShowBraveStatsPref.setChecked(NtpUtil.shouldDisplayBraveStats());
            mShowBraveStatsPref.setOnPreferenceChangeListener(this);
        }

        // Initialize Opening Screen preference
        mOpeningScreenPref =
                (BraveRadioButtonGroupOpeningScreenPreference) findPreference(PREF_OPENING_SCREEN);
        PreferenceCategory openingScreenCategory =
                (PreferenceCategory) findPreference(PREF_OPENING_SCREEN_CATEGORY);

        // Hide the preference category if feature is disabled or variant is "A"
        if (!BraveFreshNtpHelper.isEnabled()) {
            if (openingScreenCategory != null) {
                getPreferenceScreen().removePreference(openingScreenCategory);
            }
        } else {
            String variant = BraveFreshNtpHelper.getVariant();
            if (variant != null && variant.equals("A")) {
                if (openingScreenCategory != null) {
                    getPreferenceScreen().removePreference(openingScreenCategory);
                }
            } else if (mOpeningScreenPref != null) {
                int currentValue =
                        ChromeSharedPreferences.getInstance()
                                .readInt(BravePreferenceKeys.BRAVE_NEW_TAB_PAGE_OPENING_SCREEN, 1);
                mOpeningScreenPref.initialize(currentValue);
                mOpeningScreenPref.setOnPreferenceChangeListener(this);
            }
        }
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_SHOW_BACKGROUND_IMAGES.equals(key)) {
            if (mShowSponsoredImagesPref != null) {
                mShowSponsoredImagesPref.setEnabled((boolean) newValue);
            }
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE, (boolean) newValue);
            BraveRelaunchUtils.askForRelaunch(getActivity());
        } else if (PREF_SHOW_SPONSORED_IMAGES.equals(key)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(
                            BravePref.NEW_TAB_PAGE_SHOW_SPONSORED_IMAGES_BACKGROUND_IMAGE,
                            (boolean) newValue);
            BraveRelaunchUtils.askForRelaunch(getActivity());
        } else if (PREF_SHOW_TOP_SITES.equals(key)) {
            NtpUtil.setDisplayTopSites((boolean) newValue);
        } else if (PREF_SHOW_BRAVE_STATS.equals(key)) {
            NtpUtil.setDisplayBraveStats((boolean) newValue);
        } else if (PREF_OPENING_SCREEN.equals(key)) {
            int option = (int) newValue;
            ChromeSharedPreferences.getInstance()
                    .writeInt(BravePreferenceKeys.BRAVE_NEW_TAB_PAGE_OPENING_SCREEN, option);
        }
        return true;
    }

    private Callback<View> sponsoredImagesLearnMoreClickedCallback() {
        return (view) -> {
            try {
                TabUtils.openUrlInNewTab(false, NEW_TAB_TAKEOVER_LEARN_MORE_LINK_URL);
                TabUtils.bringChromeTabbedActivityToTheTop(BraveActivity.getBraveActivity());
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "sponsoredImagesLearnMoreClickedCallback" + e);
            }
        };
    }
}
