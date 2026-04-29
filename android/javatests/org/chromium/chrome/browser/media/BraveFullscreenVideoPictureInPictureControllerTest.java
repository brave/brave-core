/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
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
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BraveActivity mBraveActivity;

    private final BraveFullscreenVideoPictureInPictureController mController =
            new BraveFullscreenVideoPictureInPictureController();

    @Test
    @SmallTest
    public void maybeHandleDismiss_activeYouTubePictureInPictureDefersResumeCleanup() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ true,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ false);

        assertTrue(handled);
        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void maybeHandleDismiss_activeYouTubePictureInPictureDefersStartCleanup() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ true,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ false);

        assertTrue(handled);
        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void maybeHandleDismiss_activePictureInPictureKeepsPictureInPictureAlive() {
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ true,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ false);

        assertTrue(handled);
        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void maybeHandleDismiss_webContentsSignalDuringActivePictureInPicture_keepsAlive() {
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ true,
                        /* isNewTab= */ false);

        assertTrue(handled);
        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void maybeHandleDismiss_inactiveYouTubeAndNotInPip_doesNotKeepAlive() {
        // Not a YT PiP session and not in PiP: the hook should fall through to the upstream
        // dismiss logic.
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(false);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(false);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ true,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ false);

        assertFalse(handled);
        assertTrue(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void maybeHandleDismiss_newTabDuringActiveYouTubePictureInPicture_keepsForeground() {
        // A new tab arriving during a Brave-managed YouTube PiP session must skip upstream's
        // moveTaskToBack(true) so the activity stays in the foreground (e.g. when the user taps
        // the "New tab" launcher shortcut while PiP is active). The hook also signals the
        // controller to drop persistent fullscreen so the new tab is rendered correctly.
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ true);

        assertTrue(handled);
        assertFalse(mController.mDismissPending);
        verify(mBraveActivity).onYouTubePictureInPictureNewTab();
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void maybeHandleDismiss_newTabWhenActiveButNotInPictureInPicture_doesNotKeepAlive() {
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(false);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ true);

        assertFalse(handled);
        assertTrue(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureNewTab();
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    @SmallTest
    public void maybeHandleDismiss_newTabWithoutActiveYouTubePictureInPicture_doesNotKeepAlive() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(false);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ true);

        assertFalse(handled);
        assertTrue(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureNewTab();
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }
}
