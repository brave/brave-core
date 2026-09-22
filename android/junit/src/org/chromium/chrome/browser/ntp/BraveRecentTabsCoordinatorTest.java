/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.app.Activity;
import android.widget.ExpandableListView;

import androidx.test.ext.junit.rules.ActivityScenarioRule;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.base.supplier.SettableNonNullObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.native_page.NativePageNavigationDelegate;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.ui.base.TestActivity;

/**
 * Tests Brave's refresh of {@link RecentTabsManager} from {@link RecentTabsCoordinator}.
 *
 * <p>The coordinator registers its updated-callback partway through its constructor, so an update
 * that completes before that point is dropped and the screen stays empty - which is what happens on
 * the first launch after an install.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveRecentTabsCoordinatorTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Rule
    public final ActivityScenarioRule<TestActivity> mActivityScenarios =
            new ActivityScenarioRule<>(TestActivity.class);

    @Mock private RecentTabsManager mRecentTabsManager;
    @Mock private NativePageNavigationDelegate mNavigationDelegate;

    @Captor private ArgumentCaptor<RecentTabsManager.UpdatedCallback> mUpdatedCallbackCaptor;

    private final SettableMonotonicObservableSupplier<EdgeToEdgeController> mEdgeToEdgeSupplier =
            ObservableSuppliers.createMonotonic();
    private final SettableNonNullObservableSupplier<Integer> mTabStripHeightSupplier =
            ObservableSuppliers.createNonNull(0);

    private Activity mActivity;

    @Before
    public void setUp() {
        mActivityScenarios.getScenario().onActivity(activity -> mActivity = activity);
    }

    private RecentTabsCoordinator createCoordinator() {
        return new RecentTabsCoordinator(
                mActivity,
                mRecentTabsManager,
                mNavigationDelegate,
                mTabStripHeightSupplier,
                mEdgeToEdgeSupplier,
                /* parent= */ null);
    }

    @Test
    public void testManagerIsRefreshedAfterTheConstructorCompletes() {
        createCoordinator();

        verify(mRecentTabsManager, never()).syncStateChanged();

        ShadowLooper.idleMainLooper();

        verify(mRecentTabsManager).syncStateChanged();
    }

    @Test
    public void testListIsPopulatedWhenTheUpdateCompletedTooEarly() {
        // Stand in for RecentTabsManager.update(), which is what the refresh runs and which ends by
        // handing the fetched data to the registered callback.
        doAnswer(
                        invocation -> {
                            mUpdatedCallbackCaptor.getValue().onUpdated();
                            return null;
                        })
                .when(mRecentTabsManager)
                .syncStateChanged();

        RecentTabsCoordinator coordinator = createCoordinator();
        verify(mRecentTabsManager).setUpdatedCallback(mUpdatedCallbackCaptor.capture());

        ExpandableListView listView = coordinator.getListViewForTesting();
        assertEquals(
                "The adapter has no groups until the callback fires.",
                0,
                listView.getExpandableListAdapter().getGroupCount());

        ShadowLooper.idleMainLooper();

        // Group count is > 0 because RecentTabsRowAdapter.notifyDataSetChanged()
        // unconditionally adds the recently-closed group first.
        assertTrue(
                "Recent tabs must not stay empty.",
                listView.getExpandableListAdapter().getGroupCount() > 0);
    }
}
