/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.compositor.layouts.phone;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Rect;
import android.view.View;

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

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.toolbar.ToolbarPositionController.ToolbarPositionAndSource;
import org.chromium.chrome.browser.toolbar.settings.AddressBarPreference;
import org.chromium.components.browser_ui.styles.ChromeColors;
import org.chromium.ui.base.TestActivity;

/**
 * Unit tests for {@link BraveNewBackgroundTabAnimationData}.
 *
 * <p>Verifies that getTabSwitcherButtonRect() falls back to Brave's bottom toolbar tab switcher
 * button when the top toolbar button is GONE (isBraveBottomControlsEnabled() is true).
 */
@RunWith(BaseRobolectricTestRunner.class)
@DisableFeatures({
    ChromeFeatureList.ANDROID_BOTTOM_BAR,
    ChromeFeatureList.CROSS_DEVICE_PREF_TRACKER_EXTRA_LOGS
})
public class BraveNewBackgroundTabAnimationDataUnitTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Rule
    public ActivityScenarioRule<TestActivity> mActivityScenarioRule =
            new ActivityScenarioRule<>(TestActivity.class);

    @Mock private ToolbarManager mToolbarManager;
    @Mock private Tab mTab;
    @Mock private View mAnimationHostView;
    @Mock private View mToolbarContainer;
    @Mock private View mToolbarTabSwitcherButton;
    @Mock private View mBottomToolbar;
    @Mock private View mBottomTabSwitcherButton;

    private BraveNewBackgroundTabAnimationData mData;
    private Activity mActivity;

    private static final int BOTTOM_BUTTON_LEFT = 0;
    private static final int BOTTOM_BUTTON_TOP = 1800;
    private static final int BOTTOM_BUTTON_RIGHT = 100;
    private static final int BOTTOM_BUTTON_BOTTOM = 1900;

    @Before
    public void setUp() {
        when(mToolbarManager.getNtpSearchBoxTransitionPercentageSupplier())
                .thenReturn(ObservableSuppliers.createNonNull(0f));
        mActivityScenarioRule.getScenario().onActivity(this::onActivity);
    }

    @After
    public void tearDown() {
        // Reset to "not yet configured" state so tests don't affect each other.
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY, false);
    }

    private void onActivity(Activity activity) {
        when(mAnimationHostView.getContext()).thenReturn(activity);
        when(mAnimationHostView.findViewById(R.id.toolbar)).thenReturn(mToolbarContainer);
        when(mToolbarContainer.findViewById(R.id.tab_switcher_button))
                .thenReturn(mToolbarTabSwitcherButton);

        // Top toolbar button is GONE: getGlobalVisibleRect returns false, rect stays empty.
        when(mToolbarTabSwitcherButton.getGlobalVisibleRect(any(Rect.class))).thenReturn(false);

        // The following stubs are only exercised when Brave bottom controls are enabled;
        // mark lenient to suppress unused-stub hints in the disabled-controls test.
        // getRootView() returns itself so our code can call
        // mBraveAnimationHostView.getRootView().findViewById(R.id.bottom_toolbar).
        lenient().when(mAnimationHostView.getRootView()).thenReturn(mAnimationHostView);
        lenient()
                .when(mAnimationHostView.findViewById(R.id.bottom_toolbar))
                .thenReturn(mBottomToolbar);
        lenient()
                .when(mBottomToolbar.findViewById(R.id.bottom_tab_switcher_button))
                .thenReturn(mBottomTabSwitcherButton);

        // Bottom toolbar button is visible with known bounds.
        lenient()
                .when(mBottomTabSwitcherButton.getGlobalVisibleRect(any(Rect.class)))
                .thenAnswer(
                        invocation -> {
                            Rect r = invocation.getArgument(0);
                            r.set(
                                    BOTTOM_BUTTON_LEFT,
                                    BOTTOM_BUTTON_TOP,
                                    BOTTOM_BUTTON_RIGHT,
                                    BOTTOM_BUTTON_BOTTOM);
                            return true;
                        });

        mData = new BraveNewBackgroundTabAnimationData(mAnimationHostView, mToolbarManager);
        mActivity = activity;
    }

    private void enableBraveBottomControls() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY, true);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, true);
    }

    private void disableBraveBottomControls() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY, true);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, false);
    }

    @Test
    public void testGetTabSwitcherButtonRect_braveBottomEnabled_topGone_returnsBottomButtonRect() {
        enableBraveBottomControls();
        mData.captureState(mTab, /* isRegularNtp= */ false, /* expectedToolbarTop= */ 0);

        Rect rect = mData.getTabSwitcherButtonRect();

        assertFalse("Rect should not be empty when bottom toolbar button is used", rect.isEmpty());
        assertEquals(BOTTOM_BUTTON_LEFT, rect.left);
        assertEquals(BOTTOM_BUTTON_TOP, rect.top);
        assertEquals(BOTTOM_BUTTON_RIGHT, rect.right);
        assertEquals(BOTTOM_BUTTON_BOTTOM, rect.bottom);
    }

    @Test
    public void testGetTabSwitcherButtonRect_braveBottomDisabled_returnsEmptyRect() {
        disableBraveBottomControls();
        mData.captureState(mTab, /* isRegularNtp= */ false, /* expectedToolbarTop= */ 0);

        Rect rect = mData.getTabSwitcherButtonRect();

        // Top button returns empty rect and Brave bottom controls are off — no fallback.
        assertTrue("Rect should be empty when Brave bottom toolbar is disabled", rect.isEmpty());
    }

    @Test
    public void testGetPrimaryColor_braveBottomEnabled_addressbarTop_usesBottomToolbarColor() {
        // The website / top-toolbar color that must NOT leak into the animation here.
        when(mToolbarManager.getPrimaryColor()).thenReturn(Color.GREEN);
        enableBraveBottomControls();
        AddressBarPreference.setToolbarPositionAndSource(ToolbarPositionAndSource.TOP_SETTINGS);

        mData.captureState(mTab, /* isRegularNtp= */ false, /* expectedToolbarTop= */ 0);

        @ColorInt int color = mData.getPrimaryColor();

        assertEquals(
                "Should fill with the bottom toolbar's default theme color",
                ChromeColors.getDefaultThemeColor(mActivity, /* isIncognito= */ false),
                color);
        assertEquals("Fill must be opaque to cover the real button", 0xFF, Color.alpha(color));
        assertNotEquals("Fill must not use the website toolbar color", Color.GREEN, color);
    }

    @Test
    public void testGetPrimaryColor_braveBottomDisabled_usesUpstreamToolbarColor() {
        when(mToolbarManager.getPrimaryColor()).thenReturn(Color.GREEN);
        disableBraveBottomControls();

        mData.captureState(mTab, /* isRegularNtp= */ false, /* expectedToolbarTop= */ 0);

        // Not the special config - falls back to the upstream (website/toolbar) primary color.
        assertEquals(Color.GREEN, mData.getPrimaryColor());
    }

    @Test
    public void testGetPrimaryColor_braveBottomEnabled_addressbarTop_incognito_usesUpstreamColor() {
        when(mToolbarManager.getPrimaryColor()).thenReturn(Color.GREEN);
        when(mTab.isIncognitoBranded()).thenReturn(true);
        enableBraveBottomControls();
        AddressBarPreference.setToolbarPositionAndSource(ToolbarPositionAndSource.TOP_SETTINGS);

        mData.captureState(mTab, /* isRegularNtp= */ false, /* expectedToolbarTop= */ 0);

        // In incognito the toolbar is not website-colored, so the override does not apply and the
        // upstream (toolbar) primary color is used.
        assertEquals(Color.GREEN, mData.getPrimaryColor());
    }
}
