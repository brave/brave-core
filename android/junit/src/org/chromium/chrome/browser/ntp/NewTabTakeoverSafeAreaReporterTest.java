/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertEquals;

import android.graphics.RectF;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class NewTabTakeoverSafeAreaReporterTest {
    @Test
    public void spansTheGapBetweenTheTopBorderAndTheBottomBorder() {
        assertEquals(
                new RectF(16, 616, 1064, 1484),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 600,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f));
    }

    @Test
    public void isEmptyWhenTheTopBorderAndBottomBorderAreEqual() {
        RectF safeArea =
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 800,
                        /* safeAreaBottom= */ 800,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f);

        assertEquals(0f, safeArea.height(), 0f);
    }

    @Test
    public void clampsTheTopBorderToTheBottomBorderWhenTopExceedsBottom() {
        RectF safeArea =
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 1600,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f);

        assertEquals(new RectF(16, 1484, 1064, 1484), safeArea);
    }

    @Test
    public void clampsTheTopBorderToTheViewportBottom() {
        assertEquals(
                new RectF(16, 1920, 1064, 1920),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 2400,
                        /* safeAreaBottom= */ 2400,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f));
    }

    @Test
    public void clampsTheBottomBorderToTheViewportBottom() {
        assertEquals(
                new RectF(16, 616, 1064, 1920),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 600,
                        /* safeAreaBottom= */ 2400,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f));
    }

    @Test
    public void clampsTheTopBorderToTheViewportTop() {
        assertEquals(
                new RectF(16, 0, 1064, 1484),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ -200,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f));
    }

    @Test
    public void clampsBothBordersToTheViewportTopWhenNegative() {
        RectF safeArea =
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ -500,
                        /* safeAreaBottom= */ -200,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f);

        assertEquals(new RectF(16, 0, 1064, 0), safeArea);
    }

    @Test
    public void convertsViewPixelsToCssPixels() {
        assertEquals(
                new RectF(16, 316, 524, 734),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 600,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 2f));

        RectF safeArea =
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 600,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 2.625f);

        assertEquals(16f, safeArea.left, 0f);
        assertEquals(244.571f, safeArea.top, 0.001f);
        assertEquals(395.429f, safeArea.right, 0.001f);
        assertEquals(555.429f, safeArea.bottom, 0.001f);
    }

    @Test
    public void clampsLeftAndRightWhenWidthIsLessThanDoubleMargin() {
        assertEquals(
                new RectF(4, 616, 4, 1484),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 600,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 20,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f));
    }

    @Test
    public void clampsLeftAndRightWhenWidthIsLessThanMargin() {
        assertEquals(
                new RectF(0, 616, 0, 1484),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 600,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 10,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f));
    }

    @Test
    public void clampsTopAndBottomWhenHeightIsLessThanDoubleMargin() {
        assertEquals(
                new RectF(16, 4, 1064, 4),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 0,
                        /* safeAreaBottom= */ 20,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 20,
                        /* density= */ 1f));
    }

    @Test
    public void clampsTopAndBottomWhenHeightIsLessThanMargin() {
        assertEquals(
                new RectF(16, 0, 1064, 0),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 0,
                        /* safeAreaBottom= */ 10,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 10,
                        /* density= */ 1f));
    }
}
