/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.adaptive.settings;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doReturn;

import static org.chromium.chrome.browser.preferences.ChromePreferenceKeys.ADAPTIVE_TOOLBAR_CUSTOMIZATION_ENABLED;
import static org.chromium.chrome.browser.preferences.ChromePreferenceKeys.ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS;

import android.os.Bundle;
import android.util.Pair;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentFactory;
import androidx.fragment.app.testing.FragmentScenario;
import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.omnibox.voice.VoiceRecognitionUtil;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.settings.ProfileDependentSetting;
import org.chromium.chrome.browser.signin.services.UnifiedConsentServiceBridge;
import org.chromium.chrome.browser.toolbar.R;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarButtonVariant;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarStatePredictor;
import org.chromium.chrome.browser.toolbar.adaptive.BraveAdaptiveToolbarPrefs;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.components.user_prefs.UserPrefsJni;

import java.util.List;

/** Tests for {@link AdaptiveToolbarSettingsFragment}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
@EnableFeatures(ChromeFeatureList.ADAPTIVE_BUTTON_IN_TOP_TOOLBAR_CUSTOMIZATION_V2)
@DisableFeatures({
    ChromeFeatureList.ADAPTIVE_BUTTON_IN_TOP_TOOLBAR_PAGE_SUMMARY,
    ChromeFeatureList.READALOUD
})
public class BraveAdaptiveToolbarSettingsFragmentTest {

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();
    @Mock Profile mProfile;
    @Mock private UserPrefsJni mUserPrefsNatives;
    @Mock private PrefService mPrefService;
    @Mock private TemplateUrlService mTemplateUrlService;

    private ChromeSwitchPreference mSwitchPreference;
    private RadioButtonGroupAdaptiveToolbarPreference mRadioPreference;

    @Before
    public void setUpTest() throws Exception {
        UserPrefsJni.setInstanceForTesting(mUserPrefsNatives);
        doReturn(mPrefService).when(mUserPrefsNatives).get(any());

        ChromeSharedPreferences.getInstance().removeKey(ADAPTIVE_TOOLBAR_CUSTOMIZATION_ENABLED);
        ChromeSharedPreferences.getInstance().removeKey(ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS);
        AdaptiveToolbarStatePredictor.setSegmentationResultsForTesting(
                new Pair<>(
                        false,
                        List.of(
                                AdaptiveToolbarButtonVariant.NEW_TAB,
                                AdaptiveToolbarButtonVariant.SHARE)));

        VoiceRecognitionUtil.setIsVoiceSearchEnabledForTesting(true);
        UnifiedConsentServiceBridge.setUrlKeyedAnonymizedDataCollectionEnabled(true);

        TemplateUrlServiceFactory.setInstanceForTesting(mTemplateUrlService);
        BraveRadioButtonGroupAdaptiveToolbarPreference.setIsJunitTesting(true);
    }

    @After
    public void tearDownTest() throws Exception {
        AdaptiveToolbarStatePredictor.setSegmentationResultsForTesting(null);
        ChromeSharedPreferences.getInstance().removeKey(ADAPTIVE_TOOLBAR_CUSTOMIZATION_ENABLED);
        ChromeSharedPreferences.getInstance().removeKey(ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS);
    }

    @Test
    @SmallTest
    public void testSelectShortcuts() {
        FragmentScenario<AdaptiveToolbarSettingsFragment> scenario = buildFragmentScenario();
        scenario.onFragment(
                fragment -> {
                    mSwitchPreference =
                            (ChromeSwitchPreference)
                                    fragment.findPreference(
                                            AdaptiveToolbarSettingsFragment
                                                    .PREF_TOOLBAR_SHORTCUT_SWITCH);
                    mRadioPreference =
                            (RadioButtonGroupAdaptiveToolbarPreference)
                                    fragment.findPreference(
                                            AdaptiveToolbarSettingsFragment
                                                    .PREF_ADAPTIVE_RADIO_GROUP);

                    Assert.assertFalse(
                            ChromeSharedPreferences.getInstance()
                                    .contains(ADAPTIVE_TOOLBAR_CUSTOMIZATION_ENABLED));
                    Assert.assertFalse(
                            BraveAdaptiveToolbarPrefs.isCustomizationPreferenceEnabled());

                    mSwitchPreference.performClick();
                    Assert.assertTrue(BraveAdaptiveToolbarPrefs.isCustomizationPreferenceEnabled());
                    Assert.assertTrue(
                            ChromeSharedPreferences.getInstance()
                                    .contains(ADAPTIVE_TOOLBAR_CUSTOMIZATION_ENABLED));
                    Assert.assertTrue(
                            ChromeSharedPreferences.getInstance()
                                    .readBoolean(ADAPTIVE_TOOLBAR_CUSTOMIZATION_ENABLED, false));

                    int expectedDefaultShortcut = AdaptiveToolbarButtonVariant.NEW_TAB;
                    Assert.assertEquals(
                            "Incorrect default setting.",
                            expectedDefaultShortcut,
                            BraveAdaptiveToolbarPrefs.getCustomizationSetting());
                    assertButtonCheckedCorrectly("Based on your usage", expectedDefaultShortcut);

                    // Check that Auto button is not visible.
                    Assert.assertEquals(
                            R.id.adaptive_option_based_on_usage,
                            getButton(AdaptiveToolbarButtonVariant.AUTO).getId());
                    Assert.assertFalse(
                            getButton(AdaptiveToolbarButtonVariant.AUTO).isVisibleToUser());

                    // Select New tab
                    Assert.assertEquals(
                            R.id.adaptive_option_new_tab,
                            getButton(AdaptiveToolbarButtonVariant.NEW_TAB).getId());
                    selectButton(AdaptiveToolbarButtonVariant.NEW_TAB);
                    assertButtonCheckedCorrectly("New tab", AdaptiveToolbarButtonVariant.NEW_TAB);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.NEW_TAB, mRadioPreference.getSelection());
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.NEW_TAB,
                            ChromeSharedPreferences.getInstance()
                                    .readInt(ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS));

                    // Select Share
                    Assert.assertEquals(
                            R.id.adaptive_option_share,
                            getButton(AdaptiveToolbarButtonVariant.SHARE).getId());
                    selectButton(AdaptiveToolbarButtonVariant.SHARE);
                    assertButtonCheckedCorrectly("Share", AdaptiveToolbarButtonVariant.SHARE);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.SHARE, mRadioPreference.getSelection());
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.SHARE,
                            ChromeSharedPreferences.getInstance()
                                    .readInt(ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS));

                    // Select Voice search
                    Assert.assertEquals(
                            R.id.adaptive_option_voice_search,
                            getButton(AdaptiveToolbarButtonVariant.VOICE).getId());
                    selectButton(AdaptiveToolbarButtonVariant.VOICE);
                    assertButtonCheckedCorrectly(
                            "Voice search", AdaptiveToolbarButtonVariant.VOICE);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.VOICE, mRadioPreference.getSelection());
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.VOICE,
                            ChromeSharedPreferences.getInstance()
                                    .readInt(ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS));

                    // Check indexes of Brave buttons and MAX_VALUE
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.BOOKMARKS,
                            AdaptiveToolbarButtonVariant.TAB_GROUPING + 1);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.HISTORY,
                            AdaptiveToolbarButtonVariant.BOOKMARKS + 1);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.DOWNLOADS,
                            AdaptiveToolbarButtonVariant.HISTORY + 1);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.PLAYLIST,
                            AdaptiveToolbarButtonVariant.DOWNLOADS + 1);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.LEO,
                            AdaptiveToolbarButtonVariant.PLAYLIST + 1);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.WALLET,
                            AdaptiveToolbarButtonVariant.LEO + 1);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.NEWS,
                            AdaptiveToolbarButtonVariant.WALLET + 1);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.NEWS,
                            AdaptiveToolbarButtonVariant.MAX_VALUE);

                    // Test Bookmarks button
                    Assert.assertEquals(
                            R.id.adaptive_option_bookmarks,
                            getButton(AdaptiveToolbarButtonVariant.BOOKMARKS).getId());
                    selectButton(AdaptiveToolbarButtonVariant.BOOKMARKS);
                    assertButtonCheckedCorrectly(
                            "Bookmarks", AdaptiveToolbarButtonVariant.BOOKMARKS);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.BOOKMARKS,
                            mRadioPreference.getSelection());
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.BOOKMARKS,
                            ChromeSharedPreferences.getInstance()
                                    .readInt(ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS));

                    // Test History button
                    Assert.assertEquals(
                            R.id.adaptive_option_history,
                            getButton(AdaptiveToolbarButtonVariant.HISTORY).getId());
                    selectButton(AdaptiveToolbarButtonVariant.HISTORY);
                    assertButtonCheckedCorrectly("History", AdaptiveToolbarButtonVariant.HISTORY);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.HISTORY, mRadioPreference.getSelection());
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.HISTORY,
                            ChromeSharedPreferences.getInstance()
                                    .readInt(ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS));

                    // Test Downloads button
                    Assert.assertEquals(
                            R.id.adaptive_option_downloads,
                            getButton(AdaptiveToolbarButtonVariant.DOWNLOADS).getId());
                    selectButton(AdaptiveToolbarButtonVariant.DOWNLOADS);
                    assertButtonCheckedCorrectly(
                            "Downloads", AdaptiveToolbarButtonVariant.DOWNLOADS);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.DOWNLOADS,
                            mRadioPreference.getSelection());
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.DOWNLOADS,
                            ChromeSharedPreferences.getInstance()
                                    .readInt(ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS));

                    // Test Leo AI button
                    Assert.assertEquals(
                            R.id.adaptive_option_brave_leo,
                            getButton(AdaptiveToolbarButtonVariant.LEO).getId());
                    selectButton(AdaptiveToolbarButtonVariant.LEO);
                    assertButtonCheckedCorrectly("Leo AI", AdaptiveToolbarButtonVariant.LEO);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.LEO, mRadioPreference.getSelection());
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.LEO,
                            ChromeSharedPreferences.getInstance()
                                    .readInt(ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS));

                    // Test Wallet button
                    Assert.assertEquals(
                            R.id.adaptive_option_brave_wallet,
                            getButton(AdaptiveToolbarButtonVariant.WALLET).getId());
                    selectButton(AdaptiveToolbarButtonVariant.WALLET);
                    assertButtonCheckedCorrectly(
                            "Brave Wallet", AdaptiveToolbarButtonVariant.WALLET);
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.WALLET, mRadioPreference.getSelection());
                    Assert.assertEquals(
                            AdaptiveToolbarButtonVariant.WALLET,
                            ChromeSharedPreferences.getInstance()
                                    .readInt(ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS));
                });
    }

    private RadioButtonWithDescription getButton(@AdaptiveToolbarButtonVariant int type) {
        return (RadioButtonWithDescription) mRadioPreference.getButton(type);
    }

    private void selectButton(@AdaptiveToolbarButtonVariant int type) {
        getButton(type).onClick(null);
    }

    private boolean isRestUnchecked(@AdaptiveToolbarButtonVariant int selectedType) {
        for (int i = 0; i <= AdaptiveToolbarButtonVariant.MAX_VALUE; i++) {
            RadioButtonWithDescription button = getButton(i);
            if (i != selectedType && button != null && button.isChecked()) {
                return false;
            }
        }
        return true;
    }

    private void assertButtonCheckedCorrectly(
            String buttonTitle, @AdaptiveToolbarButtonVariant int type) {
        Assert.assertTrue(buttonTitle + " button should be checked.", getButton(type).isChecked());
        Assert.assertTrue(
                "Buttons except " + buttonTitle + " should be unchecked.", isRestUnchecked(type));
    }

    private FragmentScenario<AdaptiveToolbarSettingsFragment> buildFragmentScenario() {
        return FragmentScenario.launchInContainer(
                AdaptiveToolbarSettingsFragment.class,
                Bundle.EMPTY,
                R.style.Theme_Chromium_Settings,
                new FragmentFactory() {
                    @Override
                    public Fragment instantiate(
                            @NonNull ClassLoader classLoader, @NonNull String className) {
                        Fragment fragment = super.instantiate(classLoader, className);
                        if (fragment instanceof ProfileDependentSetting) {
                            ((ProfileDependentSetting) fragment).setProfile(mProfile);
                        }
                        return fragment;
                    }
                });
    }
}
