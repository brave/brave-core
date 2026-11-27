/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.search;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import android.view.View;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.TextView;

import androidx.appcompat.widget.ActionMenuView;
import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import androidx.slidingpanelayout.widget.SlidingPaneLayout;

import com.google.android.material.appbar.AppBarLayout;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.MultiColumnSettings;
import org.chromium.chrome.browser.settings.SettingsContainmentHelper;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;
import org.chromium.ui.modaldialog.ModalDialogManager;

/** Tests for {@link BraveSettingsSearchCoordinator}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveSettingsSearchCoordinatorTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private MultiColumnSettings mMultiColumnSettings;
    @Mock private SettingsContainmentHelper mContainmentHelper;
    @Mock private Profile mProfile;
    @Mock private ModalDialogManager mModalDialogManager;

    private FragmentActivity mActivity;
    private FragmentManager mFragmentManager;
    private SettingsSearchCoordinator mCoordinator;
    private EditText mQueryEdit;
    private EditText mLocalSearch;
    // Entering search pushes its own entries before any result is opened.
    private int mSearchBackStackCount;

    @Before
    public void setUp() {
        mActivity = Robolectric.buildActivity(FragmentActivity.class).create().get();
        mActivity.setTheme(R.style.Theme_Chromium_Settings);

        FrameLayout rootView = new FrameLayout(mActivity);
        rootView.setId(R.id.settings_activity);
        AppBarLayout appBarLayout = new AppBarLayout(mActivity);
        appBarLayout.setId(R.id.app_bar_layout);
        Toolbar toolbar = new Toolbar(mActivity);
        toolbar.setId(R.id.action_bar);
        toolbar.setTitle("Settings");
        TextView titleView = new TextView(mActivity);
        titleView.setText("Settings");
        toolbar.addView(titleView);
        toolbar.addView(new ActionMenuView(mActivity));
        appBarLayout.addView(toolbar);
        rootView.addView(appBarLayout);
        FrameLayout headerPane = new FrameLayout(mActivity);
        headerPane.setId(R.id.preferences_header);
        rootView.addView(headerPane);
        FrameLayout detailPane = new FrameLayout(mActivity);
        detailPane.setId(R.id.preferences_detail);
        rootView.addView(detailPane);
        mActivity.setContentView(rootView);

        mFragmentManager = mActivity.getSupportFragmentManager();
        SlidingPaneLayout slidingPaneLayout = new SlidingPaneLayout(mActivity);
        when(mMultiColumnSettings.getChildFragmentManagerOrNull()).thenReturn(mFragmentManager);
        when(mMultiColumnSettings.getView()).thenReturn(slidingPaneLayout);
        when(mMultiColumnSettings.requireView()).thenReturn(slidingPaneLayout);
        when(mMultiColumnSettings.getSlidingPaneLayout()).thenReturn(slidingPaneLayout);
        when(mMultiColumnSettings.getSlidingPaneLayoutOrNull()).thenReturn(slidingPaneLayout);
        int widthSpec = View.MeasureSpec.makeMeasureSpec(1000, View.MeasureSpec.EXACTLY);
        int heightSpec = View.MeasureSpec.makeMeasureSpec(600, View.MeasureSpec.EXACTLY);
        rootView.measure(widthSpec, heightSpec);
        rootView.layout(0, 0, 1000, 600);

        SettableMonotonicObservableSupplier<ModalDialogManager> modalDialogSupplier =
                ObservableSuppliers.createMonotonic();
        modalDialogSupplier.set(mModalDialogManager);
        SettingsIndexData.createInstance().resetNeedsIndexing();
        mCoordinator =
                new BraveSettingsSearchCoordinator(
                        mActivity,
                        toolbar,
                        /* useMultiColumnSupplier= */ () -> true,
                        mMultiColumnSettings,
                        mContainmentHelper,
                        mProfile,
                        (index) -> {},
                        modalDialogSupplier,
                        /* shownInTab= */ false);
        mCoordinator.initializeSearchUi(null);
        ShadowLooper.idleMainLooper();

        mQueryEdit = mActivity.findViewById(R.id.search_query);
        mLocalSearch = new EditText(mActivity);
        detailPane.addView(mLocalSearch);

        enterResultStateWithSubpage();
    }

    @After
    public void tearDown() {
        SettingsIndexData.reset();
        RecentSearchQueue.getInstance().clear();
        ShadowLooper.idleMainLooper();
    }

    // Mirrors opening a search result (e.g. Password Manager) and then a page from it.
    private void enterResultStateWithSubpage() {
        mActivity.findViewById(R.id.search_box).performClick();
        mFragmentManager.executePendingTransactions();
        ShadowLooper.idleMainLooper();
        mSearchBackStackCount = mFragmentManager.getBackStackEntryCount();

        mFragmentManager
                .beginTransaction()
                .replace(R.id.preferences_detail, new Fragment())
                .addToBackStack(SettingsSearchCoordinator.RESULT_BACKSTACK)
                .setReorderingAllowed(true)
                .commit();
        mFragmentManager
                .beginTransaction()
                .replace(R.id.preferences_detail, new Fragment())
                .addToBackStack(null)
                .setReorderingAllowed(true)
                .commit();
        mFragmentManager.executePendingTransactions();
        mCoordinator.setFragmentState(SettingsSearchCoordinator.FS_RESULTS);
    }

    private void openLocalSearch() {
        assertTrue(mLocalSearch.requestFocus());
        mCoordinator.showSearchBar(false);
    }

    @Test
    public void testLocalSearchOpen_queryIsNotFocusable() {
        openLocalSearch();

        assertFalse(mQueryEdit.isFocusable());
        assertFalse(mQueryEdit.isFocusableInTouchMode());
    }

    @Test
    public void testLocalSearchLosesFocus_staysOnResultSubpage() {
        openLocalSearch();

        mLocalSearch.clearFocus();
        mFragmentManager.executePendingTransactions();
        ShadowLooper.idleMainLooper();

        assertFalse(mQueryEdit.isFocused());
        assertEquals(mSearchBackStackCount + 2, mFragmentManager.getBackStackEntryCount());
        assertTrue(mCoordinator.isSearchOpen());
    }

    @Test
    public void testLocalSearchClosed_queryFocusStillReturnsToResults() {
        openLocalSearch();
        mLocalSearch.clearFocus();
        mCoordinator.showSearchBar(true);

        assertTrue(mQueryEdit.isFocusable());
        assertTrue(mQueryEdit.isFocusableInTouchMode());

        assertTrue(mQueryEdit.requestFocus());
        mFragmentManager.executePendingTransactions();
        ShadowLooper.idleMainLooper();

        assertEquals(mSearchBackStackCount, mFragmentManager.getBackStackEntryCount());
    }

    @Test
    public void testLocalSearchOpen_tapOnQueryReturnsToResults() {
        openLocalSearch();

        assertTrue(mQueryEdit.performClick());
        mFragmentManager.executePendingTransactions();
        ShadowLooper.idleMainLooper();

        assertTrue(mQueryEdit.isFocused());
        assertEquals(mSearchBackStackCount, mFragmentManager.getBackStackEntryCount());
    }
}
