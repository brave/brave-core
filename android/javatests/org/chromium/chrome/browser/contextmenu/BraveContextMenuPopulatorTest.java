/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.contextmenu;

import static org.junit.Assert.assertFalse;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.chrome.browser.tasks.tab_management.TabUiFeatureUtilities;

@RunWith(BaseJUnit4ClassRunner.class)
public class BraveContextMenuPopulatorTest {
    @Test
    @SmallTest
    public void testTabGroupAutoCreationPreference() {
        assertFalse(TabUiFeatureUtilities.ENABLE_TAB_GROUP_AUTO_CREATION.getValue());
    }
}
