/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.media;

import android.app.Activity;
import android.graphics.Bitmap;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.CommandLine;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.content.browser.MediaSessionImpl;
import org.chromium.content_public.browser.MediaSession;
import org.chromium.content_public.browser.MediaSessionObserver;
import org.chromium.content_public.browser.WebContents;
import org.chromium.services.media_session.MediaImage;
import org.chromium.services.media_session.MediaMetadata;
import org.chromium.services.media_session.MediaPosition;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.url.GURL;

import java.lang.ref.WeakReference;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class BraveMediaSessionHelper implements MediaImageCallback {
    private static final List<String> sBraveTalkHosts =
            Arrays.asList("talk.brave.com", "talk.bravesoftware.com", "talk.brave.software");
    private static final List<String> sBackgroundPlaybackHosts =
            Arrays.asList(
                    "music.youtube.com",
                    "www.youtube.com",
                    "m.youtube.com",
                    "youtube.com",
                    "open.spotify.com");

    // Tracks the WebContents that the Brave YouTube Picture-in-Picture controller has registered
    // as its active session, so the suppression logic in this helper can distinguish a real
    // Brave-managed session from any other YouTube tab whose hosting Activity happens to be in
    // PiP. A single static slot is sufficient because Android allows at most one PiP window per
    // device at a time.
    private static @Nullable WeakReference<WebContents> sYouTubePictureInPictureWebContents;

    public static void setYouTubePictureInPictureWebContents(
            @Nullable final WebContents webContents) {
        sYouTubePictureInPictureWebContents =
                webContents == null ? null : new WeakReference<>(webContents);
    }

    public static void clearYouTubePictureInPictureWebContents(
            @Nullable final WebContents webContents) {
        if (webContents == null) {
            sYouTubePictureInPictureWebContents = null;
            return;
        }

        if (sYouTubePictureInPictureWebContents == null) {
            return;
        }

        final WebContents pictureInPictureWebContents = sYouTubePictureInPictureWebContents.get();
        if (pictureInPictureWebContents == null
                || pictureInPictureWebContents == webContents
                || pictureInPictureWebContents.isDestroyed()) {
            sYouTubePictureInPictureWebContents = null;
        }
    }

    public static @Nullable WebContents getYouTubePictureInPictureWebContents() {
        if (sYouTubePictureInPictureWebContents == null) {
            return null;
        }

        final WebContents pictureInPictureWebContents = sYouTubePictureInPictureWebContents.get();
        if (pictureInPictureWebContents == null || pictureInPictureWebContents.isDestroyed()) {
            sYouTubePictureInPictureWebContents = null;
            return null;
        }

        return pictureInPictureWebContents;
    }

    public static boolean isBraveTalk(WebContents webContents) {
        if (webContents == null) {
            return false;
        }
        GURL pageUrl = webContents.getLastCommittedUrl();
        if (pageUrl.isValid()
                && pageUrl.getScheme().equals(UrlConstants.HTTPS_SCHEME)
                && sBraveTalkHosts.contains(pageUrl.getHost())) {
            return true;
        }

        return false;
    }

    private @Nullable WebContents getMediaSessionWebContents() {
        return (WebContents)
                BraveReflectionUtil.getField(MediaSessionHelper.class, "mWebContents", this);
    }

    private boolean shouldSuppressMediaNotificationActions() {
        return isBraveTalk(getMediaSessionWebContents());
    }

    @VisibleForTesting
    boolean shouldSuppressMediaPause(@Nullable WebContents webContents) {
        if (isBraveTalk(webContents)) {
            return true;
        }

        if (!isBackgroundPlaybackHost(webContents)) {
            return false;
        }
        return isBackgroundVideo() || isYouTubePictureInPicture(webContents);
    }

    // While the screen is locked, the media session transiently reports itself as not
    // controllable and paused when the current track ends. If that paused state reaches the
    // upstream helper, the media notification leaves its playback state and YouTube never
    // advances to the next track (brave-browser#53077), so Brave Talk and background playback
    // must report the session as still playing. An active Brave PiP session with background
    // playback disabled is deliberately excluded: its pause-on-lock flow relies on the real
    // paused state so SystemUI shows the correct action after unlock.
    @VisibleForTesting
    boolean shouldForcePlayingState(@Nullable WebContents webContents) {
        return isBraveTalk(webContents)
                || (isBackgroundPlaybackHost(webContents) && isBackgroundVideo());
    }

    @VisibleForTesting
    static boolean isBackgroundPlaybackHost(WebContents webContents) {
        if (webContents == null) return false;
        GURL pageUrl = webContents.getLastCommittedUrl();
        return pageUrl.isValid()
                && pageUrl.getScheme().equals(UrlConstants.HTTPS_SCHEME)
                && sBackgroundPlaybackHosts.contains(pageUrl.getHost());
    }

    @VisibleForTesting
    boolean isBackgroundVideo() {
        // We check the command line switch rather than reading the preference directly because
        // this class is in the components layer and cannot access Profile or UserPrefs (chrome
        // layer) without causing R8 module boundary violations in the AAB build. The switch is
        // set by BraveBrowserMainParts::PostProfileInit() when the background video playback
        // feature and preference are both enabled, and the app restarts whenever the preference
        // changes, so the switch reliably reflects the current preference state.
        // In C++ this switch is defined as switches::kDisableBackgroundMediaSuspend.
        return CommandLine.getInstance().hasSwitch("disable-background-media-suspend");
    }

    @VisibleForTesting
    boolean isYouTubePictureInPicture(WebContents webContents) {
        if (!isYouTubePictureInPictureWebContents(webContents)) {
            return false;
        }

        final WindowAndroid windowAndroid = webContents.getTopLevelNativeWindow();
        if (windowAndroid == null) {
            return false;
        }

        final WeakReference<Activity> activityRef = windowAndroid.getActivity();
        if (activityRef == null) {
            return false;
        }

        final Activity activity = activityRef.get();
        return activity != null && activity.isInPictureInPictureMode();
    }

    @VisibleForTesting
    static boolean isYouTubePictureInPictureWebContents(@Nullable WebContents webContents) {
        if (webContents == null || webContents.isDestroyed()) {
            return false;
        }

        return getYouTubePictureInPictureWebContents() == webContents;
    }

    @Override
    public void onImageDownloaded(Bitmap image) {}

    public void showNotification() {
        MediaNotificationInfo.Builder notificationInfoBuilder =
                (MediaNotificationInfo.Builder)
                        BraveReflectionUtil.getField(
                                MediaSessionHelper.class, "mNotificationInfoBuilder", this);
        if (notificationInfoBuilder != null && shouldSuppressMediaNotificationActions()) {
            notificationInfoBuilder.setActions(0);
            HashSet<Integer> actionSet = new HashSet<Integer>();
            actionSet.add(0);
            notificationInfoBuilder.setMediaSessionActions(actionSet);
            // The reflected field MediaSessionHelper#mMediaSessionActions is declared as
            // Set<Integer>, but getField returns Object so the cast is unverifiable at compile
            // time due to generic erasure.
            @SuppressWarnings("unchecked")
            Set<Integer> mediaSessionActions =
                    (Set<Integer>)
                            BraveReflectionUtil.getField(
                                    MediaSessionHelper.class, "mMediaSessionActions", this);
            if (mediaSessionActions != null) {
                mediaSessionActions = actionSet;
            }
        }
        BraveReflectionUtil.invokeMethod(MediaSessionHelper.class, this, "showNotification");
    }

    protected MediaSessionObserver createMediaSessionObserver(MediaSession mediaSession) {
        MediaSessionObserver mediaSessionObserver =
                (MediaSessionObserver)
                        BraveReflectionUtil.invokeMethod(
                                MediaSessionHelper.class,
                                this,
                                "createMediaSessionObserver",
                                MediaSession.class,
                                mediaSession);
        assert mediaSessionObserver != null;

        if (!(mediaSession instanceof MediaSessionImpl)) {
            return mediaSessionObserver;
        }
        ((MediaSessionImpl) mediaSession).removeObserver(mediaSessionObserver);
        return new MediaSessionObserver(mediaSession) {
            @Override
            public void mediaSessionDestroyed() {
                mediaSessionObserver.mediaSessionDestroyed();
            }

            @Override
            public void mediaSessionStateChanged(boolean isControllable, boolean isPaused) {
                // Keep the Android media controls alive for Brave Talk, background YouTube audio,
                // and the active Brave-managed YouTube PiP session when the page transiently
                // reports itself as not controllable. For Brave Talk and background playback the
                // session is also reported as playing so the next track can start while the
                // screen is locked (see shouldForcePlayingState); the PiP-only case preserves
                // the real paused state so SystemUI shows the correct action and forwards the
                // right command after wake/restore transitions.
                if (!isControllable) {
                    WebContents webContents = getMediaSessionWebContents();
                    if (shouldSuppressMediaPause(webContents)) {
                        isControllable = true;
                        if (shouldForcePlayingState(webContents)) {
                            isPaused = false;
                        }
                    }
                }
                mediaSessionObserver.mediaSessionStateChanged(isControllable, isPaused);
            }

            @Override
            public void mediaSessionMetadataChanged(MediaMetadata metadata) {
                mediaSessionObserver.mediaSessionMetadataChanged(metadata);
            }

            @Override
            public void mediaSessionActionsChanged(Set<Integer> actions) {
                mediaSessionObserver.mediaSessionActionsChanged(actions);
            }

            @Override
            public void mediaSessionArtworkChanged(List<MediaImage> images) {
                mediaSessionObserver.mediaSessionArtworkChanged(images);
            }

            @Override
            public void mediaSessionPositionChanged(@Nullable MediaPosition position) {
                mediaSessionObserver.mediaSessionPositionChanged(position);
            }
        };
    }
}
