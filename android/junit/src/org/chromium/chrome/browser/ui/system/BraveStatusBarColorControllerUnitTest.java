/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.system;

import static org.junit.Assert.assertEquals;

import android.content.res.Resources;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.ColorInt;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableNonNullObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.layouts.LayoutManager;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;
import org.chromium.chrome.browser.ui.bottombar.BottomBarUtils;
import org.chromium.chrome.browser.ui.system.StatusBarColorController.StatusBarColorProvider;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager;
import org.chromium.ui.base.TestActivity;
import org.chromium.ui.edge_to_edge.EdgeToEdgeSystemBarColorHelper;

/** Unit tests for {@link BraveStatusBarColorController}. */
@RunWith(BaseRobolectricTestRunner.class)
@DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
public class BraveStatusBarColorControllerUnitTest {
    private static final @ColorInt int TEST_UPSTREAM_NTP_BACKGROUND_COLOR =
            Color.rgb(0x12, 0x34, 0x56);

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

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

    private UpstreamNtpColorActivity mActivity;

    // The real upstream NTP color may be white, which couldn't be told apart from Brave's.
    public static class UpstreamNtpColorActivity extends TestActivity {
        private @Nullable Resources mResources;

        @Override
        protected void onCreate(@Nullable Bundle savedInstanceState) {
            setTheme(R.style.Theme_Chromium_Activity);
            super.onCreate(savedInstanceState);
        }

        @Override
        public Resources getResources() {
            if (mResources == null) {
                Resources base = super.getResources();
                mResources =
                        new Resources(
                                base.getAssets(),
                                base.getDisplayMetrics(),
                                base.getConfiguration()) {
                            @Override
                            public int getColor(int id, @Nullable Theme theme) {
                                return id == R.color.home_surface_background_color
                                        ? TEST_UPSTREAM_NTP_BACKGROUND_COLOR
                                        : super.getColor(id, theme);
                            }
                        };
            }
            return mResources;
        }
    }

    @Before
    public void setUp() {
        mActivity = Robolectric.buildActivity(UpstreamNtpColorActivity.class).setup().get();
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED);
    }

    @Test
    @Config(sdk = Build.VERSION_CODES.S)
    public void testBackgroundColorForNtp_dynamicColorsUserDisabled_returnsWhite() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED, false);
        BraveStatusBarColorController controller = newController();
        assertEquals(Color.WHITE, controller.getBackgroundColorForNtpForTesting());
    }

    @Test
    @Config(sdk = Build.VERSION_CODES.S)
    public void testBackgroundColorForNtp_dynamicColorsUserEnabled_returnsUpstreamDefault() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED, true);
        BraveStatusBarColorController controller = newController();
        assertEquals(
                TEST_UPSTREAM_NTP_BACKGROUND_COLOR,
                controller.getBackgroundColorForNtpForTesting());
    }

    @Test
    @Config(sdk = Build.VERSION_CODES.R)
    public void testBackgroundColorForNtp_dynamicColorsUserEnabledBelowAndroidS_returnsWhite() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED, true);
        BraveStatusBarColorController controller = newController();
        assertEquals(Color.WHITE, controller.getBackgroundColorForNtpForTesting());
    }

    // The NTP toolbar takes the bottom bar's color, and the status bar must match it.
    @Test
    @Config(sdk = Build.VERSION_CODES.S)
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testBackgroundColorForNtp_bottomBarEnabled_returnsBottomBarColor() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED, false);
        BraveStatusBarColorController controller = newController();
        assertEquals(
                BottomBarUtils.getBottomBarBackgroundColor(mActivity, /* isIncognito= */ false),
                controller.getBackgroundColorForNtpForTesting());
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
                mOverviewColorSupplier,
                mBrowserControlsStateProvider);
    }
}
