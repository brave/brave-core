/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import static org.chromium.chrome.browser.tasks.tab_management.TabListModel.CardProperties.CARD_TYPE;
import static org.chromium.chrome.browser.tasks.tab_management.TabListModel.CardProperties.ModelType.TAB;

import androidx.recyclerview.widget.RecyclerView;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.CallbackUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tasks.tab_management.TabListMediator.TabListLayoutType;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.SimpleRecyclerViewAdapter;

import java.util.function.Supplier;

/**
 * Unit tests for {@link BraveTabGridItemTouchHelperCallback}. Verifies that when the "Enable tab
 * groups" master switch is off the callback uses the FLAT drag layout, so dragging still reorders
 * tabs but never performs the group-aware move that grouping mode would.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveTabGridItemTouchHelperCallbackTest {
    private static final int TAB1_ID = 1;
    private static final int TAB2_ID = 2;

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private TabGroupCreationDialogManager mTabGroupCreationDialogManager;
    @Mock private Supplier<TabModel> mTabModelSupplier;
    @Mock private TabModel mTabModel;
    @Mock private TabActionListener mTabClosedListener;
    @Mock private RecyclerView mRecyclerView;
    @Mock private SimpleRecyclerViewAdapter.ViewHolder mFromViewHolder;
    @Mock private SimpleRecyclerViewAdapter.ViewHolder mToViewHolder;
    @Mock private Tab mTab2;

    @Before
    public void setUp() {
        when(mTabModelSupplier.get()).thenReturn(mTabModel);
        mFromViewHolder.model = tabCardModel(TAB1_ID);
        mToViewHolder.model = tabCardModel(TAB2_ID);
        when(mTabModel.getTabById(TAB2_ID)).thenReturn(mTab2);
        when(mTabModel.indexOf(mTab2)).thenReturn(1);
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED);
    }

    @Test
    public void testOnMove_tabGroupsDisabled_reordersWithoutGrouping() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);

        createCallback().onMove(mRecyclerView, mFromViewHolder, mToViewHolder);

        // FLAT layout moves a single tab; grouping mode would call moveRelatedTabs.
        verify(mTabModel).moveTab(TAB1_ID, 1);
        verify(mTabModel, never()).moveRelatedTabs(anyInt(), anyInt());
    }

    private BraveTabGridItemTouchHelperCallback createCallback() {
        // The caller passes GROUPED for the grid; Brave downgrades it to FLAT when the switch is
        // off (evaluated at construction).
        return new BraveTabGridItemTouchHelperCallback(
                ContextUtils.getApplicationContext(),
                mTabGroupCreationDialogManager,
                new TabListModel(),
                mTabModelSupplier,
                mTabClosedListener,
                /* tabGridDialogHandler= */ null,
                /* componentName= */ "",
                TabListLayoutType.GROUPED,
                CallbackUtils.emptyRunnable());
    }

    private static PropertyModel tabCardModel(int tabId) {
        return new PropertyModel.Builder(TabProperties.ALL_KEYS_TAB_GRID)
                .with(TabProperties.TAB_ID, tabId)
                .with(CARD_TYPE, TAB)
                .build();
    }
}
