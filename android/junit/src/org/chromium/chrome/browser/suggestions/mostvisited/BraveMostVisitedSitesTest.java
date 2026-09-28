/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.suggestions.mostvisited;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentCaptor.forClass;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.suggestions.SiteSuggestion;
import org.chromium.chrome.browser.suggestions.tile.TileSource;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for {@link BraveMostVisitedSites}.
 *
 * <p>Tests the Java-level tile filtering that separates "Show shortcuts" (custom links) from "Show
 * frequently visited" (top sites) without relying on C++ {@code EnableTileTypes()} changes.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveMostVisitedSitesTest {

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private MostVisitedSites mMockBridge;
    @Mock private MostVisitedSites.Observer mMockOuterObserver;

    private BraveMostVisitedSites mSubject;
    private MostVisitedSites.Observer mFilteringObserver;

    @Before
    public void setUp() {
        mSubject = new BraveMostVisitedSites(mMockBridge);
        mSubject.setObserver(mMockOuterObserver, 8);

        // Capture the FilteringObserver that BraveMostVisitedSites passed to the bridge.
        ArgumentCaptor<MostVisitedSites.Observer> captor =
                forClass(MostVisitedSites.Observer.class);
        verify(mMockBridge).setObserver(captor.capture(), eq(8));
        mFilteringObserver = captor.getValue();
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_NTP_TOP_SITES_DISPLAY_MODE);
        mSubject.destroy();
    }

    @Test
    public void testFilterForMode_shortcuts_keepsOnlyCustomLinks() {
        List<SiteSuggestion> mixed = mixedSuggestions();
        List<SiteSuggestion> result =
                BraveMostVisitedSites.filterForMode(mixed, NtpUtil.TOP_SITES_MODE_SHORTCUTS);
        assertTrue(
                "All filtered tiles must be CUSTOM_LINKS",
                allMatch(result, TileSource.CUSTOM_LINKS));
        assertEquals(1, result.size());
    }

    @Test
    public void testFilterForMode_frequent_keepsOnlyTopSites() {
        List<SiteSuggestion> mixed = mixedSuggestions();
        List<SiteSuggestion> result =
                BraveMostVisitedSites.filterForMode(mixed, NtpUtil.TOP_SITES_MODE_FREQUENT);
        assertTrue("All filtered tiles must be TOP_SITES", allMatch(result, TileSource.TOP_SITES));
        assertEquals(2, result.size());
    }

    @Test
    public void testFilterForMode_shortcuts_emptyWhenNoCustomLinks() {
        List<SiteSuggestion> topSitesOnly =
                List.of(makeSuggestion("Example", "https://example.com", TileSource.TOP_SITES));
        List<SiteSuggestion> result =
                BraveMostVisitedSites.filterForMode(topSitesOnly, NtpUtil.TOP_SITES_MODE_SHORTCUTS);
        assertTrue("Shortcuts mode with no custom links must be empty", result.isEmpty());
    }

    @Test
    public void testFilterForMode_frequent_emptyWhenNoTopSites() {
        List<SiteSuggestion> customOnly =
                List.of(makeSuggestion("My Site", "https://mysite.com", TileSource.CUSTOM_LINKS));
        List<SiteSuggestion> result =
                BraveMostVisitedSites.filterForMode(customOnly, NtpUtil.TOP_SITES_MODE_FREQUENT);
        assertTrue("Frequent mode with no top-sites must be empty", result.isEmpty());
    }

    @Test
    public void testFilterForMode_ignoresOtherSources() {
        List<SiteSuggestion> withPopular = new ArrayList<>();
        withPopular.add(makeSuggestion("Popular", "https://popular.com", TileSource.POPULAR));
        withPopular.add(makeSuggestion("Top", "https://top.com", TileSource.TOP_SITES));
        withPopular.add(makeSuggestion("Custom", "https://custom.com", TileSource.CUSTOM_LINKS));

        List<SiteSuggestion> shortcuts =
                BraveMostVisitedSites.filterForMode(withPopular, NtpUtil.TOP_SITES_MODE_SHORTCUTS);
        assertEquals(1, shortcuts.size());
        assertEquals(TileSource.CUSTOM_LINKS, shortcuts.get(0).source);

        List<SiteSuggestion> frequent =
                BraveMostVisitedSites.filterForMode(withPopular, NtpUtil.TOP_SITES_MODE_FREQUENT);
        assertEquals(1, frequent.size());
        assertEquals(TileSource.TOP_SITES, frequent.get(0).source);
    }

    @Test
    public void testBridgeCallbackFiltersForCurrentMode_defaultShortcutsMode() {
        mFilteringObserver.onSiteSuggestionsAvailable(false, mixedSuggestions());

        ArgumentCaptor<List<SiteSuggestion>> captor = captorForSuggestions();
        verify(mMockOuterObserver).onSiteSuggestionsAvailable(eq(false), captor.capture());
        List<SiteSuggestion> delivered = captor.getValue();
        assertTrue(allMatch(delivered, TileSource.CUSTOM_LINKS));
        assertEquals(1, delivered.size());
    }

    @Test
    public void testBridgeCallbackFiltersForCurrentMode_frequentMode() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);
        mFilteringObserver.onSiteSuggestionsAvailable(true, mixedSuggestions());

        ArgumentCaptor<List<SiteSuggestion>> captor = captorForSuggestions();
        verify(mMockOuterObserver).onSiteSuggestionsAvailable(eq(true), captor.capture());
        List<SiteSuggestion> delivered = captor.getValue();
        assertTrue(allMatch(delivered, TileSource.TOP_SITES));
        assertEquals(2, delivered.size());
    }

    @Test
    public void testModeChangeRefiltersCachedTiles() {
        // Start in shortcuts mode (default), receive tiles.
        mFilteringObserver.onSiteSuggestionsAvailable(false, mixedSuggestions());

        // Now switch to frequent mode — outer observer should be called again without any new
        // bridge callback, using the cached raw tile list.
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);

        ArgumentCaptor<List<SiteSuggestion>> captor = captorForSuggestions();
        // Called once initially (shortcuts) and once on mode change (frequent).
        verify(mMockOuterObserver, times(2))
                .onSiteSuggestionsAvailable(anyBoolean(), captor.capture());
        List<SiteSuggestion> secondCall = captor.getAllValues().get(1);
        assertTrue(
                "After mode change, only TOP_SITES tiles",
                allMatch(secondCall, TileSource.TOP_SITES));
        assertEquals(2, secondCall.size());
    }

    @Test
    public void testModeChangeWithNoCachedTiles_notifiesWithEmptyList() {
        // No bridge callback yet; cache is empty.
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);

        // Observer is notified with an empty list immediately (the C++ callback will arrive later).
        ArgumentCaptor<List<SiteSuggestion>> captor = captorForSuggestions();
        verify(mMockOuterObserver).onSiteSuggestionsAvailable(eq(true), captor.capture());
        assertTrue(captor.getValue().isEmpty());
    }

    @Test
    public void testIconCallbackPassedThrough() {
        GURL url = new GURL("https://example.com");
        mFilteringObserver.onIconMadeAvailable(url);
        verify(mMockOuterObserver).onIconMadeAvailable(url);
    }

    @Test
    public void testSwitchingModesTwice_correctFinalState() {
        // Receive tiles then flip mode twice: shortcuts → frequent → shortcuts.
        mFilteringObserver.onSiteSuggestionsAvailable(false, mixedSuggestions());

        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_SHORTCUTS);

        ArgumentCaptor<List<SiteSuggestion>> captor = captorForSuggestions();
        verify(mMockOuterObserver, times(3))
                .onSiteSuggestionsAvailable(anyBoolean(), captor.capture());

        List<SiteSuggestion> thirdCall = captor.getAllValues().get(2);
        assertTrue(
                "Final mode is SHORTCUTS, expect CUSTOM_LINKS only",
                allMatch(thirdCall, TileSource.CUSTOM_LINKS));
    }

    private static List<SiteSuggestion> mixedSuggestions() {
        List<SiteSuggestion> list = new ArrayList<>();
        list.add(
                makeSuggestion(
                        "My shortcut", "https://shortcut.example.com", TileSource.CUSTOM_LINKS));
        list.add(makeSuggestion("News", "https://news.example.com", TileSource.TOP_SITES));
        list.add(makeSuggestion("Video", "https://video.example.com", TileSource.TOP_SITES));
        return list;
    }

    private static SiteSuggestion makeSuggestion(String title, String url, int source) {
        return new SiteSuggestion(
                title, new GURL(url), /* titleSource= */ 0, source, /* sectionType= */ 0);
    }

    private static boolean allMatch(List<SiteSuggestion> list, int source) {
        for (SiteSuggestion s : list) {
            if (s.source != source) return false;
        }
        return true;
    }

    @SuppressWarnings("unchecked")
    private static ArgumentCaptor<List<SiteSuggestion>> captorForSuggestions() {
        return (ArgumentCaptor<List<SiteSuggestion>>) (ArgumentCaptor<?>) forClass(List.class);
    }
}
