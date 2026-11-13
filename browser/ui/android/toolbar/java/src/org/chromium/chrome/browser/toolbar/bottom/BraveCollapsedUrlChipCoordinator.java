/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.app.Activity;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Handler;
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
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.cc.input.BrowserControlsState;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.R;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.url.GURL;

import java.util.function.Supplier;

/**
 * Hosts the collapsed URL chip overlay shown while the Brave bottom toolbar is scrolled away.
 *
 * <p>The chip is an overlay on the activity content: as the bottom toolbar (with the URL bar /
 * search pill) slides off screen, a small translucent pill fades in at the bottom of the screen
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
    private final Callback<Tab> mTabObserverCallback = this::onTabChanged;
    private final BrowserStateBrowserControlsVisibilityDelegate mVisibilityDelegate;

    /**
     * Not final: re-read before each consumer decision, nav mode can change while the process is
     * alive.
     */
    private boolean mHasGestureNavigation;

    private @Nullable WebContentsObserver mWebContentsObserver;

    /** The current page background color; drives the chip's inverted styling. */
    private int mLastContentColor = Color.TRANSPARENT;

    private boolean mChipShowing;
    private boolean mPillHidden;

    /** Whether the chip is currently painted with the dark-on-light-content palette. */
    private boolean mAppliedDarkChip = true;

    /** Cadence of content-color samples while the chip is showing, in ms. */
    private static final long CONTENT_SAMPLE_INTERVAL_MS = 150;

    /** Height of the content strip sampled below the chip, in px. */
    private static final int CONTENT_SAMPLE_HEIGHT_PX = 40;

    /** Gap between the chip's bottom edge and the sampled strip, in px. */
    private static final int CONTENT_SAMPLE_GAP_PX = 8;

    private final Handler mSamplerHandler = new Handler(Looper.getMainLooper());
    private final Runnable mSampleRunnable = this::sampleContentUnderChip;
    private @Nullable Bitmap mSampleBitmap;
    private boolean mSampling;

    public BraveCollapsedUrlChipCoordinator(
            Activity activity,
            BrowserControlsSizer sizer,
            NullableObservableSupplier<Tab> tabSupplier,
            Supplier<Boolean> enabledSupplier,
            Supplier<Boolean> fullscreenSupplier,
            BrowserStateBrowserControlsVisibilityDelegate visibilityDelegate) {
        mActivity = activity;
        mTabSupplier = tabSupplier;
        mVisibilityDelegate = visibilityDelegate;
        mHasGestureNavigation = isGestureNavigation(activity);

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
                        if (visible) {
                            if (mChipView.getVisibility() != View.VISIBLE) {
                                applyNavigationBarInset();
                                applyChipColors();
                                mChipView.setAlpha(0f);
                                mChipView.setVisibility(View.VISIBLE);
                                mChipView.animate().alpha(1f).setDuration(150).start();
                            }
                            startContentSampling();
                        } else {
                            stopContentSampling();
                            mChipView.animate().cancel();
                            mChipView.setVisibility(View.GONE);
                        }
                        mChipShowing = visible;
                        setGesturePillHidden(visible);
                    }

                    @Override
                    public void setChipAlpha(float alpha) {
                        mChipView.setAlpha(alpha);
                    }
                };

        mChipView.setOnClickListener(v -> mVisibilityDelegate.showControlsTransient());

        parent.addView(mChipView);

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
        mTabSupplier.removeObserver(mTabObserverCallback);
        observeTab(null);
        setGesturePillHidden(false);
        mMediator.destroy();
        ViewGroup parent = (ViewGroup) mChipView.getParent();
        if (parent != null) parent.removeView(mChipView);
    }

    private void onTabChanged(@Nullable Tab tab) {
        observeTab(tab);
        mMediator.onUrlSpecChanged();
    }

    private void observeTab(@Nullable Tab tab) {
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
                    }

                    @Override
                    public void didFinishNavigationInPrimaryMainFrame(
                            NavigationHandle navigationHandle) {
                        onUrlSpecChanged();
                    }

                    @Override
                    public void onBackgroundColorChanged() {
                        int color = webContents.getBackgroundColor();
                        if (color == mLastContentColor) return;
                        mLastContentColor = color;
                        applyChipColors();
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
        return UrlFormatter.formatUrlForDisplayOmitSchemePathAndTrivialSubdomains(url);
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

    /** Whether the device is in full gesture navigation (i.e. shows the system bottom pill). */
    private static boolean isGestureNavigation(Activity activity) {
        return BraveCollapsedUrlChipMediator.isGestureNavigation(activity.getContentResolver());
    }

    /**
     * Lifts the chip above an opaque 3-button nav bar (inset + minimum gap); no-op with gesture
     * navigation. Re-reads nav mode so mid-session switches are handled.
     */
    private void applyNavigationBarInset() {
        mHasGestureNavigation = isGestureNavigation(mActivity);
        if (mHasGestureNavigation) return;
        WindowInsets rootInsets = mChipView.getRootWindowInsets();
        if (rootInsets == null) return;
        int bottomInset =
                WindowInsetsCompat.toWindowInsetsCompat(rootInsets)
                        .getInsets(WindowInsetsCompat.Type.navigationBars())
                        .bottom;
        int targetBottomMargin =
                BraveCollapsedUrlChipMediator.navBarBottomMarginPx(
                        bottomInset, mChipView.getResources().getDisplayMetrics().density);
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
     * Hides the system gesture pill while the chip shows (only one bottom indicator on screen) and
     * restores it after; no-op on 3-button nav.
     */
    private void setGesturePillHidden(boolean hidden) {
        mHasGestureNavigation = isGestureNavigation(mActivity);
        if (!mHasGestureNavigation || mPillHidden == hidden) return;
        Window window = mActivity.getWindow();
        WindowInsetsControllerCompat controller =
                WindowCompat.getInsetsController(window, window.getDecorView());
        if (hidden) {
            controller.setSystemBarsBehavior(
                    WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            controller.hide(WindowInsetsCompat.Type.navigationBars());
        } else {
            controller.setSystemBarsBehavior(WindowInsetsControllerCompat.BEHAVIOR_DEFAULT);
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
    private void startContentSampling() {
        mHasGestureNavigation = isGestureNavigation(mActivity);
        if (!mHasGestureNavigation || mSampling) return;
        mSampling = true;
        mSamplerHandler.post(mSampleRunnable);
    }

    private void stopContentSampling() {
        if (!mSampling) return;
        mSampling = false;
        mSamplerHandler.removeCallbacks(mSampleRunnable);
    }

    private void sampleContentUnderChip() {
        if (!mSampling || !mChipShowing) {
            mSampling = false;
            return;
        }
        Window window = mActivity.getWindow();
        View decor = window.getDecorView();
        int[] loc = new int[2];
        mChipView.getLocationInWindow(loc);
        int chipLeft = loc[0];
        int chipBottom = loc[1] + mChipView.getHeight();
        int width = mChipView.getWidth();
        int top = Math.min(chipBottom + CONTENT_SAMPLE_GAP_PX, decor.getHeight() - 1);
        int bottom = Math.min(top + CONTENT_SAMPLE_HEIGHT_PX, decor.getHeight());
        Rect strip = new Rect(chipLeft, top, chipLeft + width, bottom);
        if (strip.isEmpty()) {
            rescheduleSampling();
            return;
        }
        if (mSampleBitmap == null
                || mSampleBitmap.getWidth() != strip.width()
                || mSampleBitmap.getHeight() != strip.height()) {
            mSampleBitmap =
                    Bitmap.createBitmap(strip.width(), strip.height(), Bitmap.Config.ARGB_8888);
        }
        try {
            PixelCopy.request(
                    window,
                    strip,
                    mSampleBitmap,
                    copyResult -> {
                        if (mSampling && copyResult == PixelCopy.SUCCESS) {
                            applySampledContentColor(mSampleBitmap);
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
        long r = 0;
        long g = 0;
        long b = 0;
        int counted = 0;
        for (int y = 0; y < bitmap.getHeight(); y++) {
            for (int x = 0; x < bitmap.getWidth(); x++) {
                int pixel = bitmap.getPixel(x, y);
                if (Color.alpha(pixel) < 1) continue;
                r += Color.red(pixel);
                g += Color.green(pixel);
                b += Color.blue(pixel);
                counted++;
            }
        }
        if (counted == 0) return;
        int avg = Color.rgb((int) (r / counted), (int) (g / counted), (int) (b / counted));
        boolean darkChip = BraveCollapsedUrlChipMediator.isLightContentBackground(avg);
        if (darkChip != mAppliedDarkChip) applyChipColorsForSample(darkChip);
    }

    private void rescheduleSampling() {
        if (mSampling) mSamplerHandler.postDelayed(mSampleRunnable, CONTENT_SAMPLE_INTERVAL_MS);
    }
}
