/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.KeyguardManager;
import android.content.Context;
import android.os.PowerManager;

import androidx.test.filters.SmallTest;

import org.junit.Before;
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
    private static final int METRICS_END_REASON_LEFT_FULLSCREEN = 6;
    private static final int METRICS_END_REASON_WEB_CONTENTS_LEFT_FULLSCREEN = 7;

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BraveActivity mBraveActivity;
    @Mock private PowerManager mPowerManager;
    @Mock private KeyguardManager mKeyguardManager;

    private BraveFullscreenVideoPictureInPictureController mController;

    @Before
    public void setUp() {
        mController = new BraveFullscreenVideoPictureInPictureController();

        when(mBraveActivity.getSystemService(Context.POWER_SERVICE)).thenReturn(mPowerManager);
        when(mBraveActivity.getSystemService(Context.KEYGUARD_SERVICE)).thenReturn(mKeyguardManager);
        when(mPowerManager.isInteractive()).thenReturn(true);
        when(mKeyguardManager.isKeyguardLocked()).thenReturn(false);
    }

    @Test
    @SmallTest
    public void dismissActivityIfNeeded_screenOffKeepsPictureInPictureAlive() {
        when(mPowerManager.isInteractive()).thenReturn(false);
        mController.mDismissPending = true;

        mController.dismissActivityIfNeeded(mBraveActivity, METRICS_END_REASON_LEFT_FULLSCREEN);

        assertFalse(mController.mDismissPending);
        verify(mBraveActivity).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void dismissActivityIfNeeded_keyguardLockedKeepsPictureInPictureAlive() {
        when(mKeyguardManager.isKeyguardLocked()).thenReturn(true);
        mController.mDismissPending = true;

        mController.dismissActivityIfNeeded(mBraveActivity, METRICS_END_REASON_LEFT_FULLSCREEN);

        assertFalse(mController.mDismissPending);
        verify(mBraveActivity).onYouTubePictureInPictureFullscreenInterrupted();
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
}
