/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.fullscreen;

import android.app.Activity;

import org.chromium.chrome.browser.app.BraveActivity;

/**
 * Super class of {@link FullscreenHtmlApiHandlerBase} introduced via bytecode changes. For more
 * info see {@link org.brave.bytecode.BraveFullscreenHtmlApiHandlerBaseClassAdapter}.
 */
public abstract class BraveFullscreenHtmlApiHandlerBase {
    /**
     * Field accessed using {@code BraveFullscreenHtmlApiHandlerBase.class.cast(this)}. Used to keep
     * track whether a tab was hidden by {@code TabHidingType.CHANGED_TABS} reason.
     *
     * <p>Note: {@link FullscreenHtmlApiHandlerBase} contains a change introduced by a patch.
     */
    protected boolean mTabHiddenByChangedTabs;

    /**
     * Restores browser fullscreen UI state for a Brave-managed Picture-in-Picture return without
     * asking the WebContents to exit fullscreen synchronously.
     */
    public void exitPersistentFullscreenModeForPictureInPicture() {}

    /**
     * Invoked from upstream observer hooks (tab hidden, activity stopped) to decide whether the
     * persistent fullscreen teardown should be skipped. When a Brave-managed YouTube
     * Picture-in-Picture session is alive we want to keep both the browser-chrome and DOM
     * fullscreen state intact across transient hides (screen-off, PiP window occlusion) so there
     * is no visible flicker when the user returns. {@code activity} is the activity bound to the
     * calling fullscreen handler so that multi-window setups only preserve state for the window
     * that owns the PiP session.
     */
    public boolean shouldPreservePersistentFullscreenForPictureInPicture(Activity activity) {
        return activity instanceof final BraveActivity braveActivity
                && braveActivity.isYouTubePictureInPictureActive();
    }
}
