/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.referrer;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveReferrerTest {
    @Test
    @SmallTest
    public void returnsExactUrpc() {
        String result =
                BraveReferrer.getReferralCodeForTesting(
                        "utm_source=TESTSOURCE&gclid=GCLIDTEST&urpc=URPCTEST");

        assertEquals("URPCTEST", result);
    }

    @Test
    @SmallTest
    public void returnsPlayStoreGclidCode() {
        String result =
                BraveReferrer.getReferralCodeForTesting("utm_source=TESTSOURCE&gclid=GCLIDTEST");

        assertEquals("UAC001", result);
    }

    @Test
    @SmallTest
    public void returnsPlayStoreGbraidCode() {
        String result =
                BraveReferrer.getReferralCodeForTesting("utm_source=TESTSOURCE&gbraid=GBRAIDTEST");

        assertEquals("UAC003", result);
    }

    @Test
    @SmallTest
    public void returnsPlayStoreGclidAndGbraidCode() {
        String result =
                BraveReferrer.getReferralCodeForTesting(
                        "utm_source=TESTSOURCE&gclid=GCLIDTEST&gbraid=GBRAIDTEST");

        assertEquals("UAC004", result);
    }

    @Test
    @SmallTest
    public void returnsGoogleSearchCode() {
        String result =
                BraveReferrer.getReferralCodeForTesting(
                        "utm_source=TESTSOURCE&utm_campaign=gclid%3DGCLIDTEST");

        assertEquals("UAC002", result);
    }

    @Test
    @SmallTest
    public void returnsSearchChoiceScreenCode() {
        String result =
                BraveReferrer.getReferralCodeForTesting(
                        "utm_source=eea-search-choice&gclid=GCLIDTEST");

        assertEquals("SCS001", result);
    }

    @Test
    @SmallTest
    public void returnsBrowserChoiceScreenCode() {
        String result = BraveReferrer.getReferralCodeForTesting("utm_source=eea-browser-choice");

        assertEquals("BCS001", result);
    }

    @Test
    @SmallTest
    public void returnsNullWhenNothingMatches() {
        String result = BraveReferrer.getReferralCodeForTesting("utm_source=TESTSOURCE");

        assertNull(result);
    }
}
