/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.view.View;
import android.view.ViewStub;

import androidx.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;

/**
 * Tests that the toolbar's Google sign-in button is never created. A TopToolbarCoordinator plaster
 * leaves the button's ViewStub unused, so no SigninButtonCoordinator is constructed.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@Batch(Batch.PER_CLASS)
public class BraveToolbarSigninButtonTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    @Before
    public void setUp() {
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    @Test
    @MediumTest
    public void testSigninButtonStubStaysUnused() {
        // The NTP is the state upstream shows the button in, and the navigation to it is what
        // pushes the toolbar through updateButtonVisibility().
        mActivityTestRule.loadUrl("chrome://newtab/");

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    View toolbar = mActivityTestRule.getActivity().findViewById(R.id.toolbar);
                    assertNotNull("The top toolbar is missing.", toolbar);

                    assertNull(
                            "The toolbar sign-in button was inflated.",
                            toolbar.findViewById(R.id.signin_button));
                    // ViewStub.inflate() takes the stub out of the hierarchy, so the stub still
                    // being there is what proves nothing inflated it.
                    assertTrue(
                            "The toolbar sign-in button stub is gone.",
                            toolbar.findViewById(R.id.signin_button_stub) instanceof ViewStub);
                });
    }
}
