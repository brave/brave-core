/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.app.Activity;
import android.content.ComponentCallbacks;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Handler;
import android.text.BidiFormatter;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.PixelCopy;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.widget.FrameLayout;
import android.widget.TextView;

import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import org.chromium.base.Callback;
import org.chromium.base.supplier.NullableObservableSupplier;
import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.cc.input.BrowserControlsState;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.toolbar.R;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.ui.UiUtils;
import org.chromium.url.GURL;

import java.util.function.Supplier;

/**
 * Hosts the collapsed URL chip overlay shown while the browser controls are scrolled away.
 *
 * <p>The chip is an overlay on the activity content: as the browser controls (the toolbar with
 * the URL bar) slide off screen, a small translucent pill fades in at the bottom of the screen
 * showing the page's host minus trivial subdomains. Tapping it re-shows the browser controls. The
 * chip is never a navigation surface, so it cannot be used to spoof or open URLs.
 */
@NullMarked
public class BraveCollapsedUrlChipCoordinator {

    private final BraveCollapsedUrlChipMediator mMediator;
    private final Activity mActivity;
    private final View mChipView;
    private final TextView mChipTextView;
    private final NullableObservableSupplier<Tab> mTabSupplier;
    private final NullableObservableSupplier<Boolean> mFullscreenSupplier;
    private final Callback<@Nullable Tab> mTabObserverCallback = this::onTabChanged;
    private final Callback<@Nullable Boolean> mFullscreenObserverCallback =
            this::onFullscreenChanged;
    private final BrowserStateBrowserControlsVisibilityDelegate mVisibilityDelegate;

    private @Nullable WebContentsObserver mWebContentsObserver;
    private @Nullable Tab mObservedTab;

    /** Re-observes the tab when its content changes (e.g. WebContents swaps). */
    private final TabObserver mTabContentObserver =
            new TabObserver() {
                @Override
                public void onContentChanged(Tab tab) {
                    observeTab(tab);
                    mMediator.onUrlSpecChanged();
                }
            };

    /** The current page background color; drives the chip's inverted styling. */
    private int mLastContentColor = Color.TRANSPARENT;

    private boolean mChipShowing;
    private boolean mPillHidden;

    /** Whether the chip is currently painted with the dark-on-light-content palette. */
    private boolean mAppliedDarkChip = true;

    /** Cadence of content-color samples while the chip is showing and content is changing. */
    private static final long CONTENT_SAMPLE_INTERVAL_MS = 150;

    /** Consecutive unchanged samples before the sampling cadence backs off (idle pages). */
    private static final int CONTENT_SAMPLE_STABLE_TICKS = 3;

    /** Backed-off cadence once the sampled palette stops changing. */
    private static final long CONTENT_SAMPLE_IDLE_INTERVAL_MS = 500;

    /** Maximum sampling cadence for long-idle content. */
    private static final long CONTENT_SAMPLE_IDLE_MAX_INTERVAL_MS = 1000;

    /** Consecutive unchanged samples after which sampling stops until the content changes. */
    private static final int CONTENT_SAMPLE_SETTLED_TICKS = 30;

    /** Minimum height, in px, of a below-chip strip for sampling to be meaningful. */
    private static final int CONTENT_SAMPLE_MIN_STRIP_PX = 8;

    /** Height of the content strip sampled below the chip, in px. */
    private static final int CONTENT_SAMPLE_HEIGHT_PX = 40;

    /** Gap between the chip's bottom edge and the sampled strip, in px. */
    private static final int CONTENT_SAMPLE_GAP_PX = 8;

    /**
     * Fixed downsampled capture size; PixelCopy scales the strip rect into it, keeping the per-tick
     * cost constant regardless of strip dimensions.
     */
    private static final int SAMPLE_BITMAP_SIZE = 8;

    private final Handler mSamplerHandler = new Handler(Looper.getMainLooper());
    private final Runnable mSampleRunnable = this::sampleContentUnderChip;
    private @MonotonicNonNull Bitmap mSampleBitmap;
    private boolean mSampling;
    private int mStableSampleCount;

    /**
     * Incremented on every start/stop so in-flight {@link android.view.PixelCopy} callbacks from a
     * superseded sampling run are ignored instead of racing a new run on the shared bitmap.
     */
    private int mSampleGeneration;

    /** Re-lays-out the chip when the window bounds change (e.g. entering split view). */
    private final ComponentCallbacks mComponentCallbacks =
            new ComponentCallbacks() {
                @Override
                public void onConfigurationChanged(@Nullable Configuration newConfig) {
                    if (mChipShowing) {
                        applyNavigationBarInset(hasGestureNavigation());
                    }
                }

                @Override
                public void onLowMemory() {}
            };

    public BraveCollapsedUrlChipCoordinator(
            Activity activity,
            BrowserControlsSizer sizer,
            NullableObservableSupplier<Tab> tabSupplier,
            Supplier<Boolean> enabledSupplier,
            NullableObservableSupplier<Boolean> fullscreenSupplier,
            BrowserStateBrowserControlsVisibilityDelegate visibilityDelegate) {
        mActivity = activity;
        mTabSupplier = tabSupplier;
        mFullscreenSupplier = fullscreenSupplier;
        mVisibilityDelegate = visibilityDelegate;

        ViewGroup parent = activity.findViewById(android.R.id.content);
        mChipView =
                LayoutInflater.from(activity)
                        .inflate(R.layout.brave_collapsed_url_chip, parent, false);
        mChipTextView = mChipView.findViewById(R.id.brave_collapsed_url_chip_text);

        BraveCollapsedUrlChipMediator.Delegate delegate =
                new BraveCollapsedUrlChipMediator.Delegate() {
                    @Override
                    public void setChipText(@Nullable String text) {
                        mChipTextView.setText(text == null ? "" : text);
                        mChipTextView.setContentDescription(text);
                    }

                    @Override
                    public void setChipVisible(boolean visible) {
                        // Per-offset-change calls collapse to real transitions only; this keeps
                        // repeated show() calls from restarting sampling or re-reading insets.
                        if (visible == mChipShowing) return;
                        // One gesture-nav read per show transition: the check bottoms out in
                        // an insets query and all consumers use the same value. Reads happen
                        // only here (decor view attached, insets valid); querying during early
                        // init, before attach, crashes on null root insets.
                        boolean hasGestureNavigation = hasGestureNavigation();
                        if (visible) {
                            if (mChipView.getVisibility() != View.VISIBLE) {
                                applyNavigationBarInset(hasGestureNavigation);
                                applyChipColors();
                                mChipView.setAlpha(0f);
                                mChipView.setVisibility(View.VISIBLE);
                                mChipView.animate().alpha(1f).setDuration(150).start();
                            }
                            mStableSampleCount = 0;
                            startContentSampling(hasGestureNavigation);
                        } else {
                            stopContentSampling();
                            mChipView.animate().cancel();
                            mChipView.setVisibility(View.GONE);
                        }
                        mChipShowing = visible;
                        // The pill is only ever hidden while the chip shows, so the restore on
                        // hide is a no-op unless it was actually hidden; skip the insets query
                        // otherwise (it may be invalid during early init).
                        if (visible || mPillHidden) {
                            setGesturePillHidden(visible, hasGestureNavigation);
                        }
                    }

                    @Override
                    public void setChipAlpha(float alpha) {
                        mChipView.setAlpha(alpha);
                    }
                };

        mChipView.setOnClickListener(v -> mVisibilityDelegate.showControlsTransient());

        parent.addView(mChipView);
        mActivity.registerComponentCallbacks(mComponentCallbacks);
        // NONE: no notify-on-add; the mediator's first updateChip() already covers the current
        // state, and the window may not be attached yet (insets queries would crash).
        mFullscreenSupplier.addSyncObserver(mFullscreenObserverCallback);

        mMediator =
                new BraveCollapsedUrlChipMediator(
                        sizer,
                        this::getUrlSpec,
                        enabledSupplier,
                        fullscreenSupplier,
                        this::canShowBrowserControls,
                        this::formatChipText,
                        delegate);

        mTabSupplier.addSyncObserverAndCall(mTabObserverCallback);
    }

    public void onUrlSpecChanged() {
        mMediator.onUrlSpecChanged();
    }

    public void destroy() {
        stopContentSampling();
        mActivity.unregisterComponentCallbacks(mComponentCallbacks);
        mFullscreenSupplier.removeObserver(mFullscreenObserverCallback);
        mTabSupplier.removeObserver(mTabObserverCallback);
        observeTab(null);
        if (mPillHidden) {
            setGesturePillHidden(false, hasGestureNavigation());
        }
        mMediator.destroy();
        ViewGroup parent = (ViewGroup) mChipView.getParent();
        if (parent != null) parent.removeView(mChipView);
    }

    private void onTabChanged(@Nullable Tab tab) {
        observeTab(tab);
        mMediator.onUrlSpecChanged();
    }

    private void onFullscreenChanged(@Nullable Boolean fullscreen) {
        if (fullscreen == null) return;
        mMediator.onFullscreenChanged();
        // The page handed immersive mode back; restore the pill it didn't take (the chip's
        // own hide path defers the restore while the page is in fullscreen).
        if (!fullscreen && mPillHidden && !mChipShowing) {
            setGesturePillHidden(false, hasGestureNavigation());
        }
    }

    private void observeTab(@Nullable Tab tab) {
        if (mObservedTab != tab) {
            // Guarded so a same-tab content change (e.g. a WebContents swap) does not mutate the
            // tab's observer list while it is iterating its observers.
            if (mObservedTab != null) {
                mObservedTab.removeObserver(mTabContentObserver);
                mObservedTab = null;
            }
            if (tab != null) {
                tab.addObserver(mTabContentObserver);
                mObservedTab = tab;
            }
        }
        if (mWebContentsObserver != null) {
            mWebContentsObserver.observe(null);
            mWebContentsObserver = null;
        }
        if (tab == null) {
            mLastContentColor = Color.TRANSPARENT;
            applyChipColors();
            return;
        }
        WebContents webContents = tab.getWebContents();
        if (webContents == null) {
            mLastContentColor = Color.TRANSPARENT;
            applyChipColors();
            return;
        }
        mLastContentColor = webContents.getBackgroundColor();
        applyChipColors();
        mWebContentsObserver =
                new WebContentsObserver(webContents) {
                    @Override
                    public void didStartLoading(GURL url) {
                        onUrlSpecChanged();
                        restartContentSampling();
                    }

                    @Override
                    public void didFinishNavigationInPrimaryMainFrame(
                            NavigationHandle navigationHandle) {
                        onUrlSpecChanged();
                        restartContentSampling();
                    }

                    @Override
                    public void onBackgroundColorChanged() {
                        int color = webContents.getBackgroundColor();
                        if (color == mLastContentColor) return;
                        mLastContentColor = color;
                        applyChipColors();
                        restartContentSampling();
                    }
                };
    }

    private @Nullable String getUrlSpec() {
        GURL url = activeGurl();
        return url == null ? null : url.getSpec();
    }

    /** False while another UI surface owns the controls, e.g. the omnibox is focused. */
    private boolean canShowBrowserControls() {
        return mVisibilityDelegate.get() != BrowserControlsState.HIDDEN;
    }

    /**
     * Mirrors Safari / Brave iOS collapsed URL bar text (URLFormatter.formatURLOrigin with
     * forDisplayOmitSchemePathAndTrivialSubdomains): host minus trivial subdomains, e.g.
     * "search.brave.com".
     */
    private @Nullable String formatChipText(@Nullable String spec) {
        if (spec == null) return null;
        GURL url = gurlForSpec(spec);
        if (url == null) return null;
        String host = UrlFormatter.formatUrlForDisplayOmitSchemePathAndTrivialSubdomains(url);
        // Keep the omnibox's security cue: a bare host alone would present a trusted-looking
        // identity for plaintext HTTP pages. The placeholder keeps word order in RTL locales.
        if ("http".equals(url.getScheme())) {
            return mActivity.getString(R.string.brave_collapsed_url_chip_not_secure, host);
        }
        return BidiFormatter.getInstance().unicodeWrap(host);
    }

    private @Nullable GURL gurlForSpec(@Nullable String spec) {
        GURL url = activeGurl();
        return url != null && url.getSpec().equals(spec) ? url : null;
    }

    /** The validated GURL of the active tab, or null when there is none. */
    private @Nullable GURL activeGurl() {
        Tab tab = mTabSupplier.get();
        GURL url = tab == null ? null : tab.getUrl();
        return url != null && url.isValid() && !url.isEmpty() ? url : null;
    }

    /**
     * Lifts the chip above an opaque 3-button nav bar (inset + minimum gap); no-op with gesture
     * navigation.
     */
    private void applyNavigationBarInset(boolean hasGestureNavigation) {
        if (hasGestureNavigation) return;
        WindowInsets rootInsets = mChipView.getRootWindowInsets();
        if (rootInsets == null) return;
        int bottomInset =
                WindowInsetsCompat.toWindowInsetsCompat(rootInsets)
                        .getInsets(WindowInsetsCompat.Type.navigationBars())
                        .bottom;
        int targetBottomMargin =
                BraveCollapsedUrlChipMediator.navBarBottomMarginPx(
                        bottomInset, mChipView.getResources().getDisplayMetrics());
        ViewGroup.LayoutParams layoutParams = mChipView.getLayoutParams();
        if (layoutParams instanceof FrameLayout.LayoutParams) {
            FrameLayout.LayoutParams frameParams = (FrameLayout.LayoutParams) layoutParams;
            if (frameParams.bottomMargin != targetBottomMargin) {
                frameParams.bottomMargin = targetBottomMargin;
                mChipView.setLayoutParams(frameParams);
            }
        }
    }

    /**
     * Whether the device is in full gesture navigation. Reads happen only when the chip is showing
     * (or was showing), when the activity window is attached and {@link
     * UiUtils#isGestureNavigationMode(Window)}'s insets query is valid; querying during early init
     * crashes on a null root insets. Not memoized, so nav-mode switches are picked up on the next
     * transition.
     */
    private boolean hasGestureNavigation() {
        if (!mChipView.isAttachedToWindow()) return false;
        return UiUtils.isGestureNavigationMode(mActivity.getWindow());
    }

    private int mSavedSystemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_DEFAULT;

    /**
     * Hides the system gesture pill while the chip shows (only one bottom indicator on screen) and
     * restores it after; no-op on 3-button nav. The previous system-bar behavior is saved and
     * restored instead of forcing a default, so behavior set by other components (e.g.
     * FullscreenManager's edge-to-edge handling) survives.
     */
    private void setGesturePillHidden(boolean hidden, boolean hasGestureNavigation) {
        if (!hasGestureNavigation || mPillHidden == hidden) return;
        // While the page owns immersive mode (e.g. fullscreen video), leave the bars alone;
        // onFullscreenChanged restores the pill once the page hands the state back.
        if (!hidden && Boolean.TRUE.equals(mFullscreenSupplier.get())) return;
        Window window = mActivity.getWindow();
        WindowInsetsControllerCompat controller =
                WindowCompat.getInsetsController(window, window.getDecorView());
        if (hidden) {
            mSavedSystemBarsBehavior = controller.getSystemBarsBehavior();
            controller.setSystemBarsBehavior(
                    WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            controller.hide(WindowInsetsCompat.Type.navigationBars());
        } else {
            controller.setSystemBarsBehavior(mSavedSystemBarsBehavior);
            controller.show(WindowInsetsCompat.Type.navigationBars());
        }
        mPillHidden = hidden;
    }

    /** Inverted styling matching the system gesture bar: content-driven, not theme-driven. */
    private void applyChipColors() {
        boolean darkChip =
                BraveCollapsedUrlChipMediator.isLightContentBackground(mLastContentColor);
        applyChipColorsForSample(darkChip);
    }

    private void applyChipColorsForSample(boolean darkChip) {
        mAppliedDarkChip = darkChip;
        int backgroundRes =
                darkChip
                        ? R.color.brave_collapsed_url_chip_background_dark
                        : R.color.brave_collapsed_url_chip_background;
        int textRes =
                darkChip
                        ? R.color.brave_collapsed_url_chip_text_dark
                        : R.color.brave_collapsed_url_chip_text;
        mChipView.setBackgroundTintList(ColorStateList.valueOf(mActivity.getColor(backgroundRes)));
        mChipTextView.setTextColor(mActivity.getColor(textRes));
    }

    /**
     * Samples the rendered content directly below the chip while it shows and flips its inverted
     * styling to keep contrast, mirroring the Android 15 gesture-pill contrast behavior; nothing in
     * Chromium exposes that decision, so it is re-implemented locally.
     */
    private void startContentSampling(boolean hasGestureNavigation) {
        if (!hasGestureNavigation) return;
        mSampling = true;
        mStableSampleCount = 0;
        mSampleGeneration++;
        // Supersede any pending tick from a previous run; the generation counter keeps
        // in-flight PixelCopy callbacks from the old run from racing this one.
        mSamplerHandler.removeCallbacks(mSampleRunnable);
        mSamplerHandler.post(mSampleRunnable);
    }

    private void stopContentSampling() {
        mSampling = false;
        mSampleGeneration++;
        mSamplerHandler.removeCallbacks(mSampleRunnable);
    }

    /** Restarts settled sampling when the content under the chip is known to have changed. */
    private void restartContentSampling() {
        if (mChipShowing) startContentSampling(hasGestureNavigation());
    }

    private void sampleContentUnderChip() {
        if (!mSampling || !mChipShowing) {
            mSampling = false;
            return;
        }
        final int generation = mSampleGeneration;
        Window window = mActivity.getWindow();
        View decor = window.getDecorView();
        int[] loc = new int[2];
        mChipView.getLocationInWindow(loc);
        int chipLeft = loc[0];
        int chipTop = loc[1];
        int chipBottom = chipTop + mChipView.getHeight();
        int width = mChipView.getWidth();
        // Prefer the content strip just below the chip. When the chip sits at the
        // screen's bottom edge the strip collapses to nothing; PixelCopy reads the
        // composed window (chip included), so there is no valid region to sample —
        // skip the tick and keep the current palette.
        int stripTop = Math.min(chipBottom + CONTENT_SAMPLE_GAP_PX, decor.getHeight() - 1);
        int stripBottom = Math.min(stripTop + CONTENT_SAMPLE_HEIGHT_PX, decor.getHeight());
        if (stripBottom - stripTop < CONTENT_SAMPLE_MIN_STRIP_PX) {
            rescheduleSampling();
            return;
        }
        Rect strip = new Rect(chipLeft, stripTop, chipLeft + width, stripBottom);
        if (mSampleBitmap == null) {
            mSampleBitmap =
                    Bitmap.createBitmap(
                            SAMPLE_BITMAP_SIZE, SAMPLE_BITMAP_SIZE, Bitmap.Config.ARGB_8888);
        }
        try {
            PixelCopy.request(
                    window,
                    strip,
                    mSampleBitmap,
                    copyResult -> {
                        // Superseded by a start/stop on another chain; never touch the shared
                        // bitmap or spawn a duplicate chain.
                        if (generation != mSampleGeneration) return;
                        if (mSampling && copyResult == PixelCopy.SUCCESS) {
                            applySampledContentColor(mSampleBitmap);
                            if (mStableSampleCount >= CONTENT_SAMPLE_SETTLED_TICKS) {
                                // Content settled; navigation/color hooks restart sampling.
                                mSampling = false;
                                return;
                            }
                        }
                        rescheduleSampling();
                    },
                    mSamplerHandler);
        } catch (IllegalArgumentException e) {
            // Window may be mid-transition (e.g. rotation); retry on the next tick.
            rescheduleSampling();
        }
    }

    private void applySampledContentColor(Bitmap bitmap) {
        int width = bitmap.getWidth();
        int height = bitmap.getHeight();
        int[] pixels = new int[width * height];
        bitmap.getPixels(pixels, 0, width, 0, 0, width, height);
        long r = 0;
        long g = 0;
        long b = 0;
        int counted = 0;
        for (int pixel : pixels) {
            if (Color.alpha(pixel) < 1) continue;
            r += Color.red(pixel);
            g += Color.green(pixel);
            b += Color.blue(pixel);
            counted++;
        }
        if (counted == 0) return;
        int avg = Color.rgb((int) (r / counted), (int) (g / counted), (int) (b / counted));
        boolean darkChip = BraveCollapsedUrlChipMediator.isLightContentBackground(avg);
        mStableSampleCount = darkChip == mAppliedDarkChip ? mStableSampleCount + 1 : 0;
        if (darkChip != mAppliedDarkChip) applyChipColorsForSample(darkChip);
    }

    /** Faster cadence while the content under the chip is changing; backs off when idle. */
    private long nextSampleIntervalMs() {
        if (mStableSampleCount < CONTENT_SAMPLE_STABLE_TICKS) return CONTENT_SAMPLE_INTERVAL_MS;
        if (mStableSampleCount < 10) return CONTENT_SAMPLE_IDLE_INTERVAL_MS;
        return CONTENT_SAMPLE_IDLE_MAX_INTERVAL_MS;
    }

    private void rescheduleSampling() {
        if (mSampling) mSamplerHandler.postDelayed(mSampleRunnable, nextSampleIntervalMs());
    }
}
