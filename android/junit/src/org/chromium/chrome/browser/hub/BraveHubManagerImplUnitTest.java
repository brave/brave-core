/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.hub;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.android.controller.ActivityController;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.LazyOneshotSupplier;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.OneshotSupplierImpl;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.base.supplier.SettableNullableObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.ui.actions.button.DisplayButtonData;
import org.chromium.chrome.browser.ui.actions.button.FullButtonData;
import org.chromium.chrome.browser.ui.bottombar.BottomBarHostManager;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.searchactivityutils.SearchActivityClient;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.widget.MenuOrKeyboardActionController;
import org.chromium.components.feature_engagement.Tracker;
import org.chromium.ui.base.TestActivity;

/** Unit tests for {@link BraveHubManagerImpl}. */
@RunWith(BaseRobolectricTestRunner.class)
@DisableFeatures(BraveFeatureList.BRAVE_SHRED)
public class BraveHubManagerImplUnitTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BackPressManager mBackPressManager;
    @Mock private Pane mTabSwitcherPane;
    @Mock private ViewGroup mTabSwitcherPaneView;
    @Mock private HubLayoutController mHubLayoutController;
    @Mock private MenuOrKeyboardActionController mMenuOrKeyboardActionController;
    @Mock private SnackbarManager mSnackbarManager;
    @Mock private BottomSheetController mBottomSheetController;
    @Mock private BottomBarHostManager mBottomBarHostManager;
    @Mock private MenuButtonCoordinator mMenuButtonCoordinator;
    @Mock private HubShowPaneHelper mHubShowPaneHelper;
    @Mock private DisplayButtonData mReferenceButtonData;
    @Mock private ProfileProvider mProfileProvider;
    @Mock private Profile mProfile;
    @Mock private Tracker mTracker;
    @Mock private SearchActivityClient mSearchActivityClient;

    private final SettableNullableObservableSupplier<Tab> mTabSupplier =
            ObservableSuppliers.createNullable();
    private final SettableMonotonicObservableSupplier<FullButtonData> mActionButtonDataSupplier =
            ObservableSuppliers.createMonotonic();
    private final OneshotSupplierImpl<ProfileProvider> mProfileProviderSupplier =
            new OneshotSupplierImpl<>();
    private final MonotonicObservableSupplier<EdgeToEdgeController> mEdgeToEdgeSupplier =
            ObservableSuppliers.alwaysNull();

    private ActivityController<TestActivity> mActivityController;
    private Activity mActivity;

    @Before
    public void setUp() {
        TrackerFactory.setTrackerForTests(mTracker);
        mProfileProviderSupplier.set(mProfileProvider);
        when(mProfileProvider.getOriginalProfile()).thenReturn(mProfile);

        when(mTabSwitcherPane.getPaneId()).thenReturn(PaneId.TAB_SWITCHER);
        when(mTabSwitcherPane.getColorScheme()).thenReturn(HubColorScheme.DEFAULT);
        when(mTabSwitcherPane.getReferenceButtonDataSupplier())
                .thenReturn(ObservableSuppliers.createMonotonic(mReferenceButtonData));
        when(mTabSwitcherPane.getHubSearchEnabledStateSupplier())
                .thenReturn(ObservableSuppliers.alwaysTrue());
        when(mTabSwitcherPane.getHubSearchBoxVisibilitySupplier())
                .thenReturn(ObservableSuppliers.alwaysTrue());
        when(mTabSwitcherPane.getActionButtonDataSupplier()).thenReturn(mActionButtonDataSupplier);
        when(mTabSwitcherPane.getRootView()).thenReturn(mTabSwitcherPaneView);

        when(mHubLayoutController.getPreviousLayoutTypeSupplier())
                .thenReturn(ObservableSuppliers.alwaysNull());
        when(mHubLayoutController.getIsAnimatingSupplier())
                .thenReturn(ObservableSuppliers.alwaysFalse());
        when(mHubLayoutController.getIsHidingSupplier())
                .thenReturn(ObservableSuppliers.alwaysFalse());

        ChromeFeatureList.sAndroidBottomBarShowBottomBarOnGts.setForTesting(false);

        mActivityController = Robolectric.buildActivity(TestActivity.class).setup();
        mActivity = mActivityController.get();
        mActivity.setContentView(new FrameLayout(mActivity));
    }

    @After
    public void tearDown() {
        mActivityController.close();
        ChromeSharedPreferences.getInstance().removeKey(ChromePreferenceKeys.TOOLBAR_TOP_ANCHORED);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR);
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testToolbarOnTop_addressBarOnTop() {
        setAddressBarOnTop(true);
        assertNotEquals(Gravity.BOTTOM, showHubAndGetToolbarGravity());
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testToolbarOnBottom_addressBarOnBottom() {
        setAddressBarOnTop(false);
        assertEquals(Gravity.BOTTOM, showHubAndGetToolbarGravity());
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testToolbarOnBottom_bottomBarEnabled_addressBarOnTop() {
        setAddressBarOnTop(true);
        setBottomBarSettingEnabled(true);
        assertEquals(Gravity.BOTTOM, showHubAndGetToolbarGravity());
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testToolbarOnTop_bottomBarSettingDisabled_addressBarOnTop() {
        setAddressBarOnTop(true);
        setBottomBarSettingEnabled(false);
        assertNotEquals(Gravity.BOTTOM, showHubAndGetToolbarGravity());
    }

    private void setAddressBarOnTop(boolean onTop) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(ChromePreferenceKeys.TOOLBAR_TOP_ANCHORED, onTop);
    }

    private void setBottomBarSettingEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR, enabled);
    }

    private int showHubAndGetToolbarGravity() {
        PaneListBuilder builder =
                new PaneListBuilder(new DefaultPaneOrderController())
                        .registerPane(
                                PaneId.TAB_SWITCHER,
                                LazyOneshotSupplier.fromValue(mTabSwitcherPane));
        BraveHubManagerImpl hubManager =
                new BraveHubManagerImpl(
                        mActivity,
                        mProfileProviderSupplier,
                        builder,
                        mBackPressManager,
                        mMenuOrKeyboardActionController,
                        mSnackbarManager,
                        mBottomSheetController,
                        mBottomBarHostManager,
                        mTabSupplier,
                        mMenuButtonCoordinator,
                        mHubShowPaneHelper,
                        mEdgeToEdgeSupplier,
                        mSearchActivityClient,
                        /* xrSpaceModeObservableSupplier= */ null,
                        /* defaultPaneId= */ PaneId.TAB_SWITCHER);
        hubManager.getPaneManager().focusPane(PaneId.TAB_SWITCHER);
        hubManager.setHubLayoutController(mHubLayoutController);
        hubManager.onHubLayoutShow();

        View toolbarWrapper =
                (View) hubManager.getContainerView().findViewById(R.id.hub_toolbar).getParent();
        int gravity = ((FrameLayout.LayoutParams) toolbarWrapper.getLayoutParams()).gravity;
        hubManager.destroy();
        return gravity;
    }
}
