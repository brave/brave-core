/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_origin;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.content.Intent;
import android.net.Uri;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/** Unit tests for {@link BraveOriginDeepLinkHandler}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveOriginDeepLinkHandlerTest {
    private static final String CANONICAL_URL =
            "https://brave.com/" + BraveOriginDeepLinkHandler.PATH_TOKEN;

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_DEFERRED_DEEPLINK_ORIGIN_PROMO, false);
    }

    // consumeFromIntent — happy path

    @Test
    @SmallTest
    public void consumeFromIntent_canonicalUrl_returnsTrueAndNeutralizesIntent() {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(CANONICAL_URL));

        assertTrue(BraveOriginDeepLinkHandler.consumeFromIntent(intent));

        // Action becomes MAIN and data is cleared so upstream URL handling skips this intent.
        assertEquals(Intent.ACTION_MAIN, intent.getAction());
        assertNull(intent.getData());
    }

    @Test
    @SmallTest
    public void consumeFromIntent_uppercasePath_returnsTrue() {
        // Path matching is case-insensitive per the implementation.
        Intent intent =
                new Intent(
                        Intent.ACTION_VIEW,
                        Uri.parse("https://brave.com/DEEPLINK-ANDROID-ORIGIN-PROMO"));

        assertTrue(BraveOriginDeepLinkHandler.consumeFromIntent(intent));
    }

    @Test
    @SmallTest
    public void consumeFromIntent_canonicalUrlWithQuery_returnsTrue() {
        // Query parameters don't affect last path segment.
        Intent intent =
                new Intent(Intent.ACTION_VIEW, Uri.parse(CANONICAL_URL + "?utm_source=play"));

        assertTrue(BraveOriginDeepLinkHandler.consumeFromIntent(intent));
    }

    // consumeFromIntent — rejection paths

    @Test
    @SmallTest
    public void consumeFromIntent_nullIntent_returnsFalse() {
        assertFalse(BraveOriginDeepLinkHandler.consumeFromIntent(null));
    }

    @Test
    @SmallTest
    public void consumeFromIntent_wrongAction_returnsFalse() {
        Intent intent = new Intent(Intent.ACTION_SEND, Uri.parse(CANONICAL_URL));

        assertFalse(BraveOriginDeepLinkHandler.consumeFromIntent(intent));
        // Intent should not be mutated.
        assertEquals(Intent.ACTION_SEND, intent.getAction());
        assertEquals(Uri.parse(CANONICAL_URL), intent.getData());
    }

    @Test
    @SmallTest
    public void consumeFromIntent_nullData_returnsFalse() {
        Intent intent = new Intent(Intent.ACTION_VIEW);

        assertFalse(BraveOriginDeepLinkHandler.consumeFromIntent(intent));
    }

    @Test
    @SmallTest
    public void consumeFromIntent_unrelatedPath_returnsFalse() {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://brave.com/blog"));

        assertFalse(BraveOriginDeepLinkHandler.consumeFromIntent(intent));
    }

    @Test
    @SmallTest
    public void consumeFromIntent_partialMatch_returnsFalse() {
        // Last segment must be an exact (case-insensitive) match — substring matches don't count.
        Intent intent =
                new Intent(
                        Intent.ACTION_VIEW,
                        Uri.parse("https://brave.com/deeplink-android-origin-promo-something"));

        assertFalse(BraveOriginDeepLinkHandler.consumeFromIntent(intent));
    }

    // consumeDeferred

    @Test
    @SmallTest
    public void consumeDeferred_prefUnset_returnsFalse() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_DEFERRED_DEEPLINK_ORIGIN_PROMO, false);

        assertFalse(BraveOriginDeepLinkHandler.consumeDeferred());
    }

    @Test
    @SmallTest
    public void consumeDeferred_prefSet_returnsTrueAndClearsPref() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_DEFERRED_DEEPLINK_ORIGIN_PROMO, true);

        assertTrue(BraveOriginDeepLinkHandler.consumeDeferred());
        // Pref should be cleared so subsequent drains don't double-fire.
        assertFalse(
                ChromeSharedPreferences.getInstance()
                        .readBoolean(
                                BravePreferenceKeys.BRAVE_DEFERRED_DEEPLINK_ORIGIN_PROMO, false));
    }

    @Test
    @SmallTest
    public void consumeDeferred_calledTwice_secondCallReturnsFalse() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_DEFERRED_DEEPLINK_ORIGIN_PROMO, true);

        assertTrue(BraveOriginDeepLinkHandler.consumeDeferred());
        assertFalse(BraveOriginDeepLinkHandler.consumeDeferred());
    }
}
