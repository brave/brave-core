/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.contextmenu;

import static org.junit.Assert.assertTrue;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.tasks.tab_management.BraveTabUiFeatureUtilities;
import org.chromium.chrome.browser.tasks.tab_management.TabUiFeatureUtilities;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

@RunWith(BaseJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class BraveContextMenuPopulatorTest {
    @Test
    @SmallTest
    public void testTabGroupAutoCreationPreference() {
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            BraveTabUiFeatureUtilities.maybeOverrideEnableTabGroupAutoCreationPreference(
                    ContextUtils.getApplicationContext());
            assertTrue(TabUiFeatureUtilities.ENABLE_TAB_GROUP_AUTO_CREATION.getValue());
        });
    }
}
