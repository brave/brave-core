/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowSystemClock;

import org.chromium.base.test.BaseRobolectricTestRunner;

import java.time.Duration;

/** Unit tests for {@link BraveExternalActivityLaunchTracker}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveExternalActivityLaunchTrackerTest {
    @After
    public void tearDown() {
        BraveExternalActivityLaunchTracker.resetForTesting();
    }

    @Test
    public void noLaunchRecorded_returnsNegativeOne() {
        assertEquals(-1, BraveExternalActivityLaunchTracker.consumeElapsedMsSinceLastLaunch());
    }

    @Test
    public void afterLaunch_returnsElapsedTime() {
        BraveExternalActivityLaunchTracker.notifyLaunchingExternalActivity();
        ShadowSystemClock.advanceBy(Duration.ofMillis(500));

        long elapsed = BraveExternalActivityLaunchTracker.consumeElapsedMsSinceLastLaunch();

        assertTrue(elapsed >= 500);
    }

    @Test
    public void consuming_clearsTheRecordedLaunch() {
        BraveExternalActivityLaunchTracker.notifyLaunchingExternalActivity();

        BraveExternalActivityLaunchTracker.consumeElapsedMsSinceLastLaunch();

        assertEquals(-1, BraveExternalActivityLaunchTracker.consumeElapsedMsSinceLastLaunch());
    }

    @Test
    public void secondLaunch_resetsTheClock() {
        BraveExternalActivityLaunchTracker.notifyLaunchingExternalActivity();
        ShadowSystemClock.advanceBy(Duration.ofMinutes(5));

        BraveExternalActivityLaunchTracker.notifyLaunchingExternalActivity();
        long elapsed = BraveExternalActivityLaunchTracker.consumeElapsedMsSinceLastLaunch();

        assertTrue(elapsed < Duration.ofMinutes(5).toMillis());
    }
}
