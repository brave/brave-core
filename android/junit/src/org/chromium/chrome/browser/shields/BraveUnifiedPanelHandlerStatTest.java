/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.tab.Tab;

import java.util.ArrayList;

/**
 * Unit tests for stat-tracking logic in {@link BraveUnifiedPanelHandler}. Exercises addStat,
 * removeStat, clearBraveShieldsCount, and the various count/list accessors.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveUnifiedPanelHandlerStatTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    private static final int TAB_ID = 42;
    private static final int UNKNOWN_TAB_ID = 999;

    @Mock private Tab mIncognitoTab;

    private BraveUnifiedPanelHandler mHandler;

    @Before
    public void setUp() {
        mHandler = spy(new BraveUnifiedPanelHandler(ContextUtils.getApplicationContext()));
        mHandler.setCurrentTabForTesting(mIncognitoTab);
    }

    @Test
    public void testAddStat_adsBlocked() {
        mHandler.addStat(TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS, null);
        mHandler.addStat(TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS, null);

        assertEquals(2, mHandler.getAdsBlockedCount(TAB_ID));
        assertEquals(0, mHandler.getTrackersBlockedCount(TAB_ID));
    }

    @Test
    public void testAddStat_trackersBlocked() {
        mHandler.addStat(TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS, null);

        assertEquals(1, mHandler.getTrackersBlockedCount(TAB_ID));
        assertEquals(0, mHandler.getAdsBlockedCount(TAB_ID));
    }

    @Test
    public void testAddStat_scriptsBlocked() {
        mHandler.addStat(TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_JAVASCRIPTS, null);

        assertEquals(1, mHandler.getTotalBlockedCount(TAB_ID));
        assertEquals(0, mHandler.getAdsBlockedCount(TAB_ID));
        assertEquals(0, mHandler.getTrackersBlockedCount(TAB_ID));
    }

    @Test
    public void testAddStat_fingerprintsBlocked() {
        mHandler.addStat(
                TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING, null);

        assertEquals(1, mHandler.getTotalBlockedCount(TAB_ID));
    }

    @Test
    public void testAddStat_nullBlockTypeIsIgnored() {
        mHandler.addStat(TAB_ID, null, null);

        assertEquals(0, mHandler.getTotalBlockedCount(TAB_ID));
    }

    @Test
    public void testAddStat_blockedUrlsRecordedForAds() {
        mHandler.addStat(
                TAB_ID,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS,
                "https://tracker.example.com/ad.js");

        ArrayList<String> urls = mHandler.getBlockedUrls(TAB_ID);
        assertEquals(1, urls.size());
        assertEquals("https://tracker.example.com/ad.js", urls.get(0));
    }

    @Test
    public void testAddStat_blockedUrlsRecordedForTrackers() {
        mHandler.addStat(
                TAB_ID,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS,
                "https://analytics.example.com/track");

        ArrayList<String> urls = mHandler.getBlockedUrls(TAB_ID);
        assertEquals(1, urls.size());
    }

    @Test
    public void testAddStat_emptySubResourceNotRecorded() {
        mHandler.addStat(TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS, "");

        assertEquals(1, mHandler.getAdsBlockedCount(TAB_ID));
        assertTrue(mHandler.getBlockedUrls(TAB_ID).isEmpty());
    }

    @Test
    public void testAddStat_blockedUrlsCappedAt50() {
        for (int i = 0; i < 60; i++) {
            mHandler.addStat(
                    TAB_ID,
                    BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS,
                    "https://tracker" + i + ".example.com");
        }

        assertEquals(60, mHandler.getAdsBlockedCount(TAB_ID));
        assertEquals(50, mHandler.getBlockedUrls(TAB_ID).size());
    }

    @Test
    public void testGetTotalBlockedCount_sumsAllCategories() {
        mHandler.addStat(TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS, null);
        mHandler.addStat(TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS, null);
        mHandler.addStat(TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_JAVASCRIPTS, null);
        mHandler.addStat(
                TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING, null);

        assertEquals(4, mHandler.getTotalBlockedCount(TAB_ID));
    }

    @Test
    public void testClearBraveShieldsCount_resetsAllCounters() {
        mHandler.addStat(TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS, "url");
        mHandler.addStat(TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS, "url");
        assertEquals(2, mHandler.getTotalBlockedCount(TAB_ID));

        mHandler.clearBraveShieldsCount(TAB_ID);

        assertEquals(0, mHandler.getTotalBlockedCount(TAB_ID));
        assertEquals(0, mHandler.getAdsBlockedCount(TAB_ID));
        assertTrue(mHandler.getBlockedUrls(TAB_ID).isEmpty());
    }

    @Test
    public void testRemoveStat_removesTabEntry() {
        mHandler.addStat(TAB_ID, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS, null);
        assertEquals(1, mHandler.getAdsBlockedCount(TAB_ID));

        mHandler.removeStat(TAB_ID);

        assertEquals(0, mHandler.getAdsBlockedCount(TAB_ID));
        assertEquals(0, mHandler.getTotalBlockedCount(TAB_ID));
    }

    @Test
    public void testGetters_returnDefaultsForUnknownTab() {
        assertEquals(0, mHandler.getAdsBlockedCount(UNKNOWN_TAB_ID));
        assertEquals(0, mHandler.getTrackersBlockedCount(UNKNOWN_TAB_ID));
        assertEquals(0, mHandler.getTotalBlockedCount(UNKNOWN_TAB_ID));
        assertTrue(mHandler.getBlockedUrls(UNKNOWN_TAB_ID).isEmpty());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_SHIELDS_ELEMENT_PICKER)
    public void shouldShowElementBlocker_normalTab_featureEnabled_shown() {
        when(mIncognitoTab.isIncognito()).thenReturn(false);
        // Shouldn't matter but set it to false anyway.
        doReturn(false).when(mHandler).getElementBlockerPrivateModeEnabled();
        assertTrue(mHandler.shouldShowElementBlocker());
    }

    @Test
    @DisableFeatures(BraveFeatureList.BRAVE_SHIELDS_ELEMENT_PICKER)
    public void shouldShowElementBlocker_normalTab_featureDisabled_shown() {
        when(mIncognitoTab.isIncognito()).thenReturn(false);
        // Ditto previous test, but inverse.
        doReturn(true).when(mHandler).getElementBlockerPrivateModeEnabled();
        assertFalse(mHandler.shouldShowElementBlocker());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_SHIELDS_ELEMENT_PICKER)
    public void shouldShowElementBlocker_privateTab_privatePrefEnabled_shown() {
        when(mIncognitoTab.isIncognito()).thenReturn(true);
        doReturn(true).when(mHandler).getElementBlockerPrivateModeEnabled();
        assertTrue(mHandler.shouldShowElementBlocker());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_SHIELDS_ELEMENT_PICKER)
    public void shouldShowElementBlocker_privateTab_privatePrefDisabled_notShown() {
        when(mIncognitoTab.isIncognito()).thenReturn(true);
        doReturn(false).when(mHandler).getElementBlockerPrivateModeEnabled();
        assertFalse(mHandler.shouldShowElementBlocker());
    }

    @Test
    @DisableFeatures(BraveFeatureList.BRAVE_SHIELDS_ELEMENT_PICKER)
    public void shouldShowElementBlocker_privateTab_featureDisabled_notShown() {
        when(mIncognitoTab.isIncognito()).thenReturn(true);
        doReturn(true).when(mHandler).getElementBlockerPrivateModeEnabled();
        assertFalse(mHandler.shouldShowElementBlocker());
    }

    @Test
    public void testMultipleTabs_isolateStats() {
        int tabA = 1;
        int tabB = 2;

        mHandler.addStat(tabA, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS, null);
        mHandler.addStat(tabA, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS, null);
        mHandler.addStat(tabB, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS, null);

        assertEquals(2, mHandler.getAdsBlockedCount(tabA));
        assertEquals(0, mHandler.getTrackersBlockedCount(tabA));
        assertEquals(0, mHandler.getAdsBlockedCount(tabB));
        assertEquals(1, mHandler.getTrackersBlockedCount(tabB));
    }
}
