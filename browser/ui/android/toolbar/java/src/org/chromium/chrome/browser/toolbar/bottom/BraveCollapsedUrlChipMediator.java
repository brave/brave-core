/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.content.ContentResolver;
import android.graphics.Color;
import android.provider.Settings;
import android.view.View;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.url.GURL;

import java.util.Objects;
import java.util.function.Function;
import java.util.function.Supplier;

/**
 * Drives the collapsed URL chip shown while the Brave bottom toolbar is scrolled away.
 *
 * <p>Chromium's browser controls hide the toolbar (and with it the URL) completely while the user
 * scrolls down. This mediator shows a small "collapsed chip" whenever the toolbar is not on screen,
 * so that the page's host (minus trivial subdomains, e.g. "search.brave.com") remains visible at
 * all times, matching Safari / Brave iOS behavior. The chip is display-only: tapping it re-shows
 * the browser controls, so the chip never becomes a navigation or spoofing surface.
 */
@NullMarked
class BraveCollapsedUrlChipMediator implements BrowserControlsStateProvider.Observer {

    /** Interface through which the mediator updates the chip UI. Implemented by the view host. */
    interface Delegate {
        void setChipText(@Nullable String text);

        void setChipVisible(boolean visible);

        void setChipAlpha(float alpha);
    }

    private final BrowserControlsSizer mSizer;
    private final Supplier<String> mUrlSpecSupplier;
    private final Supplier<Boolean> mEnabledSupplier;
    private final Supplier<Boolean> mFullscreenSupplier;
    private final Supplier<Boolean> mCanShowControlsSupplier;
    private final Function<String, @Nullable String> mTextFormatter;
    private final Delegate mDelegate;

    private @Nullable String mLastUrlSpec;
    private @Nullable String mLastChipText;
    private boolean mChipTextInitialized;
    private boolean mAndroidControlsVisible = true;
    private boolean mDestroyed;

    BraveCollapsedUrlChipMediator(
            BrowserControlsSizer sizer,
            Supplier<String> urlSpecSupplier,
            Supplier<Boolean> enabledSupplier,
            Supplier<Boolean> fullscreenSupplier,
            Supplier<Boolean> canShowControlsSupplier,
            Function<String, @Nullable String> textFormatter,
            Delegate delegate) {
        mSizer = sizer;
        mUrlSpecSupplier = urlSpecSupplier;
        mEnabledSupplier = enabledSupplier;
        mFullscreenSupplier = fullscreenSupplier;
        mCanShowControlsSupplier = canShowControlsSupplier;
        mTextFormatter = textFormatter;
        mDelegate = delegate;
        mSizer.addObserver(this);
        updateChip();
    }

    void destroy() {
        mDestroyed = true;
        mSizer.removeObserver(this);
        mDelegate.setChipVisible(false);
        mDelegate.setChipAlpha(0f);
        mDelegate.setChipText(null);
    }

    void onUrlSpecChanged() {
        updateChip();
    }

    @Override
    public void onControlsOffsetChanged(
            int topOffset,
            int topControlsMinHeightOffset,
            boolean topControlsMinHeightChanged,
            int bottomOffset,
            int bottomControlsMinHeightOffset,
            boolean bottomControlsMinHeightChanged,
            boolean requestNewFrame,
            boolean isVisibilityForced) {
        updateChip();
    }

    @Override
    public void onAndroidControlsVisibilityChanged(int visibility) {
        // Authoritative "toolbar collapsed" signal: with Browser Controls In View the
        // per-frame scroll offsets stay in the compositor and onControlsOffsetChanged does
        // not fire for renderer-driven scrolls (docs/ui/android/browser_controls.md).
        boolean visible = visibility == View.VISIBLE;
        if (visible != mAndroidControlsVisible) {
            mAndroidControlsVisible = visible;
            updateChip();
        }
    }

    private void updateChip() {
        if (mDestroyed) return;

        String spec = mUrlSpecSupplier.get();
        // Also force the first update so the delegate never inherits stale
        // text from a previous mediator instance.
        if (!Objects.equals(spec, mLastUrlSpec) || !mChipTextInitialized) {
            mLastUrlSpec = spec;
            mChipTextInitialized = true;
            mLastChipText = isEligibleUrlSpec(spec) ? mTextFormatter.apply(spec) : null;
            mDelegate.setChipText(mLastChipText);
        }
        updateChipDisplay();
    }

    private void updateChipDisplay() {
        boolean featureEnabled = Boolean.TRUE.equals(mEnabledSupplier.get());
        boolean fullscreen = Boolean.TRUE.equals(mFullscreenSupplier.get());
        boolean canShowControls = Boolean.TRUE.equals(mCanShowControlsSupplier.get());
        boolean visible =
                featureEnabled
                        && !fullscreen
                        && canShowControls
                        && !mAndroidControlsVisible
                        && mLastChipText != null;
        mDelegate.setChipVisible(visible);
        mDelegate.setChipAlpha(visible ? 1f : 0f);
    }

    /**
     * Chip-eligible = http/https with a non-empty host; everything else carries no spoofable web
     * identity. Parses the canonical GURL spec with GURL, whose getHost() is the real host
     * (userinfo stripped), so "http://example.com@evil.com/" resolves to evil.com.
     */
    static boolean isEligibleUrlSpec(@Nullable String spec) {
        if (spec == null || spec.isEmpty()) return false;
        GURL url = new GURL(spec);
        if (GURL.isEmptyOrInvalid(url)) return false;
        String scheme = url.getScheme();
        if (!"http".equals(scheme) && !"https".equals(scheme)) return false;
        String host = url.getHost();
        return host != null && !host.isEmpty();
    }

    /**
     * True when the background behind the chip is light: dark pill on light content, light pill on
     * dark content, like the system gesture bar. Translucent colors compose over white, so fully
     * transparent reads as light.
     */
    static boolean isLightContentBackground(int color) {
        float alpha = Color.alpha(color) / 255f;
        float r = Color.red(color) * alpha + 255f * (1f - alpha);
        float g = Color.green(color) * alpha + 255f * (1f - alpha);
        float b = Color.blue(color) * alpha + 255f * (1f - alpha);
        float luminance = (0.2126f * r + 0.7152f * g + 0.0722f * b) / 255f;
        return luminance > 0.6f;
    }

    /** Minimum vertical gap, in dp, kept between the chip and an opaque 3-button navigation bar. */
    public static final int MIN_NAV_BAR_GAP_DP = 8;

    public static final String NAVIGATION_MODE_KEY = "navigation_mode";

    public static final int NAVIGATION_MODE_GESTURE = 2;

    /** Fresh read on every call: nav mode can change while the process is alive. */
    public static boolean isGestureNavigation(ContentResolver resolver) {
        try {
            return Settings.Secure.getInt(resolver, NAVIGATION_MODE_KEY) == NAVIGATION_MODE_GESTURE;
        } catch (Settings.SettingNotFoundException e) {
            return false;
        }
    }

    /** Nav bar inset plus the minimum gap, in px, to lift the chip off the 3-button bar. */
    public static int navBarBottomMarginPx(int bottomInsetPx, float density) {
        return bottomInsetPx + (int) (MIN_NAV_BAR_GAP_DP * density + 0.5f);
    }
}
