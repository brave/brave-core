/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.media;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import android.app.Activity;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.test.util.Batch;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.url.GURL;

import java.lang.ref.WeakReference;

/**
 * Unit tests for the YouTube-aware predicates that {@link BraveMediaSessionHelper} uses to decide
 * whether to suppress media-session pause events.
 */
@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveMediaSessionHelperTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private WebContents mWebContents;
    @Mock private WebContents mOtherWebContents;
    @Mock private WindowAndroid mWindowAndroid;
    @Mock private Activity mActivity;

    private final BraveMediaSessionHelper mHelper = new BraveMediaSessionHelper();

    private void mockUrl(String url) {
        when(mWebContents.getLastCommittedUrl()).thenReturn(new GURL(url));
    }

    @After
    public void tearDown() {
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(null);
    }

    @Test
    @SmallTest
    public void isBraveTalk_recognizesProductionHost() {
        mockUrl("https://talk.brave.com/room");
        assertTrue(BraveMediaSessionHelper.isBraveTalk(mWebContents));
    }

    @Test
    @SmallTest
    public void isBraveTalk_rejectsHttpScheme() {
        mockUrl("http://talk.brave.com/room");
        assertFalse(BraveMediaSessionHelper.isBraveTalk(mWebContents));
    }

    @Test
    @SmallTest
    public void isBraveTalk_rejectsUnrelatedHost() {
        mockUrl("https://www.example.com/");
        assertFalse(BraveMediaSessionHelper.isBraveTalk(mWebContents));
    }

    @Test
    @SmallTest
    public void isYouTube_recognizesMobileHost() {
        mockUrl("https://m.youtube.com/watch?v=dQw4w9WgXcQ");
        assertTrue(BraveMediaSessionHelper.isBackgroundPlaybackHost(mWebContents));
    }

    @Test
    @SmallTest
    public void isYouTube_rejectsThirdPartyHost() {
        mockUrl("https://www.example.com/watch");
        assertFalse(BraveMediaSessionHelper.isBackgroundPlaybackHost(mWebContents));
    }

    @Test
    @SmallTest
    public void isYouTubePictureInPicture_returnsFalseWithoutActivity() {
        when(mWebContents.getTopLevelNativeWindow()).thenReturn(mWindowAndroid);
        when(mWindowAndroid.getActivity()).thenReturn(new WeakReference<>(null));

        assertFalse(mHelper.isYouTubePictureInPicture(mWebContents));
    }

    @Test
    @SmallTest
    public void isYouTubePictureInPicture_returnsFalseWhenActivityInPipWithoutBraveSession() {
        when(mWebContents.getTopLevelNativeWindow()).thenReturn(mWindowAndroid);
        when(mWindowAndroid.getActivity()).thenReturn(new WeakReference<>(mActivity));
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);

        assertFalse(mHelper.isYouTubePictureInPicture(mWebContents));
    }

    @Test
    @SmallTest
    public void isYouTubePictureInPicture_returnsTrueForBraveManagedSession() {
        when(mWebContents.getTopLevelNativeWindow()).thenReturn(mWindowAndroid);
        when(mWindowAndroid.getActivity()).thenReturn(new WeakReference<>(mActivity));
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(mWebContents);

        assertTrue(mHelper.isYouTubePictureInPicture(mWebContents));
    }

    @Test
    @SmallTest
    public void isYouTubePictureInPicture_returnsFalseWhenActivityNotInPip() {
        when(mWebContents.getTopLevelNativeWindow()).thenReturn(mWindowAndroid);
        when(mWindowAndroid.getActivity()).thenReturn(new WeakReference<>(mActivity));
        when(mActivity.isInPictureInPictureMode()).thenReturn(false);
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(mWebContents);

        assertFalse(mHelper.isYouTubePictureInPicture(mWebContents));
    }

    @Test
    @SmallTest
    public void isYouTubePictureInPicture_returnsFalseForDifferentBraveManagedSession() {
        when(mWebContents.getTopLevelNativeWindow()).thenReturn(mWindowAndroid);
        when(mWindowAndroid.getActivity()).thenReturn(new WeakReference<>(mActivity));
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(mOtherWebContents);

        assertFalse(mHelper.isYouTubePictureInPicture(mWebContents));
    }
}
