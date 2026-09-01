/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import android.view.View;

import androidx.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.OmniboxTestUtils;
import org.chromium.ui.base.DeviceFormFactor;

/** Tests for the top toolbar while upstream's bottom bar carries Brave's button set. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@Batch(Batch.PER_CLASS)
@EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
@Restriction(DeviceFormFactor.PHONE)
public class BraveBottomBarToolbarTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private OmniboxTestUtils mOmnibox;

    @Before
    public void setUp() {
        mActivityTestRule.startMainActivityOnBlankPage();
        mOmnibox = new OmniboxTestUtils(mActivityTestRule.getActivity());
    }

    /**
     * The bottom bar carries the tab switcher and the app menu, so ToolbarPhone hides the top
     * toolbar's own. Brave's bottom controls are off in that configuration and must leave them
     * alone - their visibility handling used to put both back on every omnibox focus change, which
     * left a second tab switcher and app menu on top once the omnibox had been dismissed.
     */
    @Test
    @MediumTest
    public void testTopToolbarButtonsStayHiddenAcrossOmniboxFocus() {
        assertTopToolbarButtonsHidden("before focusing the omnibox");

        mOmnibox.requestFocus();
        // Dismisses the omnibox the way the back button does.
        mOmnibox.clearFocus();

        assertTopToolbarButtonsHidden("after dismissing the omnibox");
    }

    private void assertTopToolbarButtonsHidden(String when) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    View toolbar = mActivityTestRule.getActivity().findViewById(R.id.toolbar);
                    assertNotNull("The top toolbar is missing.", toolbar);

                    // Looked up on the toolbar rather than on the activity: the bottom bar holds a
                    // tab switcher button under the same id.
                    View tabSwitcherButton = toolbar.findViewById(R.id.tab_switcher_button);
                    assertNotNull(
                            "The top toolbar tab switcher button is missing.", tabSwitcherButton);
                    assertEquals(
                            "The top toolbar tab switcher button is shown " + when + ".",
                            View.GONE,
                            tabSwitcherButton.getVisibility());

                    View menuButton = toolbar.findViewById(R.id.menu_button_wrapper);
                    assertNotNull("The top toolbar menu button is missing.", menuButton);
                    assertEquals(
                            "The top toolbar menu button is shown " + when + ".",
                            View.GONE,
                            menuButton.getVisibility());
                });
    }
}
