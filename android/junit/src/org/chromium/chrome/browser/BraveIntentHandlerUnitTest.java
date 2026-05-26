/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doReturn;

import android.content.Intent;
import android.net.Uri;

import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.searchwidget.SearchWidgetProvider;
import org.chromium.url.GURL;

@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveIntentHandlerUnitTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock IntentHandler.Natives mIntentHandlerNativesMock;

    @Before
    public void setUp() {
        // Return true (URL is valid) so the native check doesn't block anything; our
        // Brave-specific brave:// guard is what the tests below actually exercise.
        doReturn(true).when(mIntentHandlerNativesMock).validateLaunchUrl(any(GURL.class));
        IntentHandlerJni.setInstanceForTesting(mIntentHandlerNativesMock);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_widgetSearch_rewritesSourceToAndroidWidget() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(Uri.parse("https://search.brave.com/search?q=test&source=android"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals("https://search.brave.com/search?q=test&source=android-widget", result);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_nonWidgetSearch_keepsAndroidSource() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(Uri.parse("https://search.brave.com/search?q=test&source=android"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, false);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals("https://search.brave.com/search?q=test&source=android", result);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_widgetSearch_nonBraveSearchHost_isUnchanged() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(Uri.parse("https://example.com/search?q=test&source=android"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals("https://example.com/search?q=test&source=android", result);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_widgetSearch_preservesLiteralPlusInQuery() {
        // Regression: a search for "C++ tutorial" arrives as
        // "q=C%2B%2B+tutorial". Going through Uri.getQueryParameters() /
        // appendQueryParameter() conflates the literal '+' (%2B) with the
        // form-encoded space ('+'), corrupting the query. The encoded query
        // bytes must pass through unchanged.
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(
                Uri.parse("https://search.brave.com/search?q=C%2B%2B+tutorial&source=android"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals(
                "https://search.brave.com/search?q=C%2B%2B+tutorial&source=android-widget",
                result);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_widgetSearch_preservesFormEncodedSpaceInQuery() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(
                Uri.parse("https://search.brave.com/search?q=hello+world&source=android"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals(
                "https://search.brave.com/search?q=hello+world&source=android-widget", result);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_widgetSearch_preservesPercentEncodedSpaceInQuery() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(
                Uri.parse("https://search.brave.com/search?q=hello%20world&source=android"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals(
                "https://search.brave.com/search?q=hello%20world&source=android-widget",
                result);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_widgetSearch_preservesUnicodeInQuery() {
        // %E6%97%A5%E6%9C%AC%E8%AA%9E is UTF-8 percent-encoded "日本語".
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(
                Uri.parse(
                        "https://search.brave.com/search"
                                + "?q=%E6%97%A5%E6%9C%AC%E8%AA%9E&source=android"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals(
                "https://search.brave.com/search"
                        + "?q=%E6%97%A5%E6%9C%AC%E8%AA%9E&source=android-widget",
                result);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_widgetSearch_sourceBeforeQuery_rewritesOnlySource() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(Uri.parse("https://search.brave.com/search?source=android&q=test"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals("https://search.brave.com/search?source=android-widget&q=test", result);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_widgetSearch_multipleParams_rewritesOnlySource() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(
                Uri.parse(
                        "https://search.brave.com/search"
                                + "?q=test&source=android&tf=pw&safesearch=moderate"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals(
                "https://search.brave.com/search"
                        + "?q=test&source=android-widget&tf=pw&safesearch=moderate",
                result);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_widgetSearch_sourceNotAndroid_isUnchanged() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(Uri.parse("https://search.brave.com/search?q=test&source=web"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals("https://search.brave.com/search?q=test&source=web", result);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_widgetSearch_noSourceParam_isUnchanged() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(Uri.parse("https://search.brave.com/search?q=test"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals("https://search.brave.com/search?q=test", result);
    }

    @Test
    @SmallTest
    public void extractUrlFromIntent_widgetSearch_reservedCharsInQuery_passThrough() {
        // Ampersand and equals inside the q value are percent-encoded by the
        // sender as %26 and %3D. They must survive untouched, otherwise the
        // server would re-split the query and misinterpret it.
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(
                Uri.parse(
                        "https://search.brave.com/search?q=a%26b%3Dc&source=android"));
        intent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);

        String result = BraveIntentHandler.extractUrlFromIntent(intent);

        assertEquals(
                "https://search.brave.com/search?q=a%26b%3Dc&source=android-widget", result);
    }

    @Test
    @SmallTest
    public void intentHasUnsafeInternalScheme_braveScheme_isBlocked() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.addCategory(Intent.CATEGORY_BROWSABLE);
        assertTrue(BraveIntentHandler.intentHasUnsafeUrl("brave://flags/", intent));
    }

    @Test
    @SmallTest
    public void intentHasUnsafeInternalScheme_braveScheme_mixedCase_isBlocked() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.addCategory(Intent.CATEGORY_DEFAULT);
        assertTrue(BraveIntentHandler.intentHasUnsafeUrl("Brave://flags/", intent));
    }

    @Test
    @SmallTest
    public void intentHasUnsafeInternalScheme_httpsScheme_isAllowed() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.addCategory(Intent.CATEGORY_BROWSABLE);
        assertFalse(BraveIntentHandler.intentHasUnsafeUrl("https://example.com/", intent));
    }
}
