/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.quick_search_engines.utils;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.brave.browser.quick_search_engines.settings.QuickSearchEnginesModel;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.regional_capabilities.RegionalCapabilitiesServiceFactory;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.omnibox.OmniboxFeatureList;
import org.chromium.components.regional_capabilities.RegionalCapabilitiesService;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Unit tests for {@link QuickSearchEnginesUtil}, focused on the filtering of OpenSearch
 * auto-discovered "recently visited" search engines added in the quick search engines list.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
// Disable so the upstream SearchEngineAdapter#sortAndFilterUnnecessaryTemplateUrl keeps
// RECENT engines in the list, otherwise it would strip them before the new filter runs.
@DisableFeatures(OmniboxFeatureList.OMNIBOX_SITE_SEARCH)
public class QuickSearchEnginesUtilTest {
    private static final String GOOGLE_KEYWORD = ":g";
    private static final String BRAVE_KEYWORD = ":br";
    private static final String BING_KEYWORD = ":b";
    private static final String CUSTOM_KEYWORD = "mycustom.com";
    private static final String AUTO_DISCOVERED_KEYWORD = "recent.example.com";

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;
    @Mock private TemplateUrlService mTemplateUrlService;
    @Mock private RegionalCapabilitiesService mRegionalCapabilities;

    private long mNextNativePtr = 1L;

    @Before
    public void setUp() {
        clearPrefs();

        TemplateUrlServiceFactory.setInstanceForTesting(mTemplateUrlService);
        RegionalCapabilitiesServiceFactory.setInstanceForTesting(mRegionalCapabilities);
        when(mRegionalCapabilities.isInEeaCountry()).thenReturn(false);
    }

    @After
    public void tearDown() {
        clearPrefs();
    }

    private void clearPrefs() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES_PREVIOUS_DSE);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.CUSTOM_SEARCH_ENGINE_KEYWORDS);
    }

    private TemplateUrl mockTemplateUrl(String shortName, String keyword, boolean prepopulated) {
        TemplateUrl t = mock(TemplateUrl.class);
        when(t.getShortName()).thenReturn(shortName);
        when(t.getKeyword()).thenReturn(keyword);
        when(t.getIsPrepopulated()).thenReturn(prepopulated);
        when(t.getURL()).thenReturn("https://example.com/?q={searchTerms}&k=" + keyword);
        when(t.getNativePtr()).thenReturn(mNextNativePtr++);
        // Recent timestamp so upstream sortAndFilterUnnecessaryTemplateUrl keeps non-prepop
        // engines as "recent" (within the 2-day display window).
        when(t.getLastVisitedTime()).thenReturn(System.currentTimeMillis());
        return t;
    }

    private void setCustomKeywords(String... keywords) {
        Set<String> set = new HashSet<>(Arrays.asList(keywords));
        ChromeSharedPreferences.getInstance()
                .writeStringSet(BravePreferenceKeys.CUSTOM_SEARCH_ENGINE_KEYWORDS, set);
    }

    private List<String> getKeywordsForSettings() {
        return QuickSearchEnginesUtil.getQuickSearchEnginesForSettings(mProfile).stream()
                .map(QuickSearchEnginesModel::getKeyword)
                .collect(Collectors.toList());
    }

    @Test
    @SmallTest
    public void testFiltersOutAutoDiscoveredEngines() {
        TemplateUrl google = mockTemplateUrl("Google", GOOGLE_KEYWORD, true);
        TemplateUrl brave = mockTemplateUrl("Brave", BRAVE_KEYWORD, true);
        TemplateUrl autoDiscovered = mockTemplateUrl("Recent Site", AUTO_DISCOVERED_KEYWORD, false);

        when(mTemplateUrlService.getTemplateUrls())
                .thenReturn(new ArrayList<>(Arrays.asList(google, brave, autoDiscovered)));
        when(mTemplateUrlService.getDefaultSearchEngineTemplateUrl()).thenReturn(google);

        List<String> keywords = getKeywordsForSettings();

        assertTrue("Prepopulated Brave engine should be kept", keywords.contains(BRAVE_KEYWORD));
        assertFalse(
                "Auto-discovered non-prepopulated engine should be filtered out",
                keywords.contains(AUTO_DISCOVERED_KEYWORD));
    }

    @Test
    @SmallTest
    public void testKeepsUserAddedCustomEngines() {
        setCustomKeywords(CUSTOM_KEYWORD);

        TemplateUrl google = mockTemplateUrl("Google", GOOGLE_KEYWORD, true);
        TemplateUrl custom = mockTemplateUrl("My Custom", CUSTOM_KEYWORD, false);
        TemplateUrl autoDiscovered = mockTemplateUrl("Recent Site", AUTO_DISCOVERED_KEYWORD, false);

        when(mTemplateUrlService.getTemplateUrls())
                .thenReturn(new ArrayList<>(Arrays.asList(google, custom, autoDiscovered)));
        when(mTemplateUrlService.getDefaultSearchEngineTemplateUrl()).thenReturn(google);

        List<String> keywords = getKeywordsForSettings();

        assertTrue(
                "User-added custom engine should be kept even though it is not prepopulated",
                keywords.contains(CUSTOM_KEYWORD));
        assertFalse(
                "Auto-discovered non-prepopulated engine should still be filtered out",
                keywords.contains(AUTO_DISCOVERED_KEYWORD));
    }

    @Test
    @SmallTest
    public void testKeepsAllPrepopulatedEngines() {
        TemplateUrl google = mockTemplateUrl("Google", GOOGLE_KEYWORD, true);
        TemplateUrl brave = mockTemplateUrl("Brave", BRAVE_KEYWORD, true);
        TemplateUrl bing = mockTemplateUrl("Bing", BING_KEYWORD, true);

        when(mTemplateUrlService.getTemplateUrls())
                .thenReturn(new ArrayList<>(Arrays.asList(google, brave, bing)));
        when(mTemplateUrlService.getDefaultSearchEngineTemplateUrl()).thenReturn(google);

        List<String> keywords = getKeywordsForSettings();

        assertTrue(keywords.contains(BRAVE_KEYWORD));
        assertTrue(keywords.contains(BING_KEYWORD));
    }
}
