/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.test.filters.SmallTest;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.search_engines.settings.BraveSearchEngineAdapter;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import java.util.Arrays;

/** Tests for {@link BraveSearchEngineUtils}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveSearchEngineUtilsTest {

    private static final String GOOGLE_SEARCH_ENGINE = "Google";
    private static final String BRAVE_KEYWORD = ":br";

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();
    @Mock private Profile mProfile;
    @Mock private TemplateUrlService mTemplateUrlService;
    @Mock private TemplateUrl mDefaultTemplateUrl;
    @Mock private TemplateUrl mBraveTemplateUrl;

    @Before
    public void setUp() {
        // Clear any existing preferences
        ChromeSharedPreferences.getInstance()
                .removeKey(BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME);
        ChromeSharedPreferences.getInstance()
                .removeKey(BraveSearchEngineAdapter.PRIVATE_DSE_SHORTNAME);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.SEARCH_CHOICE_SCREEN_INSTALL);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.DEFAULT_SEARCH_ENGINE_CHANGED);

        // Set up mock TemplateUrlService
        TemplateUrlServiceFactory.setInstanceForTesting(mTemplateUrlService);
        doReturn(true).when(mTemplateUrlService).isLoaded();
        doReturn(mDefaultTemplateUrl).when(mTemplateUrlService).getDefaultSearchEngineTemplateUrl();
        when(mDefaultTemplateUrl.getShortName()).thenReturn(GOOGLE_SEARCH_ENGINE);

        when(mBraveTemplateUrl.getShortName()).thenReturn(OnboardingPrefManager.BRAVE);
        when(mBraveTemplateUrl.getKeyword()).thenReturn(BRAVE_KEYWORD);
        doReturn(Arrays.asList(mDefaultTemplateUrl, mBraveTemplateUrl))
                .when(mTemplateUrlService)
                .getTemplateUrls();
        doAnswer(
                        invocation -> {
                            ((Runnable) invocation.getArgument(0)).run();
                            return null;
                        })
                .when(mTemplateUrlService)
                .runWhenLoaded(org.mockito.ArgumentMatchers.any(Runnable.class));
    }

    @After
    public void tearDown() {
        // Clean up preferences after each test
        ChromeSharedPreferences.getInstance()
                .removeKey(BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME);
        ChromeSharedPreferences.getInstance()
                .removeKey(BraveSearchEngineAdapter.PRIVATE_DSE_SHORTNAME);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.SEARCH_CHOICE_SCREEN_INSTALL);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.DEFAULT_SEARCH_ENGINE_CHANGED);
    }

    @Test
    @SmallTest
    public void testDefaultSearchEngineIsGoogleWithoutSearchChoiceFlag() {
        // When SEARCH_CHOICE_SCREEN_INSTALL flag is not set (default behavior)
        // the default search engine should be whatever the system default is (Google in this mock)
        BraveSearchEngineUtils.initializeDSEPrefsForTesting(mProfile);

        assertEquals(
                GOOGLE_SEARCH_ENGINE,
                ChromeSharedPreferences.getInstance()
                        .readString(BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, ""));
        assertEquals(
                GOOGLE_SEARCH_ENGINE,
                ChromeSharedPreferences.getInstance()
                        .readString(BraveSearchEngineAdapter.PRIVATE_DSE_SHORTNAME, ""));
    }

    @Test
    @SmallTest
    public void testDefaultSearchEngineIsBraveWithSearchChoiceFlag() {
        // When SEARCH_CHOICE_SCREEN_INSTALL flag is set to true
        // the default search engine should be Brave Search
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.SEARCH_CHOICE_SCREEN_INSTALL, true);

        BraveSearchEngineUtils.initializeDSEPrefsForTesting(mProfile);

        assertEquals(
                OnboardingPrefManager.BRAVE,
                ChromeSharedPreferences.getInstance()
                        .readString(BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, ""));
        assertEquals(
                OnboardingPrefManager.BRAVE,
                ChromeSharedPreferences.getInstance()
                        .readString(BraveSearchEngineAdapter.PRIVATE_DSE_SHORTNAME, ""));
    }

    @Test
    @SmallTest
    public void testDSEPrefsNotOverwrittenOnSubsequentCalls() {
        // First initialization without the flag
        BraveSearchEngineUtils.initializeDSEPrefsForTesting(mProfile);

        assertEquals(
                GOOGLE_SEARCH_ENGINE,
                ChromeSharedPreferences.getInstance()
                        .readString(BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, ""));

        // Now set the flag and call again - should NOT change the DSE
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.SEARCH_CHOICE_SCREEN_INSTALL, true);

        BraveSearchEngineUtils.initializeDSEPrefsForTesting(mProfile);

        // Should still be Google because DSE prefs were already initialized
        assertEquals(
                GOOGLE_SEARCH_ENGINE,
                ChromeSharedPreferences.getInstance()
                        .readString(BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, ""));
    }

    @Test
    @SmallTest
    public void testInitializeOnProfileAddedSkipsWhenAlreadyInitialized() {
        ChromeSharedPreferences.getInstance()
                .writeString(BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, GOOGLE_SEARCH_ENGINE);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.SEARCH_CHOICE_SCREEN_INSTALL, true);

        BraveSearchEngineUtils.initializeOnProfileAdded(mProfile);

        // The DSE was already reconciled once, so the stored engine must be left alone.
        assertEquals(
                GOOGLE_SEARCH_ENGINE,
                ChromeSharedPreferences.getInstance()
                        .readString(BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, ""));
        verify(mTemplateUrlService, never()).setSearchEngine(BRAVE_KEYWORD);
    }

    @Test
    @SmallTest
    public void testInitializeOnProfileAddedIgnoresOffTheRecordProfile() {
        when(mProfile.isOffTheRecord()).thenReturn(true);

        BraveSearchEngineUtils.initializeOnProfileAdded(mProfile);

        assertEquals(
                "",
                ChromeSharedPreferences.getInstance()
                        .readString(BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, ""));
    }

    @Test
    @SmallTest
    public void testApplySearchChoiceScreenDefaultKeepsUserSelection() {
        // The referrer fetch is retried on later launches, by which point the user may have
        // chosen an engine. That selection must win over the Search Choice Screen.
        BraveSearchEngineUtils.initializeDSEPrefsForTesting(mProfile);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.DEFAULT_SEARCH_ENGINE_CHANGED, true);

        BraveSearchEngineUtils.applySearchChoiceScreenDefault(mProfile);

        assertEquals(
                GOOGLE_SEARCH_ENGINE,
                ChromeSharedPreferences.getInstance()
                        .readString(BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, ""));
        verify(mTemplateUrlService, never()).setSearchEngine(BRAVE_KEYWORD);
    }

    @Test
    @SmallTest
    public void testNullProfileIsIgnored() {
        BraveSearchEngineUtils.initializeOnProfileAdded(null);
        BraveSearchEngineUtils.applySearchChoiceScreenDefault(null);

        assertEquals(
                "",
                ChromeSharedPreferences.getInstance()
                        .readString(BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, ""));
        verify(mTemplateUrlService, never()).setSearchEngine(BRAVE_KEYWORD);
    }
}
