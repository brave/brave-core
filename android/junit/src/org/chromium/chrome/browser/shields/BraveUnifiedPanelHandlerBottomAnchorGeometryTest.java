/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;

import android.view.Gravity;

import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider.ControlsPosition;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.shields.BraveUnifiedPanelHandler.PanelGeometry;

/** Unit tests for the Shields panel's positioning math (brave/brave-browser#59894). */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveUnifiedPanelHandlerBottomAnchorGeometryTest {
    private static final int ANCHOR_HEIGHT_PX = 100;
    private static final int WINDOW_TOP_PX = 50;
    private static final int WINDOW_BOTTOM_PX = 3000;

    // --- isToolbarAtBottom ---

    @Test
    public void testIsToolbarAtBottom_nullControls_isFalse() {
        assertFalse(BraveUnifiedPanelHandler.isToolbarAtBottom(null));
    }

    @Test
    public void testIsToolbarAtBottom_controlsPositionTop_isFalse() {
        BrowserControlsManager controls = mock(BrowserControlsManager.class);
        doReturn(ControlsPosition.TOP).when(controls).getControlsPosition();

        assertFalse(BraveUnifiedPanelHandler.isToolbarAtBottom(controls));
    }

    @Test
    public void testIsToolbarAtBottom_controlsPositionBottom_isTrue() {
        BrowserControlsManager controls = mock(BrowserControlsManager.class);
        doReturn(ControlsPosition.BOTTOM).when(controls).getControlsPosition();

        assertTrue(BraveUnifiedPanelHandler.isToolbarAtBottom(controls));
    }

    // --- computePanelGeometry: toolbar on top ---

    @Test
    public void testComputePanelGeometry_toolbarOnTop_usesGravityTopEnd() {
        PanelGeometry geometry =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        /* toolbarAtBottom= */ false,
                        /* anchorTop= */ 0,
                        ANCHOR_HEIGHT_PX,
                        WINDOW_TOP_PX,
                        WINDOW_BOTTOM_PX,
                        /* density= */ 1f);

        assertEquals(Gravity.TOP | Gravity.END, geometry.gravity);
    }

    @Test
    public void testComputePanelGeometry_toolbarOnTop_startsJustBelowToolbar() {
        int anchorTop = 0;
        PanelGeometry geometry =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        /* toolbarAtBottom= */ false,
                        anchorTop,
                        ANCHOR_HEIGHT_PX,
                        WINDOW_TOP_PX,
                        WINDOW_BOTTOM_PX,
                        /* density= */ 1f);

        int toolbarBottom = anchorTop + ANCHOR_HEIGHT_PX;
        assertTrue(geometry.y < toolbarBottom && geometry.y > anchorTop);
    }

    @Test
    public void testComputePanelGeometry_toolbarOnTop_capsAgainstWindowBottom() {
        PanelGeometry shortWindow =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        false, 0, ANCHOR_HEIGHT_PX, WINDOW_TOP_PX, 500, 1f);
        PanelGeometry tallWindow =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        false, 0, ANCHOR_HEIGHT_PX, WINDOW_TOP_PX, 3000, 1f);

        assertTrue(tallWindow.maxHeightPx > shortWindow.maxHeightPx);
    }

    // --- computePanelGeometry: toolbar at the bottom ---

    @Test
    public void testComputePanelGeometry_toolbarAtBottom_usesGravityBottomCenter() {
        PanelGeometry geometry =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        /* toolbarAtBottom= */ true,
                        /* anchorTop= */ 2200,
                        ANCHOR_HEIGHT_PX,
                        WINDOW_TOP_PX,
                        WINDOW_BOTTOM_PX,
                        /* density= */ 1f);

        assertEquals(Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL, geometry.gravity);
    }

    @Test
    public void testComputePanelGeometry_toolbarAtBottom_matchesObservedRegressionValues() {
        // Exact on-device values from the #59894 repro; 2144px of content exceeded maxHeightPx,
        // which is what caused the clamp-into-toolbar bug.
        PanelGeometry geometry =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        /* toolbarAtBottom= */ true,
                        /* anchorTop= */ 2200,
                        /* anchorHeightPx= */ 147,
                        /* windowTop= */ 172,
                        WINDOW_BOTTOM_PX,
                        /* density= */ 2.625f);

        assertEquals(178, geometry.y);
        assertEquals(1997, geometry.maxHeightPx);
        assertTrue(geometry.maxHeightPx < 2144);
    }

    @Test
    public void testComputePanelGeometry_toolbarAtBottom_yIsIndependentOfAnchorPosition() {
        PanelGeometry nearTopOfScreen =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        true, 200, ANCHOR_HEIGHT_PX, WINDOW_TOP_PX, WINDOW_BOTTOM_PX, 1f);
        PanelGeometry nearBottomOfScreen =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        true, 2000, ANCHOR_HEIGHT_PX, WINDOW_TOP_PX, WINDOW_BOTTOM_PX, 1f);

        assertEquals(nearTopOfScreen.y, nearBottomOfScreen.y);
    }

    @Test
    public void testComputePanelGeometry_toolbarAtBottom_grantsMoreRoomWhenFartherFromWindowTop() {
        PanelGeometry toolbarCloseToWindowTop =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        true, 200, ANCHOR_HEIGHT_PX, WINDOW_TOP_PX, WINDOW_BOTTOM_PX, 1f);
        PanelGeometry toolbarFarFromWindowTop =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        true, 2000, ANCHOR_HEIGHT_PX, WINDOW_TOP_PX, WINDOW_BOTTOM_PX, 1f);

        assertTrue(toolbarFarFromWindowTop.maxHeightPx > toolbarCloseToWindowTop.maxHeightPx);
    }

    @Test
    public void testComputePanelGeometry_toolbarAtBottom_neverNeedsTopAboveWindowTop() {
        // The actual fix: the panel's worst-case required top must never land above windowTop,
        // across positions and densities, or Android's clamp pushes it into the toolbar.
        for (int anchorTop : new int[] {100, 500, 2200, 5000}) {
            for (float density : new float[] {1f, 1.5f, 2.625f, 3f, 4f}) {
                PanelGeometry geometry =
                        BraveUnifiedPanelHandler.computePanelGeometry(
                                true,
                                anchorTop,
                                ANCHOR_HEIGHT_PX,
                                WINDOW_TOP_PX,
                                WINDOW_BOTTOM_PX,
                                density);

                // Re-derived from the method's own outputs rather than duplicating
                // PANEL_SHADOW_SAFETY_GAP_DP here.
                int toolbarBottom = anchorTop + ANCHOR_HEIGHT_PX;
                int panelBottom = toolbarBottom - geometry.y;
                int worstCaseTop = panelBottom - geometry.maxHeightPx;

                assertTrue(
                        "worstCaseTop=" + worstCaseTop + " < windowTop for anchorTop=" + anchorTop,
                        worstCaseTop >= WINDOW_TOP_PX);
            }
        }
    }

    // --- computePanelGeometry: window not starting at the top of the screen (split-screen) ---

    @Test
    public void testComputePanelGeometry_toolbarAtBottom_windowNotAtScreenTop_stillCapsCorrectly() {
        // Shifting anchorTop and windowTop by the same amount simulates the window sitting lower
        // on the physical screen; only their difference should matter.
        int windowShift = 5000;

        PanelGeometry windowAtScreenTop =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        /* toolbarAtBottom= */ true,
                        /* anchorTop= */ 2200,
                        ANCHOR_HEIGHT_PX,
                        /* windowTop= */ 172,
                        WINDOW_BOTTOM_PX,
                        /* density= */ 2.625f);
        PanelGeometry windowShiftedDownScreen =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        /* toolbarAtBottom= */ true,
                        /* anchorTop= */ 2200 + windowShift,
                        ANCHOR_HEIGHT_PX,
                        /* windowTop= */ 172 + windowShift,
                        WINDOW_BOTTOM_PX,
                        /* density= */ 2.625f);

        assertEquals(windowAtScreenTop.y, windowShiftedDownScreen.y);
        assertEquals(windowAtScreenTop.maxHeightPx, windowShiftedDownScreen.maxHeightPx);
    }

    @Test
    public void testComputePanelGeometry_splitScreenWindow_capsToActualAvailableSpace() {
        // A constrained split-screen window: only 400px between windowTop and the toolbar.
        PanelGeometry geometry =
                BraveUnifiedPanelHandler.computePanelGeometry(
                        /* toolbarAtBottom= */ true,
                        /* anchorTop= */ 2200,
                        ANCHOR_HEIGHT_PX,
                        /* windowTop= */ 1800,
                        WINDOW_BOTTOM_PX,
                        /* density= */ 1f);

        assertTrue(geometry.maxHeightPx < 500);
    }
}
