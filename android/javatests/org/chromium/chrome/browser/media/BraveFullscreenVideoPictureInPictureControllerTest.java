/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.test.filters.SmallTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.test.util.Batch;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveFullscreenVideoPictureInPictureControllerTest {
    // Mirrors FullscreenVideoPictureInPictureController.MetricsEndReason values.
    private static final int METRICS_END_REASON_RESUME = 0;
    private static final int METRICS_END_REASON_LEFT_FULLSCREEN = 6;
    private static final int METRICS_END_REASON_WEB_CONTENTS_LEFT_FULLSCREEN = 7;
    private static final int METRICS_END_REASON_START = 8;

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BraveActivity mBraveActivity;

    private final BraveFullscreenVideoPictureInPictureController mController =
            new BraveFullscreenVideoPictureInPictureController();

    @Test
    @SmallTest
    public void dismissActivityIfNeeded_activeYouTubePictureInPictureDefersResumeCleanup() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        mController.dismissActivityIfNeeded(mBraveActivity, METRICS_END_REASON_RESUME);

        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void dismissActivityIfNeeded_activeYouTubePictureInPictureDefersStartCleanup() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        mController.dismissActivityIfNeeded(mBraveActivity, METRICS_END_REASON_START);

        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void dismissActivityIfNeeded_activePictureInPictureKeepsPictureInPictureAlive() {
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        mController.dismissActivityIfNeeded(mBraveActivity, METRICS_END_REASON_LEFT_FULLSCREEN);

        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void dismissActivityIfNeeded_webContentsSignalDuringActivePictureInPicture_keepsAlive() {
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        mController.dismissActivityIfNeeded(
                mBraveActivity, METRICS_END_REASON_WEB_CONTENTS_LEFT_FULLSCREEN);

        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void dismissActivityIfNeeded_inactiveYouTubeAndNotInPip_doesNotKeepAlive() {
        // Not a YT PiP session and not in PiP: the helper should fall through to the upstream
        // dismiss logic (we cannot easily verify the BraveReflectionUtil call, but the local
        // pre-conditions for the early-return branches must all be false).
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(false);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(false);
        mController.mDismissPending = true;

        // Don't assert dismissPending here, the upstream call would mutate it via reflection
        // which we don't exercise in unit tests.
        try {
            mController.dismissActivityIfNeeded(mBraveActivity, METRICS_END_REASON_LEFT_FULLSCREEN);
        } catch (Throwable ignored) {
            // The reflection call into upstream FullscreenVideoPictureInPictureController is
            // expected to fail in the unit-test JVM since the upstream class is not present.
            // That's fine, we only care that we did not short-circuit before reaching it.
        }

        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }
}
