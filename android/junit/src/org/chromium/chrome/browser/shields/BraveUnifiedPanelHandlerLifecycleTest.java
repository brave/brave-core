/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.widget.PopupWindow;

import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.LifecycleOwner;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.android.controller.ActivityController;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;

import java.lang.reflect.Field;

/**
 * Tests that {@link BraveUnifiedPanelHandler} closes the Shields panel when its host Activity is
 * backgrounded (e.g. the app is switched away from, or the recent-apps/tab switcher is invoked), so
 * a panel left open on one tab can't be reached after the app leaves the foreground.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveUnifiedPanelHandlerLifecycleTest {
    private ActivityController<FragmentActivity> mActivityController;

    @After
    public void tearDown() {
        if (mActivityController != null) {
            mActivityController.pause().stop().destroy();
        }
    }

    @Test
    public void testPanelHidesWhenHostActivityPauses() throws Exception {
        mActivityController = Robolectric.buildActivity(FragmentActivity.class).setup();
        FragmentActivity activity = mActivityController.get();
        BraveUnifiedPanelHandler handler = new BraveUnifiedPanelHandler(activity);

        PopupWindow popupWindow = mock(PopupWindow.class);
        when(popupWindow.isShowing()).thenReturn(true);
        setPopupWindow(handler, popupWindow);

        mActivityController.pause();

        verify(popupWindow, times(1)).dismiss();
        assertFalse(handler.isShowing());
    }

    @Test
    public void testDestroyStopsObservingHostActivityLifecycle() throws Exception {
        mActivityController = Robolectric.buildActivity(FragmentActivity.class).setup();
        FragmentActivity activity = mActivityController.get();
        BraveUnifiedPanelHandler handler = new BraveUnifiedPanelHandler(activity);

        PopupWindow popupWindow = mock(PopupWindow.class);
        when(popupWindow.isShowing()).thenReturn(true);
        setPopupWindow(handler, popupWindow);

        handler.destroy();
        verify(popupWindow, times(1)).dismiss();

        // Simulate the popup still being shown (as a real one could be, had destroy() not
        // dismissed it) to prove the observer was actually removed rather than merely not firing.
        when(popupWindow.isShowing()).thenReturn(true);
        mActivityController.pause();

        // The lifecycle observer must have been unregistered by destroy(), so pausing must not
        // trigger a second dismiss() call.
        verify(popupWindow, times(1)).dismiss();
    }

    @Test(expected = ClassCastException.class)
    public void testNonLifecycleOwnerActivityThrows() {
        // A bare android.app.Activity (as opposed to an AppCompatActivity/FragmentActivity) is
        // not a LifecycleOwner. BraveUnifiedPanelHandler requires its host Activity to be a
        // LifecycleOwner so it can guarantee the panel closes on backgrounding; construction must
        // fail fast here rather than silently skip that protection.
        Activity activity = Robolectric.buildActivity(Activity.class).create().get();
        new BraveUnifiedPanelHandler(activity);
    }

    @Test
    public void testProductionHostActivitiesAreLifecycleOwners() {
        // BraveUnifiedPanelHandler only ever hosts itself in one of these two Activity types
        // (see the constructor and ensureInitializedForCustomTabs()), and requires the host to be
        // a LifecycleOwner, crashing immediately if it isn't (see
        // testNonLifecycleOwnerActivityThrows). This asserts the invariant directly on the
        // concrete production types: if either ever stopped being a LifecycleOwner, every Shields
        // panel open would start crashing instead of the panel silently losing its
        // background-dismiss protection.
        assertTrue(
                "BraveActivity must be a LifecycleOwner for the Shields panel's"
                        + " background-dismiss protection to apply",
                LifecycleOwner.class.isAssignableFrom(BraveActivity.class));
        assertTrue(
                "CustomTabActivity must be a LifecycleOwner for the Shields panel's"
                        + " background-dismiss protection to apply",
                LifecycleOwner.class.isAssignableFrom(CustomTabActivity.class));
    }

    private static void setPopupWindow(BraveUnifiedPanelHandler handler, PopupWindow popupWindow)
            throws Exception {
        Field field = BraveUnifiedPanelHandler.class.getDeclaredField("mPopupWindow");
        field.setAccessible(true);
        field.set(handler, popupWindow);
    }
}
