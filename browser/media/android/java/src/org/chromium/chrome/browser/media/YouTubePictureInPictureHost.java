/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import org.chromium.build.annotations.NullMarked;
import org.chromium.content_public.browser.WebContents;

/**
 * Implemented by an Activity that owns Brave-managed YouTube picture-in-picture sessions, so
 * callers can drive a session without depending on BraveActivity.
 */
@NullMarked
public interface YouTubePictureInPictureHost {
    /** Called before entering PiP for a YouTube video playing in {@code webContents}. */
    void onYouTubePictureInPictureRequested(WebContents webContents);

    /**
     * Called when the PiP entry requested via {@link #onYouTubePictureInPictureRequested} fails.
     */
    void onYouTubePictureInPictureEnterFailed();

    /**
     * Returns true from {@link #onYouTubePictureInPictureRequested} until the session ends (PiP
     * exit, entry failure or activity destroy), including while it is paused by a screen-lock
     * interruption and no PiP window is showing.
     */
    boolean isYouTubePictureInPictureActive();

    /** Called when the session's video leaves fullscreen while the screen is off or locked. */
    void onYouTubePictureInPictureFullscreenInterrupted();

    /** Called when a new tab is opened while the session is in PiP. */
    void onYouTubePictureInPictureNewTab();
}
