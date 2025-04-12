/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.view.View;

import androidx.preference.Preference;
import androidx.preference.Preference.OnPreferenceChangeListener;
import androidx.preference.PreferenceCategory;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.ClickableSpansTextMessagePreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.text.ChromeClickableSpan;
import org.chromium.ui.text.SpanApplier;

/** Fragment to keep track of all the display related preferences. */
public class BackgroundImagesPreferences extends BravePreferenceFragment
        implements OnPreferenceChangeListener {
    // deprecated preferences from browser-android-tabs
    public static final String PREF_SHOW_BACKGROUND_IMAGES = "show_background_images";
    public static final String PREF_SHOW_SPONSORED_IMAGES = "show_sponsored_images";
    public static final String PREF_SHOW_TOP_SITES = "show_top_sites";
    public static final String PREF_SHOW_BRAVE_STATS = "show_brave_stats";
    public static final String PREF_BACKGROUND_IMAGES_CATEGORY = "background_images";

    public static final String PREF_SPONSORED_IMAGES_LEARN_MORE = "sponsored_images_learn_more";
    public static final String SPONSORED_IMAGES_LEARN_MORE_URL =
            "https://support.brave.com/hc/en-us/articles/35182999599501";

    private ChromeSwitchPreference mShowBackgroundImagesPref;
    private ChromeSwitchPreference mShowSponsoredImagesPref;
    private ChromeSwitchPreference mShowBraveStatsPref;
    private ChromeSwitchPreference mShowTopSitesPref;
    private ClickableSpansTextMessagePreference mLearnMorePreference;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPageTitle.set(getString(R.string.prefs_new_tab_page));
        SettingsUtils.addPreferencesFromResource(this, R.xml.background_images_preferences);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
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
            mLearnMorePreference.setSummary(
                    SpanApplier.applySpans(
                            getResources().getString(R.string.sponsored_images_learn_more),
                            new SpanApplier.SpanInfo(
                                    "<link1>",
                                    "</link1>",
                                    new ChromeClickableSpan(
                                            getContext(),
                                            sponsoredImagesLearnMoreClickedCallback()))));
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
        }
        return true;
    }

    private Callback<View> sponsoredImagesLearnMoreClickedCallback() {
        return (view) -> {
            try {
                TabUtils.openUrlInNewTab(false, SPONSORED_IMAGES_LEARN_MORE_URL);
                TabUtils.bringChromeTabbedActivityToTheTop(BraveActivity.getBraveActivity());
            } catch (BraveActivity.BraveActivityNotFoundException e) {
            }
        };
    }
}
