/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import static org.junit.Assert.assertEquals;

import android.content.Intent;
import android.net.Uri;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.searchwidget.SearchWidgetProvider;

@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveIntentHandlerUnitTest {
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
}
