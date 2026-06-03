/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.view.ContextThemeWrapper;

import androidx.test.core.app.ApplicationProvider;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.RobolectricUtil;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.layouts.LayoutManager;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.ToolbarPositionController.ToolbarPositionAndSource;
import org.chromium.components.browser_ui.styles.SemanticColorUtils;
import org.chromium.ui.edge_to_edge.EdgeToEdgeSystemBarColorHelper;

/** Unit tests for {@link BraveTabbedNavigationBarColorControllerBase}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = 29)
@DisableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
public class BraveTabbedNavigationBarColorControllerBaseTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private TabModelSelector mTabModelSelector;
    @Mock private LayoutManager mLayoutManager;
    @Mock private FullscreenManager mFullscreenManager;
    @Mock private EdgeToEdgeSystemBarColorHelper mEdgeToEdgeSystemBarColorHelper;
    @Mock private BottomAttachedUiObserver mBottomAttachedUiObserver;

    private final SettableMonotonicObservableSupplier<TabModel> mTabModelSupplier =
            ObservableSuppliers.createMonotonic();

    private BraveTabbedNavigationBarColorControllerBase mBase;
    private Context mContext;

    @Before
    public void setUp() {
        mContext =
                new ContextThemeWrapper(
                        ApplicationProvider.getApplicationContext(),
                        R.style.Theme_BrowserUI_DayNight);

        when(mTabModelSelector.getCurrentTabModelSupplier()).thenReturn(mTabModelSupplier);

        // Pre-set the "bottom toolbar initialized" flags to bypass the isSmallScreen() call
        // inside BottomToolbarConfiguration (which requires a running Activity).
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY, true);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, true);

        SettableMonotonicObservableSupplier<LayoutManager> layoutManagerSupplier =
                ObservableSuppliers.createMonotonic();
        SettableMonotonicObservableSupplier<Integer> overviewColorSupplier =
                ObservableSuppliers.createMonotonic();

        // TabbedNavigationBarColorController extends BraveTabbedNavigationBarColorControllerBase
        // after bytecode patching, but the compiler doesn't know that — hence the double cast.
        mBase =
                (BraveTabbedNavigationBarColorControllerBase)
                        (Object)
                                new TabbedNavigationBarColorController(
                                        mContext,
                                        mTabModelSelector,
                                        layoutManagerSupplier,
                                        mFullscreenManager,
                                        ObservableSuppliers.createMonotonic(),
                                        overviewColorSupplier,
                                        mEdgeToEdgeSystemBarColorHelper,
                                        mBottomAttachedUiObserver);
        layoutManagerSupplier.set(mLayoutManager);
        RobolectricUtil.runAllBackgroundAndUi();
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        ChromeSharedPreferences.getInstance().removeKey(ChromePreferenceKeys.TOOLBAR_TOP_ANCHORED);
    }

    @Test
    public void testGetNavigationBarColor_dynamicColorsDisabled_regularTab() {
        when(mTabModelSelector.isIncognitoSelected()).thenReturn(false);
        assertEquals(
                mContext.getColor(R.color.default_bg_color_baseline),
                mBase.getNavigationBarColor(false));
    }

    @Test
    public void testGetNavigationBarColor_dynamicColorsDisabled_incognitoTab() {
        when(mTabModelSelector.isIncognitoSelected()).thenReturn(true);
        assertEquals(
                mContext.getColor(R.color.toolbar_background_primary_dark),
                mBase.getNavigationBarColor(false));
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    public void testGetNavigationBarColor_dynamicColorsEnabled_delegatesToUpstream() {
        // When dynamic colors is enabled, the Brave-specific block is skipped and the
        // upstream TabbedNavigationBarColorController logic runs, returning the semantic
        // bottom system nav color rather than Brave's hardcoded colors.
        assertEquals(
                SemanticColorUtils.getBottomSystemNavColor(mContext),
                mBase.getNavigationBarColor(false));
    }

    @Test
    public void testUseActiveTabColor_bottomAnchored_returnsFalse() {
        // When the address bar is bottom-anchored, useActiveTabColor() must short-circuit to
        // false so upstream's getNavigationBarColor falls through past the "use active tab's
        // page background" branch and reaches the themed surface branch instead. Otherwise
        // the system nav bar ends up with an arbitrary page background color rather than
        // matching the adjacent bottom toolbar.
        ChromeSharedPreferences.getInstance()
                .writeInt(
                        ChromePreferenceKeys.TOOLBAR_TOP_ANCHORED,
                        ToolbarPositionAndSource.BOTTOM_SETTINGS);
        assertFalse(mBase.useActiveTabColor());
    }
}
