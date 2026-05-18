/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.fullscreen;

import android.app.Activity;

import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.tab.TabHidingType;

/**
 * Super class of {@link FullscreenHtmlApiHandlerBase} introduced via bytecode changes. For more
 * info see {@link org.brave.bytecode.BraveFullscreenHtmlApiHandlerBaseClassAdapter}.
 *
 * <p>The hooks below are called from upstream FullscreenHtmlApiHandlerBase via Plaster
 * substitutions; if the names or signatures change, update the corresponding entries in {@code
 * rewrite/chrome/android/.../FullscreenHtmlApiHandlerBase.java.toml}.
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
     * Picture-in-Picture session is alive we want to keep both the browser and DOM fullscreen state
     * intact across transient hides (screen-off, PiP window occlusion) so there is no visible
     * flicker when the user returns. {@code activity} is the activity bound to the calling
     * fullscreen handler so that multi-window setups only preserve state for the window that owns
     * the PiP session.
     */
    public boolean shouldPreservePersistentFullscreenForPictureInPicture(Activity activity) {
        return activity instanceof final BraveActivity braveActivity
                && braveActivity.isYouTubePictureInPictureActive();
    }

    /**
     * Tab-hidden variant of {@link #shouldPreservePersistentFullscreenForPictureInPicture}. Records
     * whether the tab was hidden because of a tab switch (so the Compat/Legacy
     * {@code exitPersistentFullscreenMode} override can still tear down fullscreen UI in that case)
     * and returns whether upstream should skip its tab-hidden fullscreen exit.
     */
    protected boolean maybeSkipExitFullscreenOnTabHidden(
            Activity activity, @TabHidingType int reason) {
        mTabHiddenByChangedTabs = reason == TabHidingType.CHANGED_TABS;
        return shouldPreservePersistentFullscreenForPictureInPicture(activity);
    }
}
