/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.content.Context;

import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.ClassRule;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.ThreadUtils;
import org.chromium.base.UserDataHost;
import org.chromium.base.test.BaseActivityTestRule;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.embedder_support.view.ContentView;
import org.chromium.third_party.android.swiperefresh.SwipeRefreshLayout;
import org.chromium.ui.OverscrollAction;
import org.chromium.ui.base.BackGestureEventSwipeEdge;
import org.chromium.ui.test.util.BlankUiTestActivity;

/** Tests {@link BraveSwipeRefreshHandler}. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@Batch(Batch.PER_CLASS)
public class BraveSwipeRefreshHandlerTest {
    private static final int ACCESSIBILITY_SWIPE_REFRESH_STRING_ID =
            R.string.accessibility_swipe_refresh;

    @ClassRule
    public static BaseActivityTestRule<BlankUiTestActivity> activityTestRule =
            new BaseActivityTestRule<>(BlankUiTestActivity.class);

    // Can't initialize here since myActivityTestRule.getActivity() is still null.
    private static String sAccessibilitySwipeRefreshString;

    @Rule public MockitoRule mockitoRule = MockitoJUnit.rule();
    @Mock private Tab mTab;
    private StartCountingSwipeRefreshLayout mSwipeRefreshLayout;
    private final SwipeRefreshHandler.SwipeRefreshLayoutCreator mSwipeRefreshLayoutCreator =
            context -> {
                mSwipeRefreshLayout = new StartCountingSwipeRefreshLayout(context);
                return mSwipeRefreshLayout;
            };

    private static class StartCountingSwipeRefreshLayout extends SwipeRefreshLayout {
        int mStartCount;

        StartCountingSwipeRefreshLayout(Context context) {
            super(context);
        }

        @Override
        public boolean start() {
            mStartCount++;
            return super.start();
        }
    }

    @BeforeClass
    public static void setUpSuite() {
        activityTestRule.launchActivity(null);
        sAccessibilitySwipeRefreshString =
                activityTestRule.getActivity().getString(ACCESSIBILITY_SWIPE_REFRESH_STRING_ID);
    }

    @Before
    public void setup() {
        Activity activity = activityTestRule.getActivity();
        when(mTab.getContext()).thenReturn(activity);
        when(mTab.getUserDataHost()).thenReturn(new UserDataHost());
        ContentView contentView =
                ThreadUtils.runOnUiThreadBlocking(
                        () -> {
                            ContentView view =
                                    ContentView.createContentView(
                                            activity, /* webContents= */ null);
                            activity.setContentView(view);
                            return view;
                        });
        when(mTab.getContentView()).thenReturn(contentView);
    }

    @Test
    @SmallTest
    public void testPullToRefreshIgnored() {
        BraveSwipeRefreshHandler braveHandler = createBraveHandler();
        braveHandler.setIgnorePullToRefresh(true);
        triggerRefresh(braveHandler);

        // SwipeRefreshLayout is initialized in a lazy way during swipe itself
        // If it is empty - this means swipe didn't happen
        assertNull(mSwipeRefreshLayout);
    }

    @Test
    @SmallTest
    public void testPullToRefreshPassthrough() {
        BraveSwipeRefreshHandler braveHandler = createBraveHandler();

        // By default mIgnorePullToRefresh is set to false
        triggerRefresh(braveHandler);

        assertNotNull(mSwipeRefreshLayout);
        assertEquals(1, mSwipeRefreshLayout.mStartCount);
    }

    @Test
    @SmallTest
    public void testSetIgnorePullToRefresh() {
        BraveSwipeRefreshHandler braveHandler = createBraveHandler();

        braveHandler.setIgnorePullToRefresh(false);
        triggerRefresh(braveHandler);

        assertNotNull(mSwipeRefreshLayout);
        assertEquals(1, mSwipeRefreshLayout.mStartCount);

        braveHandler.setIgnorePullToRefresh(true);
        triggerRefresh(braveHandler);
        assertEquals(1, mSwipeRefreshLayout.mStartCount);
    }

    /**
     * Triggers a refresh. We need to do this through the handler so that the SwipeRefreshLayout is
     * initialized.
     *
     * @param handler The {@link SwipeRefreshHandler} to use.
     */
    private void triggerRefresh(SwipeRefreshHandler handler) {
        ThreadUtils.runOnUiThreadBlocking(
                () ->
                        handler.start(
                                OverscrollAction.PULL_TO_REFRESH,
                                // The left/right swipe direction is arbitrary for an action type of
                                // PULL_TO_REFRESH.
                                BackGestureEventSwipeEdge.LEFT));
        // # of pixels (of reasonably small value) which a finger moves across per one motion event.
        final float distancePx = 6.0f;
        for (int numPullSteps = 0; numPullSteps < 10; numPullSteps++) {
            ThreadUtils.runOnUiThreadBlocking(() -> handler.pull(0, distancePx));
        }
    }

    private BraveSwipeRefreshHandler createBraveHandler() {
        SwipeRefreshHandler chromeHandler =
                SwipeRefreshHandler.from(mTab, mSwipeRefreshLayoutCreator);
        chromeHandler.initWebContents(mock()); // Needed to enable the overscroll refresh handler.
        return (BraveSwipeRefreshHandler) chromeHandler;
    }
}
