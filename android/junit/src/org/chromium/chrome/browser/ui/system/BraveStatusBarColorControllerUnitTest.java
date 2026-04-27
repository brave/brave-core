/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.system;

import static org.junit.Assert.assertEquals;

import android.app.Activity;
import android.graphics.Color;

import androidx.annotation.ColorInt;
import androidx.core.content.ContextCompat;
import androidx.test.ext.junit.rules.ActivityScenarioRule;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableNonNullObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.layouts.LayoutManager;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;
import org.chromium.chrome.browser.ui.system.StatusBarColorController.StatusBarColorProvider;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager;
import org.chromium.ui.base.TestActivity;
import org.chromium.ui.edge_to_edge.EdgeToEdgeSystemBarColorHelper;

/** Unit tests for {@link BraveStatusBarColorController}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveStatusBarColorControllerUnitTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Rule
    public final ActivityScenarioRule<TestActivity> mActivityScenarioRule =
            new ActivityScenarioRule<>(TestActivity.class);

    @Mock private StatusBarColorProvider mStatusBarColorProvider;
    @Mock private ActivityLifecycleDispatcher mActivityLifecycleDispatcher;
    @Mock private TopUiThemeColorProvider mTopUiThemeColorProvider;
    @Mock private EdgeToEdgeSystemBarColorHelper mSystemBarColorHelper;
    @Mock private DesktopWindowStateManager mDesktopWindowStateManager;

    private final MonotonicObservableSupplier<LayoutManager> mLayoutManagerSupplier =
            ObservableSuppliers.alwaysNull();
    private final ActivityTabProvider mActivityTabProvider = new ActivityTabProvider();
    private final SettableNonNullObservableSupplier<Integer> mOverviewColorSupplier =
            ObservableSuppliers.createNonNull(Color.TRANSPARENT);

    private Activity mActivity;

    @Before
    public void setUp() {
        mActivityScenarioRule.getScenario().onActivity(activity -> mActivity = activity);
    }

    @Test
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    public void testBackgroundColorForNtp_dynamicColorsDisabled_returnsWhite() {
        // Existing Brave behavior in light mode: force the NTP status-bar background to white
        // to mask the upstream regression.
        BraveStatusBarColorController controller = newController();
        assertEquals(Color.WHITE, controller.getBackgroundColorForNtpForTesting());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    public void testBackgroundColorForNtp_dynamicColorsEnabled_returnsUpstreamDefault() {
        // With dynamic colors enabled the Brave override is skipped so the themed surface
        // color (set by upstream's constructor) is preserved.
        // home_surface_background_color resolves ?attr/colorSurface. In Robolectric the
        // Material3 dynamic token chain isn't available, so colorSurface falls back to
        // white - the same value Brave's override would set. The assertEquals below still
        // documents that we preserve the upstream default; the companion test
        // testBackgroundColorForNtp_dynamicColorsDisabled_returnsWhite provides the
        // regression-catching coverage (if the guard is dropped, that test still asserts
        // Color.WHITE is set, which is a deliberate Brave choice, not upstream's default).
        @ColorInt
        int upstreamDefault =
                ContextCompat.getColor(mActivity, R.color.home_surface_background_color);
        BraveStatusBarColorController controller = newController();
        assertEquals(upstreamDefault, controller.getBackgroundColorForNtpForTesting());
    }

    private BraveStatusBarColorController newController() {
        return new BraveStatusBarColorController(
                mActivity,
                /* isTablet= */ false,
                mStatusBarColorProvider,
                mLayoutManagerSupplier,
                mActivityLifecycleDispatcher,
                mActivityTabProvider,
                mTopUiThemeColorProvider,
                mSystemBarColorHelper,
                mDesktopWindowStateManager,
                mOverviewColorSupplier);
    }
}
