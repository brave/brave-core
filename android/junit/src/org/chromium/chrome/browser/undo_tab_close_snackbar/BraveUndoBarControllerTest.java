/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.undo_tab_close_snackbar;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;

import androidx.test.core.app.ApplicationProvider;

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
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.undo.UndoClosureSnapshot;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarManageable;

import java.util.Collections;
import java.util.List;

/**
 * Unit tests for the Brave rewrite of {@link UndoBarController}: the SHOW_UNDO_WHEN_TABS_CLOSED
 * preference decides whether a closure gets an undo bar or is committed right away.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveUndoBarControllerTest {
    private static final int TAB_ID_1 = 1;
    private static final int TAB_ID_2 = 2;

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private TabModelSelector mTabModelSelector;
    @Mock private TabModel mTabModel;
    @Mock private SnackbarManageable mSnackbarManageable;
    @Mock private SnackbarManager mSnackbarManager;

    private UndoBarController mController;

    private static class TestUndoBarController extends UndoBarController {
        TestUndoBarController(
                Context context, TabModelSelector selector, SnackbarManageable snackbarManageable) {
            super(context, selector, snackbarManageable);
        }
    }

    @Before
    public void setUp() {
        mController =
                new TestUndoBarController(
                        ApplicationProvider.getApplicationContext(),
                        mTabModelSelector,
                        mSnackbarManageable);
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.SHOW_UNDO_WHEN_TABS_CLOSED);
    }

    @Test
    public void testUndoBarShownByDefault() {
        when(mSnackbarManageable.getSnackbarManager()).thenReturn(mSnackbarManager);

        mController.queueUndoBar(twoClosedTabs());

        verify(mSnackbarManager).showSnackbar(any());
        verify(mTabModelSelector, never()).getModelForTabId(anyInt());
    }

    @Test
    public void testPreferenceOffCommitsClosuresWithoutUndoBar() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.SHOW_UNDO_WHEN_TABS_CLOSED, false);
        when(mTabModelSelector.getModelForTabId(anyInt())).thenReturn(mTabModel);

        mController.queueUndoBar(twoClosedTabs());

        verify(mTabModel).commitTabClosure(TAB_ID_1);
        verify(mTabModel).commitTabClosure(TAB_ID_2);
        verify(mSnackbarManageable, never()).getSnackbarManager();
    }

    private static UndoClosureSnapshot twoClosedTabs() {
        return UndoClosureSnapshot.forTabModel(
                List.of(
                        new UndoClosureSnapshot.ClosedTab(TAB_ID_1, "Tab 1", /* groupId= */ null),
                        new UndoClosureSnapshot.ClosedTab(TAB_ID_2, "Tab 2", /* groupId= */ null)),
                Collections.emptyMap(),
                Collections.emptySet(),
                Collections.emptySet(),
                /* isTabGroupSyncEnabled= */ false,
                /* isAllTabs= */ false);
    }
}
