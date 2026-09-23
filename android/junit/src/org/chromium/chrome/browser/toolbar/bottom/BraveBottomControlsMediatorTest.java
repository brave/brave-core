/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.browser_controls.BottomControlsStacker;
import org.chromium.chrome.browser.browser_controls.BottomControlsStacker.LayerType;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.overlay_panel.PanelState;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tab.TabObscuringHandler;
import org.chromium.ui.KeyboardVisibilityDelegate;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;

/**
 * Unit tests for {@link BraveBottomControlsMediator}. Verifies that the tab groups bar follows the
 * "Tab groups bar" and "Enable tab groups" switches as soon as they change, without waiting for the
 * tab group UI to request a new visibility (which only happens on a tab switch).
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveBottomControlsMediatorTest {
    private static final int BOTTOM_CONTROLS_HEIGHT = 60;

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private WindowAndroid mWindowAndroid;
    @Mock private KeyboardVisibilityDelegate mKeyboardVisibilityDelegate;
    @Mock private BottomControlsStacker mBottomControlsStacker;
    @Mock private BrowserControlsStateProvider mBrowserControlsStateProvider;
    @Mock private FullscreenManager mFullscreenManager;
    @Mock private OneshotSupplier<BottomControlsContentDelegate> mContentDelegateSupplier;
    @Mock private TabObscuringHandler mTabObscuringHandler;

    private PropertyModel mModel;
    private BraveBottomControlsMediator mMediator;

    @Before
    public void setUp() {
        when(mWindowAndroid.getKeyboardDelegate()).thenReturn(mKeyboardVisibilityDelegate);
        when(mBottomControlsStacker.getBrowserControls()).thenReturn(mBrowserControlsStateProvider);

        mModel = new PropertyModel.Builder(BottomControlsProperties.ALL_KEYS).build();
        mMediator =
                new BraveBottomControlsMediator(
                        mWindowAndroid,
                        mModel,
                        mBottomControlsStacker,
                        // Not a mock: NullableObservableSupplier is @DoNotMock, so supply the
                        // persistent fullscreen state instead.
                        new BrowserStateBrowserControlsVisibilityDelegate(
                                ObservableSuppliers.createNonNull(false)),
                        mFullscreenManager,
                        LayerType.BOTTOM_TOOLBAR,
                        mContentDelegateSupplier,
                        mTabObscuringHandler,
                        BOTTOM_CONTROLS_HEIGHT,
                        /* bottomControlsShadowHeight= */ 0,
                        ObservableSuppliers.createNonNull(PanelState.UNDEFINED),
                        ObservableSuppliers.alwaysNull(),
                        ObservableSuppliers.createNullable(),
                        /* readAloudRestoringSupplier= */ () -> false);
    }

    @Test
    public void testTabGroupsBarSwitchChange_appliesWithoutTabSwitch() {
        // The tab group UI asks for the bar, as it does when the current tab joins a group.
        mMediator.setBottomControlsVisible(true);
        assertBarVisible(true);

        setTabGroupsBarEnabled(false);
        mMediator.onTabGroupsSettingsChanged();
        assertBarVisible(false);

        setTabGroupsBarEnabled(true);
        mMediator.onTabGroupsSettingsChanged();
        assertBarVisible(true);
    }

    @Test
    public void testTabGroupsMasterSwitchChange_appliesWithoutTabSwitch() {
        mMediator.setBottomControlsVisible(true);
        assertBarVisible(true);

        setTabGroupsEnabled(false);
        mMediator.onTabGroupsSettingsChanged();
        assertBarVisible(false);

        setTabGroupsEnabled(true);
        mMediator.onTabGroupsSettingsChanged();
        assertBarVisible(true);
    }

    @Test
    public void testSwitchChange_ignoredWhenTabGroupUiDidNotRequestTheBar() {
        // No group is open, so the bar must stay hidden no matter what the switches say.
        mMediator.setBottomControlsVisible(false);

        setTabGroupsBarEnabled(false);
        mMediator.onTabGroupsSettingsChanged();
        assertBarVisible(false);

        setTabGroupsBarEnabled(true);
        mMediator.onTabGroupsSettingsChanged();
        assertBarVisible(false);
    }

    private static void setTabGroupsBarEnabled(boolean enabled) {
        // Mimics BraveTabUiFeatureUtilities, which the tab groups settings screen writes through.
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_BAR_ENABLED, enabled);
    }

    private static void setTabGroupsEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, enabled);
    }

    private void assertBarVisible(boolean visible) {
        // The supplier drives the Android view; the model property drives the composited layer.
        if (visible) {
            assertTrue(mMediator.getTabGroupUiVisibleSupplier().get());
            assertTrue(mModel.get(BottomControlsProperties.COMPOSITED_VIEW_VISIBLE));
        } else {
            assertFalse(mMediator.getTabGroupUiVisibleSupplier().get());
            assertFalse(mModel.get(BottomControlsProperties.COMPOSITED_VIEW_VISIBLE));
        }
    }
}
