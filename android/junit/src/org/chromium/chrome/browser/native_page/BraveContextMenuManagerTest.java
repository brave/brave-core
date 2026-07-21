/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.native_page;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.native_page.ContextMenuManager.ContextMenuItemId;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.ui.native_page.TouchEnabledDelegate;

/**
 * Unit tests for {@link BraveContextMenuManager}. Verifies the "Open in new tab in group" native
 * page context menu item is only shown when the "Enable tab groups" master switch is on. Mirrors
 * upstream {@link ContextMenuManager}'s test.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveContextMenuManagerTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private NativePageNavigationDelegate mNavigationDelegate;
    @Mock private TouchEnabledDelegate mTouchEnabledDelegate;
    @Mock private ContextMenuManager.Delegate mDelegate;

    private BraveContextMenuManager mManager;

    @Before
    public void setUp() {
        mManager =
                new BraveContextMenuManager(
                        mNavigationDelegate, mTouchEnabledDelegate, () -> {}, "");
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED);
    }

    @Test
    public void testOpenInNewTabInGroup_TabGroupsEnabled_Shown() {
        // Default state: the master switch is on, so the item is shown when upstream allows it.
        doReturn(true).when(mDelegate).isItemSupported(ContextMenuItemId.OPEN_IN_NEW_TAB_IN_GROUP);
        doReturn(true).when(mNavigationDelegate).isOpenInNewTabInGroupEnabled();

        assertTrue(mManager.shouldShowItem(ContextMenuItemId.OPEN_IN_NEW_TAB_IN_GROUP, mDelegate));
    }

    @Test
    public void testOpenInNewTabInGroup_TabGroupsDisabled_Hidden() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);
        doReturn(true).when(mDelegate).isItemSupported(ContextMenuItemId.OPEN_IN_NEW_TAB_IN_GROUP);
        doReturn(true).when(mNavigationDelegate).isOpenInNewTabInGroupEnabled();

        assertFalse(mManager.shouldShowItem(ContextMenuItemId.OPEN_IN_NEW_TAB_IN_GROUP, mDelegate));
    }

    @Test
    public void testOtherItemUnaffectedWhenTabGroupsDisabled() {
        // A non-group item stays visible regardless of the master switch.
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);
        doReturn(true).when(mDelegate).isItemSupported(ContextMenuItemId.OPEN_IN_NEW_TAB);

        assertTrue(mManager.shouldShowItem(ContextMenuItemId.OPEN_IN_NEW_TAB, mDelegate));
    }
}
