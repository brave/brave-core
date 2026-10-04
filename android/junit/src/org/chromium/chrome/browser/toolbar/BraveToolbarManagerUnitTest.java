/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.lenient;

import android.content.res.Configuration;

import androidx.test.filters.SmallTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.url.GURL;
import org.chromium.url.JUnitTestGURLs;

/** Unit tests for {@link BraveToolbarManager}. */
@RunWith(BaseRobolectricTestRunner.class)
public final class BraveToolbarManagerUnitTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Tab mTab;

    /**
     * Lenient because the decision short-circuits on the upstream verdict, so the URL is not always
     * read.
     */
    private void setTabUrl(GURL url) {
        lenient().doReturn(url).when(mTab).getUrl();
    }

    /**
     * On the NTP upstream always suppresses the long press menu, so this is the case that makes
     * "Move address bar to the bottom/top" reachable from Brave's new tab page.
     */
    @Test
    @SmallTest
    public void testNotSuppressedOnNewTabPage() {
        setTabUrl(JUnitTestGURLs.NTP_NATIVE_URL);
        assertFalse(
                BraveToolbarManager.shouldSuppressToolbarLongPressForTab(
                        /* suppressedByUpstream= */ true, mTab, /* isOmniboxFocused= */ false));
    }

    /** The WebUI flavour of the NTP url must be treated the same as the native one. */
    @Test
    @SmallTest
    public void testNotSuppressedOnWebUiNewTabPage() {
        setTabUrl(JUnitTestGURLs.NTP_URL);
        assertFalse(
                BraveToolbarManager.shouldSuppressToolbarLongPressForTab(
                        /* suppressedByUpstream= */ true, mTab, /* isOmniboxFocused= */ false));
    }

    /** Off the NTP the upstream decision must be left untouched. */
    @Test
    @SmallTest
    public void testUpstreamSuppressionKeptOnRegularPage() {
        setTabUrl(JUnitTestGURLs.EXAMPLE_URL);
        assertTrue(
                BraveToolbarManager.shouldSuppressToolbarLongPressForTab(
                        /* suppressedByUpstream= */ true, mTab, /* isOmniboxFocused= */ false));
    }

    /** Being on the NTP must never turn a non-suppressed state into a suppressed one. */
    @Test
    @SmallTest
    public void testNotSuppressedStaysNotSuppressedOnNewTabPage() {
        setTabUrl(JUnitTestGURLs.NTP_NATIVE_URL);
        assertFalse(
                BraveToolbarManager.shouldSuppressToolbarLongPressForTab(
                        /* suppressedByUpstream= */ false, mTab, /* isOmniboxFocused= */ false));
    }

    /**
     * A focused address bar is being edited, so the long press menu must stay suppressed on the NTP
     * as well.
     */
    @Test
    @SmallTest
    public void testUpstreamSuppressionKeptOnFocusedAddressBar() {
        setTabUrl(JUnitTestGURLs.NTP_NATIVE_URL);
        assertTrue(
                BraveToolbarManager.shouldSuppressToolbarLongPressForTab(
                        /* suppressedByUpstream= */ true, mTab, /* isOmniboxFocused= */ true));
    }

    @Test
    @SmallTest
    public void testUpstreamSuppressionKeptWithoutTab() {
        assertTrue(
                BraveToolbarManager.shouldSuppressToolbarLongPressForTab(
                        /* suppressedByUpstream= */ true, null, /* isOmniboxFocused= */ false));
    }

    @Test
    @SmallTest
    public void testUpstreamSuppressionKeptWithoutTabUrl() {
        setTabUrl(null);
        assertTrue(
                BraveToolbarManager.shouldSuppressToolbarLongPressForTab(
                        /* suppressedByUpstream= */ true, mTab, /* isOmniboxFocused= */ false));
    }

    @Test
    @SmallTest
    public void testBottomControlsVisibleOnlyForUnfocusedPortraitOmnibox() {
        assertTrue(
                BraveToolbarManager.shouldShowBraveBottomControls(
                        /* isBottomControlsEnabled= */ true,
                        Configuration.ORIENTATION_PORTRAIT,
                        /* isOmniboxFocused= */ false));
        assertFalse(
                BraveToolbarManager.shouldShowBraveBottomControls(
                        /* isBottomControlsEnabled= */ true,
                        Configuration.ORIENTATION_PORTRAIT,
                        /* isOmniboxFocused= */ true));
        assertFalse(
                BraveToolbarManager.shouldShowBraveBottomControls(
                        /* isBottomControlsEnabled= */ true,
                        Configuration.ORIENTATION_LANDSCAPE,
                        /* isOmniboxFocused= */ false));
        assertFalse(
                BraveToolbarManager.shouldShowBraveBottomControls(
                        /* isBottomControlsEnabled= */ false,
                        Configuration.ORIENTATION_PORTRAIT,
                        /* isOmniboxFocused= */ false));
    }
}
