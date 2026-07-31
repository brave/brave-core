/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.compositor.layouts.phone;

import android.content.res.ColorStateList;
import android.graphics.Rect;
import android.view.View;

import androidx.annotation.ColorInt;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.theme.ThemeUtils;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.ui.theme.BrandedColorScheme;
import org.chromium.components.browser_ui.styles.ChromeColors;

/**
 * Brave extension of NewBackgroundTabAnimationData.
 *
 * <p>When isBraveBottomControlsEnabled() is true, BraveToolbarLayoutImpl hides
 * R.id.tab_switcher_button (top toolbar) via maybeHideTopTabSwitcherButton(). The upstream
 * captureState() then gets a GONE view: getGlobalVisibleRect returns an empty rect, setSize(0,0) is
 * called on the fake animation button, and an assertion fires in getButtonLocation().
 *
 * <p>Fix: override getTabSwitcherButtonRect() — the method tabCreatedInBackground() calls to size
 * and position the animation — to fall back to Brave's bottom toolbar tab switcher button when the
 * upstream rect is empty.
 */
@NullMarked
public class BraveNewBackgroundTabAnimationData extends NewBackgroundTabAnimationData {

    // mAnimationHostView is private in the parent so not directly accessible; store our own
    // reference to the constructor parameter instead.
    private final View mBraveAnimationHostView;

    public BraveNewBackgroundTabAnimationData(
            View animationHostView, ToolbarManager toolbarManager) {
        super(animationHostView, toolbarManager);
        mBraveAnimationHostView = animationHostView;
    }

    @Override
    /* package */ Rect getTabSwitcherButtonRect() {
        Rect rect = super.getTabSwitcherButtonRect();
        if (!rect.isEmpty() || !BottomToolbarConfiguration.isBraveBottomControlsEnabled()) {
            return rect;
        }
        // Top toolbar's tab switcher button is GONE (Brave's bottom toolbar is active).
        // Find the bottom toolbar's tab switcher button and return its screen rect so that
        // the background-tab animation is sized and positioned correctly.
        View bottomToolbar =
                mBraveAnimationHostView.getRootView().findViewById(R.id.bottom_toolbar);
        if (bottomToolbar != null) {
            View bottomButton = bottomToolbar.findViewById(R.id.bottom_tab_switcher_button);
            if (bottomButton != null) {
                Rect braveRect = new Rect();
                if (bottomButton.getGlobalVisibleRect(braveRect)) {
                    return braveRect;
                }
            }
        }
        return rect;
    }

    @Override
    @ColorInt
    /* package */ int getPrimaryColor() {
        if (shouldUseBottomToolbarColor()) {
            // The animated tab-switcher button sits over Brave's bottom toolbar, which is not
            // website-colored. Fill it with the bottom toolbar's own (default theme) background so
            // it covers the real button underneath and blends with the bar - rather than the
            // website color.
            return ChromeColors.getDefaultThemeColor(
                    mBraveAnimationHostView.getContext(), /* isIncognito= */ false);
        }
        return super.getPrimaryColor();
    }

    // Base has no incognito getter, but its computed branded scheme encodes it.
    private boolean isIncognitoAnimation() {
        return super.getBrandedColorScheme() == BrandedColorScheme.INCOGNITO;
    }

    @Override
    @BrandedColorScheme
    /* package */ int getBrandedColorScheme() {
        if (shouldUseBottomToolbarColor()) {
            // The animated button belongs to Brave's bottom toolbar, which uses the default app
            // theme (not the website color), so style the tab-count icon for the default scheme.
            return BrandedColorScheme.APP_DEFAULT;
        }
        return super.getBrandedColorScheme();
    }

    @Override
    /* package */ ColorStateList getIconTint() {
        if (shouldUseBottomToolbarColor()) {
            return ThemeUtils.getThemedToolbarIconTint(
                    mBraveAnimationHostView.getContext(), BrandedColorScheme.APP_DEFAULT);
        }
        return super.getIconTint();
    }

    // In incognito the toolbar is never website-colored
    private boolean shouldUseBottomToolbarColor() {
        return BottomToolbarConfiguration.isBraveBottomControlsEnabled()
                && BottomToolbarConfiguration.isToolbarTopAnchored()
                && !isIncognitoAnimation();
    }
}
