/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;

import android.app.Activity;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.test.BaseRobolectricTestRunner;

import java.util.concurrent.TimeUnit;

/** Unit tests for {@link BraveExclusiveAccessNotice}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveExclusiveAccessNoticeTest {
    private static final String TEXT = "example.com is in full screen. Swipe down to exit.";

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private ExclusiveAccessContext mExclusiveAccessContext;

    private Activity mActivity;
    private ViewGroup mContent;
    private boolean mInPictureInPicture;
    private BraveExclusiveAccessNotice mNotice;

    @Before
    public void setUp() {
        mActivity = Robolectric.buildActivity(Activity.class).setup().get();
        mContent = mActivity.findViewById(android.R.id.content);
        doReturn(mActivity).when(mExclusiveAccessContext).getActivity();
        mNotice =
                new BraveExclusiveAccessNotice(mExclusiveAccessContext) {
                    @Override
                    boolean isInPictureInPicture(Activity activity) {
                        return mInPictureInPicture;
                    }
                };
    }

    @After
    public void tearDown() {
        mNotice.hide();
        mActivity.finish();
    }

    private void advanceMs(long ms) {
        ShadowLooper.idleMainLooper(ms, TimeUnit.MILLISECONDS);
    }

    private int noticeCount() {
        int count = 0;
        for (int i = 0; i < mContent.getChildCount(); i++) {
            if (mContent.getChildAt(i) instanceof TextView) count++;
        }
        return count;
    }

    @Test
    public void showsTopCenterWithText() {
        assertTrue(mNotice.update(TEXT));

        assertTrue(mNotice.isVisible());
        assertEquals(1, noticeCount());
        TextView view = (TextView) mContent.getChildAt(mContent.getChildCount() - 1);
        assertEquals(TEXT, view.getText().toString());
        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) view.getLayoutParams();
        assertEquals(Gravity.TOP | Gravity.CENTER_HORIZONTAL, params.gravity);
        assertFalse(view.isClickable());
    }

    @Test
    public void hidesAfterFourSeconds() {
        mNotice.update(TEXT);

        advanceMs(BraveExclusiveAccessNotice.AUTO_HIDE_DELAY_MS - 1);
        assertTrue(mNotice.isVisible());
        advanceMs(1);
        assertFalse(mNotice.isVisible());
        assertEquals(0, noticeCount());
    }

    @Test
    public void sameTextWhileVisibleDoesNotRestartTimer() {
        mNotice.update(TEXT);
        advanceMs(3000);

        mNotice.update(TEXT);
        advanceMs(1000);

        assertFalse(mNotice.isVisible());
    }

    @Test
    public void newTextReplacesNoticeAndRestartsTimer() {
        mNotice.update(TEXT);
        advanceMs(3000);

        mNotice.update("other text");
        advanceMs(3000);

        assertEquals(1, noticeCount());
        assertTrue(mNotice.isVisible());
        advanceMs(1000);
        assertFalse(mNotice.isVisible());
    }

    @Test
    public void reshowsWhenWindowRegainsFocus() {
        mNotice.update(TEXT);
        advanceMs(BraveExclusiveAccessNotice.AUTO_HIDE_DELAY_MS);
        assertFalse(mNotice.isVisible());

        mNotice.onWindowFocusChanged(mActivity, /* hasFocus= */ false);
        assertFalse(mNotice.isVisible());
        mNotice.onWindowFocusChanged(mActivity, /* hasFocus= */ true);

        assertTrue(mNotice.isVisible());
        assertEquals(1, noticeCount());
        advanceMs(BraveExclusiveAccessNotice.AUTO_HIDE_DELAY_MS);
        assertFalse(mNotice.isVisible());
    }

    @Test
    public void focusRegainedWhileVisibleDoesNotStackNotices() {
        mNotice.update(TEXT);

        mNotice.onWindowFocusChanged(mActivity, true);

        assertEquals(1, noticeCount());
    }

    @Test
    public void focusRegainedWhileVisibleDoesNotRestartTimer() {
        mNotice.update(TEXT);
        advanceMs(3000);

        mNotice.onWindowFocusChanged(mActivity, false);
        mNotice.onWindowFocusChanged(mActivity, true);
        advanceMs(1000);

        assertFalse(mNotice.isVisible());
    }

    @Test
    public void layoutChangeDoesNotRestartTimer() {
        mNotice.update(TEXT);
        advanceMs(3000);

        // Simulates a rotation: the container is measured and laid out again.
        mContent.layout(0, 0, mContent.getHeight() + 100, mContent.getWidth() + 100);
        mContent.requestLayout();
        ShadowLooper.idleMainLooper();
        assertTrue(mNotice.isVisible());
        assertEquals(1, noticeCount());
        advanceMs(1000);

        assertFalse(mNotice.isVisible());
    }

    @Test
    public void ignoresFocusOfOtherActivity() {
        mNotice.update(TEXT);
        advanceMs(BraveExclusiveAccessNotice.AUTO_HIDE_DELAY_MS);

        mNotice.onWindowFocusChanged(Robolectric.buildActivity(Activity.class).get(), true);

        assertFalse(mNotice.isVisible());
    }

    @Test
    public void hideEndsSessionSoFocusDoesNotReshow() {
        mNotice.update(TEXT);

        mNotice.hide();
        assertFalse(mNotice.isVisible());
        mNotice.onWindowFocusChanged(mActivity, true);

        assertFalse(mNotice.isVisible());
        assertEquals(0, noticeCount());
    }

    @Test
    public void neverShownInPictureInPicture() {
        mInPictureInPicture = true;

        assertTrue(mNotice.update(TEXT));
        assertFalse(mNotice.isVisible());
        mNotice.onWindowFocusChanged(mActivity, true);

        assertFalse(mNotice.isVisible());
        assertEquals(0, noticeCount());
    }

    @Test
    public void enteringPictureInPictureHidesVisibleNotice() {
        mNotice.update(TEXT);
        assertTrue(mNotice.isVisible());

        mInPictureInPicture = true;
        mNotice.onPictureInPictureModeChanged(true);

        assertFalse(mNotice.isVisible());
        assertEquals(0, noticeCount());
    }

    @Test
    public void leavingPictureInPictureShowsNoticeAgain() {
        mNotice.update(TEXT);
        mNotice.onPictureInPictureModeChanged(true);
        assertFalse(mNotice.isVisible());

        mNotice.onPictureInPictureModeChanged(false);

        assertTrue(mNotice.isVisible());
    }

    @Test
    public void fallsBackToSnackbarWithoutActivity() {
        doReturn(null).when(mExclusiveAccessContext).getActivity();

        assertFalse(mNotice.update(TEXT));
        assertFalse(mNotice.isVisible());
    }

    @Test
    public void showWithoutTextDoesNothing() {
        assertTrue(mNotice.show(null));
        assertFalse(mNotice.isVisible());
    }
}
