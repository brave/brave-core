/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.compositor.layouts.phone;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.when;

import android.graphics.Rect;
import android.view.View;
import android.view.ViewGroup;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.compositor.layouts.phone.BraveNewTabAnimationLayout.BraveTabSwitcherState;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/** Unit tests for {@link BraveNewTabAnimationLayout}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveNewTabAnimationLayoutUnitTest {
    private static final int ORIG_TOOLBAR_HEIGHT = 150;
    private static final int BOTTOM_LEFT = 700;
    private static final int BOTTOM_TOP = 1900;
    private static final int BOTTOM_RIGHT = 850;
    private static final int BOTTOM_BOTTOM = 2000;

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private ViewGroup mAnimationHostView;
    @Mock private View mRootView;
    @Mock private View mBottomToolbar;
    @Mock private View mBottomTabSwitcherButton;

    private final Rect mOrigRect = new Rect(0, 0, 0, 0);

    @Before
    public void setUp() {
        // Wire getRootView() -> mRootView -> bottom_toolbar -> bottom_tab_switcher_button.
        lenient().when(mAnimationHostView.getRootView()).thenReturn(mRootView);
        lenient().when(mRootView.findViewById(R.id.bottom_toolbar)).thenReturn(mBottomToolbar);
        lenient()
                .when(mBottomToolbar.findViewById(R.id.bottom_tab_switcher_button))
                .thenReturn(mBottomTabSwitcherButton);
        lenient()
                .when(mBottomTabSwitcherButton.getGlobalVisibleRect(any(Rect.class)))
                .thenAnswer(
                        invocation -> {
                            Rect r = invocation.getArgument(0);
                            r.set(BOTTOM_LEFT, BOTTOM_TOP, BOTTOM_RIGHT, BOTTOM_BOTTOM);
                            return true;
                        });
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY, false);
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
    @SmallTest
    public void testCompute_bottomEnabled_bottomButtonVisible_returnsBottomState() {
        enableBraveBottomControls();

        BraveTabSwitcherState s =
                BraveNewTabAnimationLayout.compute(
                        mAnimationHostView,
                        mOrigRect,
                        /* origIsVisible= */ false,
                        ORIG_TOOLBAR_HEIGHT,
                        /* origIsTopToolbar= */ true);

        assertFalse("Rect should not be empty", s.rect.isEmpty());
        assertEquals(BOTTOM_LEFT, s.rect.left);
        assertEquals(BOTTOM_TOP, s.rect.top);
        assertEquals(BOTTOM_RIGHT, s.rect.right);
        assertEquals(BOTTOM_BOTTOM, s.rect.bottom);
        assertTrue("isVisible should be true", s.isVisible);
        assertEquals("toolbarHeight should equal bottom button top", BOTTOM_TOP, s.toolbarHeight);
        assertFalse("isTopToolbar should be false", s.isTopToolbar);
    }

    @Test
    @SmallTest
    public void testCompute_bottomDisabled_returnsOriginals() {
        // Bottom address bar is enabled
        disableBraveBottomControls();

        BraveTabSwitcherState s =
                BraveNewTabAnimationLayout.compute(
                        mAnimationHostView,
                        mOrigRect,
                        /* origIsVisible= */ false,
                        ORIG_TOOLBAR_HEIGHT,
                        /* origIsTopToolbar= */ true);

        assertTrue("Rect should be the original empty rect", s.rect.isEmpty());
        assertFalse("isVisible should be the original false", s.isVisible);
        assertEquals(ORIG_TOOLBAR_HEIGHT, s.toolbarHeight);
        assertTrue("isTopToolbar should be the original true", s.isTopToolbar);
    }

    @Test
    @SmallTest
    public void testCompute_bottomEnabled_bottomButtonNotVisible_returnsOriginals() {
        // Split-screen / small-window case
        enableBraveBottomControls();
        when(mBottomTabSwitcherButton.getGlobalVisibleRect(any(Rect.class))).thenReturn(false);

        BraveTabSwitcherState s =
                BraveNewTabAnimationLayout.compute(
                        mAnimationHostView,
                        mOrigRect,
                        /* origIsVisible= */ false,
                        ORIG_TOOLBAR_HEIGHT,
                        /* origIsTopToolbar= */ true);

        assertTrue("Rect should be the original when bottom button not visible", s.rect.isEmpty());
        assertEquals(ORIG_TOOLBAR_HEIGHT, s.toolbarHeight);
        assertTrue(
                "isTopToolbar should be original when bottom button not visible", s.isTopToolbar);
    }
}
