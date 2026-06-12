/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.fullscreen;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import android.app.Activity;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.app.BraveActivity;

@RunWith(BaseRobolectricTestRunner.class)
public class BraveFullscreenHtmlApiHandlerBaseTest {
    private static final class TestBraveFullscreenHtmlApiHandlerBase
            extends BraveFullscreenHtmlApiHandlerBase {}

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Activity mActivity;
    @Mock private BraveActivity mBraveActivity;

    private final TestBraveFullscreenHtmlApiHandlerBase mHandler =
            new TestBraveFullscreenHtmlApiHandlerBase();

    @Test
    public void shouldPreservePersistentFullscreenForPictureInPicture_nonBraveActivity() {
        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    public void shouldPreservePersistentFullscreenForPictureInPicture_inactiveBraveActivity() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(false);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mBraveActivity));
    }

    @Test
    public void shouldPreservePersistentFullscreenForPictureInPicture_activeBraveActivity() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);

        assertTrue(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mBraveActivity));
    }
}
