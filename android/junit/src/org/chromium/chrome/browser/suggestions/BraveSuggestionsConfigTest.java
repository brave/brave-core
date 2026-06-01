/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.suggestions;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/** Verifies that Brave's plaster override of {@link SuggestionsConfig} is in effect. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveSuggestionsConfigTest {
    @Test
    @SmallTest
    public void testMaxTileCountIsOverridden() {
        // (4 rows x 4 columns - 1 for the plus button) via a plaster on SuggestionsConfig.java.
        Assert.assertEquals(15, SuggestionsConfig.MAX_TILE_COUNT);
    }
}
