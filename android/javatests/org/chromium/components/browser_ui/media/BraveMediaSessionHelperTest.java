/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.media;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
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

import org.chromium.base.CommandLine;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Batch;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.MediaSessionImpl;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.content_public.browser.test.NativeLibraryTestUtils;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.url.GURL;

import java.lang.ref.WeakReference;

/** Tests for media pause suppression and notification lifetime. */
@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveMediaSessionHelperTest {
    // Java-side mirror of switches::kDisableBackgroundMediaSuspend, set at startup when the
    // background video playback feature and preference are both enabled.
    private static final String DISABLE_BACKGROUND_MEDIA_SUSPEND =
            "disable-background-media-suspend";

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock(extraInterfaces = WebContentsObserver.Observable.class)
    private WebContents mWebContents;

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
        CommandLine.getInstance().removeSwitch(DISABLE_BACKGROUND_MEDIA_SUSPEND);
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

    @Test
    @SmallTest
    public void shouldForcePlayingState_trueForYouTubeWithBackgroundPlayback() {
        mockUrl("https://m.youtube.com/watch?v=dQw4w9WgXcQ");
        CommandLine.getInstance().appendSwitch(DISABLE_BACKGROUND_MEDIA_SUSPEND);

        assertTrue(mHelper.shouldForcePlayingState(mWebContents));
    }

    @Test
    @SmallTest
    public void shouldForcePlayingState_falseForYouTubeWithoutBackgroundPlayback() {
        mockUrl("https://m.youtube.com/watch?v=dQw4w9WgXcQ");

        assertFalse(mHelper.shouldForcePlayingState(mWebContents));
    }

    @Test
    @SmallTest
    public void shouldForcePlayingState_trueForBraveTalk() {
        mockUrl("https://talk.brave.com/room");

        assertTrue(mHelper.shouldForcePlayingState(mWebContents));
    }

    @Test
    @SmallTest
    public void shouldForcePlayingState_falseForPipSessionWithoutBackgroundPlayback() {
        mockUrl("https://m.youtube.com/watch?v=dQw4w9WgXcQ");
        when(mWebContents.getTopLevelNativeWindow()).thenReturn(mWindowAndroid);
        when(mWindowAndroid.getActivity()).thenReturn(new WeakReference<>(mActivity));
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(mWebContents);

        // The pause suppression still applies, so the media controls stay alive, but the real
        // paused state is preserved for the PiP pause-on-lock flow.
        assertTrue(mHelper.shouldSuppressMediaPause(mWebContents));
        assertFalse(mHelper.shouldForcePlayingState(mWebContents));
    }

    @Test
    @SmallTest
    public void shouldSuppressMediaPause_trueForYouTubeWithBackgroundPlayback() {
        mockUrl("https://m.youtube.com/watch?v=dQw4w9WgXcQ");
        CommandLine.getInstance().appendSwitch(DISABLE_BACKGROUND_MEDIA_SUSPEND);

        assertTrue(mHelper.shouldSuppressMediaPause(mWebContents));
    }

    @Test
    @SmallTest
    public void shouldSuppressMediaPause_falseForYouTubeWithoutBackgroundPlaybackOrPip() {
        mockUrl("https://m.youtube.com/watch?v=dQw4w9WgXcQ");

        assertFalse(mHelper.shouldSuppressMediaPause(mWebContents));
    }

    @Test
    @SmallTest
    public void reloadClearsPreservedMediaNotification() {
        NativeLibraryTestUtils.loadNativeLibraryNoBrowserProcess();
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    // Set up a YouTube Music page with background playback enabled.
                    CommandLine.getInstance().appendSwitch(DISABLE_BACKGROUND_MEDIA_SUSPEND);
                    MediaSessionHelper.Delegate delegate = mock(MediaSessionHelper.Delegate.class);
                    MediaSessionHelper helper =
                            createMediaSessionHelper(
                                    "https://music.youtube.com/watch?v=qrZhOZyXc-I", delegate);
                    try {
                        // Simulate the initial page load.
                        NavigationHandle navigation = mock(NavigationHandle.class);
                        when(navigation.hasCommitted()).thenReturn(true);
                        helper.mWebContentsObserver.didFinishNavigationInPrimaryMainFrame(
                                navigation);

                        // Start playback, then preserve controls when it becomes uncontrollable.
                        helper.mMediaSessionObserver.mediaSessionStateChanged(true, false);
                        helper.mMediaSessionObserver.mediaSessionStateChanged(false, true);
                        assertFalse(helper.mNotificationInfoBuilder.build().isPaused);

                        // A playlist URL change must keep the existing background controls.
                        when(navigation.isSameDocument()).thenReturn(true);
                        helper.mWebContentsObserver.didFinishNavigationInPrimaryMainFrame(
                                navigation);
                        assertFalse(helper.mNotificationInfoBuilder.build().isPaused);
                        clearInvocations(delegate);

                        // A navigation that does not commit must leave the current controls intact.
                        when(navigation.isSameDocument()).thenReturn(false);
                        when(navigation.hasCommitted()).thenReturn(false);
                        helper.mWebContentsObserver.didFinishNavigationInPrimaryMainFrame(
                                navigation);
                        assertFalse(helper.mNotificationInfoBuilder.build().isPaused);

                        // Later session updates must still preserve those controls.
                        helper.mMediaSessionObserver.mediaSessionStateChanged(false, true);
                        assertFalse(helper.mNotificationInfoBuilder.build().isPaused);
                        clearInvocations(delegate);

                        // A committed reload must clear the preserved controls.
                        when(navigation.hasCommitted()).thenReturn(true);
                        helper.mWebContentsObserver.didFinishNavigationInPrimaryMainFrame(
                                navigation);
                        assertNull(helper.mNotificationInfoBuilder);

                        // A stale session update must not bring the old controls back.
                        helper.mMediaSessionObserver.mediaSessionStateChanged(false, true);
                        assertNull(helper.mNotificationInfoBuilder);
                        verify(delegate, never()).showMediaNotification(any());
                    } finally {
                        helper.destroy();
                        MediaSessionHelper.setOverriddenMediaSessionForTesting(null);
                    }
                });
    }

    @Test
    @SmallTest
    public void reloadKeepsBraveTalkMediaNotification() {
        NativeLibraryTestUtils.loadNativeLibraryNoBrowserProcess();
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    // Brave Talk keeps controls even with background playback disabled.
                    CommandLine.getInstance().removeSwitch(DISABLE_BACKGROUND_MEDIA_SUSPEND);
                    MediaSessionHelper.Delegate delegate = mock(MediaSessionHelper.Delegate.class);
                    MediaSessionHelper helper =
                            createMediaSessionHelper("https://talk.brave.com/room", delegate);
                    try {
                        NavigationHandle navigation = mock(NavigationHandle.class);
                        when(navigation.hasCommitted()).thenReturn(true);
                        helper.mWebContentsObserver.didFinishNavigationInPrimaryMainFrame(
                                navigation);

                        // Talk does not require a prior controllable session to keep controls.
                        helper.mMediaSessionObserver.mediaSessionStateChanged(false, true);
                        assertFalse(helper.mNotificationInfoBuilder.build().isPaused);
                        clearInvocations(delegate);

                        // Reloading Talk must keep its controls, unlike the YouTube case.
                        helper.mWebContentsObserver.didFinishNavigationInPrimaryMainFrame(
                                navigation);
                        assertFalse(helper.mNotificationInfoBuilder.build().isPaused);

                        helper.mMediaSessionObserver.mediaSessionStateChanged(false, true);
                        assertFalse(helper.mNotificationInfoBuilder.build().isPaused);
                        verify(delegate, never()).hideMediaNotification();
                    } finally {
                        helper.destroy();
                        MediaSessionHelper.setOverriddenMediaSessionForTesting(null);
                    }
                });
    }

    private MediaSessionHelper createMediaSessionHelper(
            String url, MediaSessionHelper.Delegate delegate) {
        mockUrl(url);
        when(mWebContents.getVisibleUrl()).thenReturn(new GURL(url));
        when(delegate.createMediaNotificationInfoBuilder())
                .thenAnswer(
                        invocation ->
                                new MediaNotificationInfo.Builder().setInstanceId(1).setId(1));
        // The Brave wrapper only handles MediaSessionImpl instances.
        MediaSessionHelper.setOverriddenMediaSessionForTesting(mock(MediaSessionImpl.class));
        return new MediaSessionHelper(mWebContents, delegate);
    }
}
