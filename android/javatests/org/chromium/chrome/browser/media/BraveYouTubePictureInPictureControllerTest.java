/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.os.Bundle;

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
import org.chromium.content_public.browser.WebContents;

/**
 * Unit tests for {@link BraveYouTubePictureInPictureController}'s pure state-transition surface.
 */
@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveYouTubePictureInPictureControllerTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BraveActivity mBraveActivity;
    @Mock private WebContents mWebContents;

    @Test
    @SmallTest
    public void initiallyInactive() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        assertFalse(controller.isActive());
        assertFalse(controller.isExitingForTesting());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    @SmallTest
    public void onSessionRequested_marksActive() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        controller.onSessionRequested(mWebContents);

        assertTrue(controller.isActive());
        assertFalse(controller.isExitingForTesting());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    @SmallTest
    public void onSessionEnterFailed_clearsActive() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onSessionRequested(mWebContents);

        controller.onSessionEnterFailed();

        assertFalse(controller.isActive());
        assertFalse(controller.isExitingForTesting());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    @SmallTest
    public void onFullscreenInterrupted_whenInactive_isNoop() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        controller.onFullscreenInterrupted();

        assertFalse(controller.isActive());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    @SmallTest
    public void saveAndRestore_preservesActive() {
        BraveYouTubePictureInPictureController source =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        source.onSessionRequested(mWebContents);

        Bundle out = new Bundle();
        source.onSaveInstanceState(out);

        BraveYouTubePictureInPictureController restored =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        restored.onPostCreate(out);

        assertTrue(restored.isActive());
        // Web contents reference is intentionally not persisted; recovery picks up from the
        // current activity tab on the next visible-after-screen-on tick.
        assertFalse(restored.isExitingForTesting());
    }

    @Test
    @SmallTest
    public void onPostCreate_withNullBundle_initializesInactive() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        controller.onPostCreate(null);

        assertFalse(controller.isActive());
    }
}
