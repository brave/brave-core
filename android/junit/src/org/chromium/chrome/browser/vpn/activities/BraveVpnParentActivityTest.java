// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.vpn.activities;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;

import org.chromium.base.test.BaseRobolectricTestRunner;

@RunWith(BaseRobolectricTestRunner.class)
public class BraveVpnParentActivityTest {

    @Test
    @SmallTest
    public void testShouldBringToTop_TrueForProfileActivity() {
        BraveVpnProfileActivity activity =
                Robolectric.buildActivity(BraveVpnProfileActivity.class).create().get();
        assertTrue(
                "shouldBringChromeToTop should return true for BraveVpnProfileActivity",
                activity.shouldBringChromeToTop());
    }

    @Test
    @SmallTest
    public void testShouldBringToTop_FalseForCountrySelection() {
        VpnServerSelectionActivity activity =
                Robolectric.buildActivity(VpnServerSelectionActivity.class).create().get();
        assertFalse(
                "shouldBringChromeToTop should return false for VpnServerSelectionActivity",
                activity.shouldBringChromeToTop());
    }

    @Test
    @SmallTest
    public void testShouldBringToTop_FalseForCitySelection() {
        VpnServerActivity activity =
                Robolectric.buildActivity(VpnServerActivity.class).create().get();
        assertFalse(
                "shouldBringChromeToTop should return false for VpnServerActivity",
                activity.shouldBringChromeToTop());
    }
}
