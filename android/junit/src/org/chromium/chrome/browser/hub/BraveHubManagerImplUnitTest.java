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
    private BraveHubManagerImpl mHubManager;

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
        if (mHubManager != null) mHubManager.destroy();
        mActivityController.close();
        ChromeSharedPreferences.getInstance().removeKey(ChromePreferenceKeys.TOOLBAR_TOP_ANCHORED);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR);
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testToolbarOnTop_addressBarOnTop() {
        setAddressBarOnTop(true);
        assertToolbarOnTop(showHub());
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testToolbarOnBottom_addressBarOnBottom() {
        setAddressBarOnTop(false);
        assertToolbarOnBottom(showHub());
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testToolbarOnBottom_bottomBarEnabled_addressBarOnTop() {
        setAddressBarOnTop(true);
        setBottomBarSettingEnabled(true);
        assertToolbarOnBottom(showHub());
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testToolbarOnTop_bottomBarSettingDisabled_addressBarOnTop() {
        setAddressBarOnTop(true);
        setBottomBarSettingEnabled(false);
        assertToolbarOnTop(showHub());
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testActionButtonCentered_paneSwitcherFits() {
        setAddressBarOnTop(false);
        View hubToolbar = showHub().findViewById(R.id.hub_toolbar);
        layoutActionContainer(hubToolbar, /* width= */ 360);
        assertEquals(Gravity.CENTER, getGravity(getActionButtonGroup(hubToolbar)));
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testActionButtonAtEnd_paneSwitcherOverlapsCenter() {
        setAddressBarOnTop(false);
        View hubToolbar = showHub().findViewById(R.id.hub_toolbar);
        View paneSwitcherCard = hubToolbar.findViewById(R.id.pane_switcher_card);
        // The card is hidden with a single pane; show it as if multiple panes were registered.
        paneSwitcherCard.setVisibility(View.VISIBLE);
        paneSwitcherCard.setMinimumWidth(300);
        layoutActionContainer(hubToolbar, /* width= */ 360);
        assertEquals(
                Gravity.END | Gravity.CENTER_VERTICAL,
                getGravity(getActionButtonGroup(hubToolbar)));
    }

    private static void layoutActionContainer(View hubToolbar, int width) {
        View actionContainer = hubToolbar.findViewById(R.id.toolbar_action_container);
        int height = 56;
        actionContainer.measure(
                View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
        actionContainer.layout(0, 0, width, height);
    }

    private void assertToolbarOnTop(View containerView) {
        View hubToolbar = containerView.findViewById(R.id.hub_toolbar);
        assertNotEquals(Gravity.BOTTOM, getGravity((View) hubToolbar.getParent()));
        assertEquals(
                Gravity.START | Gravity.CENTER_VERTICAL,
                getGravity(getActionButtonGroup(hubToolbar)));
        assertNotEquals(
                Gravity.START | Gravity.CENTER_VERTICAL,
                getGravity(hubToolbar.findViewById(R.id.pane_switcher_card)));
        // Space makes itself INVISIBLE rather than VISIBLE, so only check it isn't GONE.
        assertNotEquals(View.GONE, hubToolbar.findViewById(R.id.margin_spacer).getVisibility());
    }

    private void assertToolbarOnBottom(View containerView) {
        View hubToolbar = containerView.findViewById(R.id.hub_toolbar);
        assertEquals(Gravity.BOTTOM, getGravity((View) hubToolbar.getParent()));
        assertEquals(Gravity.CENTER, getGravity(getActionButtonGroup(hubToolbar)));
        assertEquals(
                Gravity.START | Gravity.CENTER_VERTICAL,
                getGravity(hubToolbar.findViewById(R.id.pane_switcher_card)));
        assertEquals(View.GONE, hubToolbar.findViewById(R.id.margin_spacer).getVisibility());
    }

    private static View getActionButtonGroup(View hubToolbar) {
        return (View) hubToolbar.findViewById(R.id.toolbar_action_button).getParent();
    }

    private static int getGravity(View view) {
        return ((FrameLayout.LayoutParams) view.getLayoutParams()).gravity;
    }

    private void setAddressBarOnTop(boolean onTop) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(ChromePreferenceKeys.TOOLBAR_TOP_ANCHORED, onTop);
    }

    private void setBottomBarSettingEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR, enabled);
    }

    private View showHub() {
        PaneListBuilder builder =
                new PaneListBuilder(new DefaultPaneOrderController())
                        .registerPane(
                                PaneId.TAB_SWITCHER,
                                LazyOneshotSupplier.fromValue(mTabSwitcherPane));
        mHubManager =
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
        mHubManager.getPaneManager().focusPane(PaneId.TAB_SWITCHER);
        mHubManager.setHubLayoutController(mHubLayoutController);
        mHubManager.onHubLayoutShow();
        return mHubManager.getContainerView();
    }
}
