/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.compositor.layouts.phone;

import android.graphics.Rect;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;

/**
 * Brave helper for {@link NewTabAnimationLayout}.
 *
 * <p>When Brave's bottom toolbar is active the top tab-switcher button may be GONE (empty rect) or
 * visible alongside the bottom one. In either case the bottom toolbar's tab-switcher button is the
 * correct animation destination. {@link #compute} encapsulates the fallback logic and returns a
 * {@link BraveTabSwitcherState} that callers unpack into the four variables that drive {@code
 * tabCreatedInBackground}: {@code tabSwitcherRect}, {@code tabSwitcherButtonIsVisible}, {@code
 * toolbarHeight}, and {@code isTopToolbar}.
 *
 * <p>Both classes share the same Java package so no import is needed in the patch.
 */
@NullMarked
public class BraveNewTabAnimationLayout {

    /** Immutable snapshot of the four variables that drive the background-tab animation. */
    public static class BraveTabSwitcherState {
        public final Rect rect;
        public final boolean isVisible;
        public final int toolbarHeight;
        public final boolean isTopToolbar;

        BraveTabSwitcherState(
                Rect rect, boolean isVisible, int toolbarHeight, boolean isTopToolbar) {
            this.rect = rect;
            this.isVisible = isVisible;
            this.toolbarHeight = toolbarHeight;
            this.isTopToolbar = isTopToolbar;
        }
    }

    /**
     * Returns a {@link BraveTabSwitcherState} for the background-tab animation.
     *
     * <p>When Brave's bottom toolbar is active and its tab-switcher button is visible, overrides
     * the supplied originals so the animation targets the bottom button. Otherwise returns the
     * originals unchanged (e.g. split-screen small-window mode where the bottom toolbar is hidden,
     * or when Brave bottom controls are not enabled).
     *
     * @param animationHostView The host view used to traverse to the root view.
     * @param origRect Original {@code tabSwitcherRect} from the top toolbar button.
     * @param origIsVisible Original {@code tabSwitcherButtonIsVisible}.
     * @param origToolbarHeight Original {@code toolbarHeight} (top-toolbar Y).
     * @param origIsTopToolbar Original {@code isTopToolbar} flag.
     */
    public static BraveTabSwitcherState compute(
            ViewGroup animationHostView,
            Rect origRect,
            boolean origIsVisible,
            int origToolbarHeight,
            boolean origIsTopToolbar) {
        if (BottomToolbarConfiguration.isBraveBottomControlsEnabled()) {
            View bottomToolbar = animationHostView.getRootView().findViewById(R.id.bottom_toolbar);
            if (bottomToolbar != null) {
                View bottomButton = bottomToolbar.findViewById(R.id.bottom_tab_switcher_button);
                if (bottomButton != null) {
                    Rect braveRect = new Rect();
                    if (bottomButton.getGlobalVisibleRect(braveRect)) {
                        return new BraveTabSwitcherState(
                                braveRect,
                                /* isVisible= */ true,
                                /* toolbarHeight= */ braveRect.top,
                                /* isTopToolbar= */ false);
                    }
                }
            }
        }
        return new BraveTabSwitcherState(
                origRect, origIsVisible, origToolbarHeight, origIsTopToolbar);
    }

    private BraveNewTabAnimationLayout() {}
}
