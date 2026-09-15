/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.mock;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link BraveTabListEditorManager}. Verifies that the multi-select menu keeps its
 * tab group creation action only while the "Enable tab groups" master switch is on.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveTabListEditorManagerTest {
    private final TabListEditorAction mCloseAction = mock(TabListEditorCloseAction.class);
    private final TabListEditorAction mAddToGroupAction = mock(TabListEditorAddToGroupAction.class);
    private final TabListEditorAction mShareAction = mock(TabListEditorShareAction.class);

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED);
    }

    @Test
    public void testTabGroupsEnabled_keepsUpstreamActions() {
        BraveTabUiFeatureUtilities.setTabGroupsEnabled(true);

        assertNull(
                BraveTabListEditorManager.filterActions(
                        Arrays.asList(mCloseAction, mAddToGroupAction, mShareAction)));
    }

    @Test
    public void testTabGroupsDisabled_dropsAddToGroupAction() {
        BraveTabUiFeatureUtilities.setTabGroupsEnabled(false);

        List<TabListEditorAction> actions =
                BraveTabListEditorManager.filterActions(
                        Arrays.asList(mCloseAction, mAddToGroupAction, mShareAction));

        assertNotNull(actions);
        assertEquals(Arrays.asList(mCloseAction, mShareAction), actions);
    }

    @Test
    public void testTabGroupsDisabled_withoutAddToGroupActionKeepsUpstreamActions() {
        BraveTabUiFeatureUtilities.setTabGroupsEnabled(false);

        assertNull(
                BraveTabListEditorManager.filterActions(Arrays.asList(mCloseAction, mShareAction)));
    }
}
