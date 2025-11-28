/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.ClassRule;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
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
import org.chromium.third_party.android.swiperefresh.SwipeRefreshLayout;
import org.chromium.third_party.android.swiperefresh.SwipeRefreshLayout.OnRefreshListener;
import org.chromium.third_party.android.swiperefresh.SwipeRefreshLayout.OnResetListener;
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
    private OnRefreshListener mOnRefreshListener;
    private OnResetListener mOnResetListener;
    private SwipeRefreshLayout mSwipeRefreshLayout;
    private final SwipeRefreshHandler.SwipeRefreshLayoutCreator mSwipeRefreshLayoutCreator =
            context -> {
                mSwipeRefreshLayout = mock();
                doAnswer((invocation) -> mOnRefreshListener = invocation.getArgument(0))
                        .when(mSwipeRefreshLayout)
                        .setOnRefreshListener(any());
                doAnswer((invocation) -> mOnResetListener = invocation.getArgument(0))
                        .when(mSwipeRefreshLayout)
                        .setOnResetListener(any());
                return mSwipeRefreshLayout;
            };

    @BeforeClass
    public static void setUpSuite() {
        activityTestRule.launchActivity(null);
        sAccessibilitySwipeRefreshString =
                activityTestRule.getActivity().getString(ACCESSIBILITY_SWIPE_REFRESH_STRING_ID);
    }

    @Before
    public void setup() {
        when(mTab.getContext()).thenReturn(activityTestRule.getActivity());
        when(mTab.getUserDataHost()).thenReturn(new UserDataHost());
        when(mTab.getContentView()).thenReturn(mock());
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

        // By default ignorePullToRefresh is set to false
        triggerRefresh(braveHandler);

        assertNotNull(mSwipeRefreshLayout);
        Mockito.verify(mSwipeRefreshLayout, Mockito.times(1)).start();
    }

    @Test
    @SmallTest
    public void testSetIgnorePullToRefresh() {
        BraveSwipeRefreshHandler braveHandler = createBraveHandler();

        braveHandler.setIgnorePullToRefresh(false);
        triggerRefresh(braveHandler);

        assertNotNull(mSwipeRefreshLayout);
        Mockito.verify(mSwipeRefreshLayout, Mockito.times(1)).start();

        clearInvocations(mSwipeRefreshLayout);

        braveHandler.setIgnorePullToRefresh(true);
        triggerRefresh(braveHandler);
        Mockito.verify(mSwipeRefreshLayout, Mockito.never()).start();
    }

    /**
     * Triggers a refresh. We need to do this through the handler so that the SwipeRefreshLayout is
     * initialized.
     *
     * @param handler The {@link SwipeRefreshHandler} to use.
     */
    private void triggerRefresh(BraveSwipeRefreshHandler handler) {
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
        if (mOnRefreshListener != null) mOnRefreshListener.onRefresh();
    }

    private BraveSwipeRefreshHandler createBraveHandler() {
        SwipeRefreshHandler chromeHandler =
                SwipeRefreshHandler.from(mTab, mSwipeRefreshLayoutCreator);
        chromeHandler.initWebContents(mock()); // Needed to enable the overscroll refresh handler.
        return BraveSwipeRefreshHandler.from(mTab);
    }
}
