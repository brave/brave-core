/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.system;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;

import android.app.Activity;
import android.graphics.Color;
import android.os.Build;

import androidx.annotation.ColorInt;
import androidx.test.ext.junit.rules.ActivityScenarioRule;

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
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableNonNullObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.layouts.LayoutManager;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;
import org.chromium.chrome.browser.ui.system.StatusBarColorController.StatusBarColorProvider;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager;
import org.chromium.ui.base.TestActivity;
import org.chromium.ui.edge_to_edge.EdgeToEdgeSystemBarColorHelper;

/** Unit tests for {@link BraveStatusBarColorController}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveStatusBarColorControllerUnitTest {
    private static final @ColorInt int TEST_UPSTREAM_NTP_BACKGROUND_COLOR =
            Color.rgb(0x12, 0x34, 0x56);

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Rule
    public final ActivityScenarioRule<TestActivity> mActivityScenarioRule =
            new ActivityScenarioRule<>(TestActivity.class);

    @Mock private StatusBarColorProvider mStatusBarColorProvider;
    @Mock private ActivityLifecycleDispatcher mActivityLifecycleDispatcher;
    @Mock private TopUiThemeColorProvider mTopUiThemeColorProvider;
    @Mock private EdgeToEdgeSystemBarColorHelper mSystemBarColorHelper;
    @Mock private DesktopWindowStateManager mDesktopWindowStateManager;
    @Mock private BrowserControlsStateProvider mBrowserControlsStateProvider;

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

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED);
    }

    @Test
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    @Config(sdk = Build.VERSION_CODES.S)
    public void testBackgroundColorForNtp_dynamicColorsDisabled_returnsWhite() {
        BraveStatusBarColorController controller =
                newControllerWithUpstreamNtpBackground(TEST_UPSTREAM_NTP_BACKGROUND_COLOR);
        assertEquals(Color.WHITE, controller.getBackgroundColorForNtpForTesting());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    @Config(sdk = Build.VERSION_CODES.S)
    public void testBackgroundColorForNtp_dynamicColorsEnabled_returnsUpstreamDefault() {
        BraveStatusBarColorController controller =
                newControllerWithUpstreamNtpBackground(TEST_UPSTREAM_NTP_BACKGROUND_COLOR);
        assertEquals(
                TEST_UPSTREAM_NTP_BACKGROUND_COLOR,
                controller.getBackgroundColorForNtpForTesting());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    @Config(sdk = Build.VERSION_CODES.R)
    public void testBackgroundColorForNtp_dynamicColorsBelowAndroidS_returnsWhite() {
        BraveStatusBarColorController controller =
                newControllerWithUpstreamNtpBackground(TEST_UPSTREAM_NTP_BACKGROUND_COLOR);
        assertEquals(Color.WHITE, controller.getBackgroundColorForNtpForTesting());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    @Config(sdk = Build.VERSION_CODES.S)
    public void testBackgroundColorForNtp_dynamicColorsUserDisabled_returnsWhite() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED, false);
        BraveStatusBarColorController controller =
                newControllerWithUpstreamNtpBackground(TEST_UPSTREAM_NTP_BACKGROUND_COLOR);
        assertEquals(Color.WHITE, controller.getBackgroundColorForNtpForTesting());
    }

    private BraveStatusBarColorController newControllerWithUpstreamNtpBackground(
            @ColorInt int upstreamNtpBackground) {
        Activity activity = spy(mActivity);
        doReturn(upstreamNtpBackground)
                .when(activity)
                .getColor(R.color.home_surface_background_color);

        return new BraveStatusBarColorController(
                activity,
                /* isTablet= */ false,
                mStatusBarColorProvider,
                mLayoutManagerSupplier,
                mActivityLifecycleDispatcher,
                mActivityTabProvider,
                mTopUiThemeColorProvider,
                mSystemBarColorHelper,
                mDesktopWindowStateManager,
                mOverviewColorSupplier,
                mBrowserControlsStateProvider);
    }
}
