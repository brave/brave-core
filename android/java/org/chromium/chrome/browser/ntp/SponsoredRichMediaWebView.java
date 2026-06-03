/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.app.Activity;
import android.graphics.Color;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.chromium.base.Log;
import org.chromium.base.version_info.VersionInfo;
import org.chromium.chrome.browser.content.WebContentsFactory;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.embedder_support.view.ContentView;
import org.chromium.components.thinwebview.ThinWebView;
import org.chromium.components.thinwebview.ThinWebViewAttachParams;
import org.chromium.components.thinwebview.ThinWebViewConstraints;
import org.chromium.components.thinwebview.ThinWebViewFactory;
import org.chromium.content_public.browser.GlobalRenderFrameHostId;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.content_public.browser.Page;
import org.chromium.content_public.browser.Visibility;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.net.NetId;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.url.GURL;

import java.util.Objects;

public class SponsoredRichMediaWebView {
    private static final String TAG = "SponsoredRichMedia";
    private static final String NEW_TAB_TAKEOVER_URL = "chrome://new-tab-takeover";

    private static final int FIRST_PAINT_TIMEOUT_MS = 5_000;
    private static final int POST_LOAD_GRACE_PERIOD_MS = 1_000;

    private final WebContents mWebContents;
    private final ThinWebView mWebView;
    private final WebContentsObserver mObserver;
    private final Runnable mOnFailure;
    private final Handler mHandler = new Handler(Looper.getMainLooper());
    private final Runnable mFirstPaintTimeout =
            () -> {
                Log.w(TAG, "First paint timeout elapsed after %dms.", FIRST_PAINT_TIMEOUT_MS);
                notifyFailure();
            };
    private final Runnable mAcceptAsRendered =
            () -> {
                Log.i(TAG, "Assuming rendered after didStopLoading grace; FNVEP not observed.");
                mHandler.removeCallbacks(mFirstPaintTimeout);
            };
    private boolean mDocumentLoaded;
    private boolean mIframeStarted;
    private String mPlacementId;
    private String mCreativeInstanceId;

    public SponsoredRichMediaWebView(
            Activity activity, WindowAndroid windowAndroid, Profile profile, Runnable onFailure) {
        mOnFailure = onFailure;

        Log.i(TAG, "Creating WebContents with warm renderer.");
        mWebContents =
                WebContentsFactory.createWebContentsWithWarmRenderer(
                        profile,
                        /* initiallyHidden= */ false,
                        /* usesPlatformAutofill= */ true,
                        /* targetNetwork= */ NetId.INVALID);
        Log.i(TAG, "WebContents created.");

        final ContentView webContentView = ContentView.createContentView(activity, mWebContents);
        mWebContents.setDelegates(
                VersionInfo.getProductVersion(),
                ViewAndroidDelegate.createBasicDelegate(webContentView),
                webContentView,
                windowAndroid,
                WebContents.createDefaultInternalsHolder());
        Log.i(TAG, "WebContents delegates set.");

        Log.i(TAG, "Creating ThinWebView.");
        final ThinWebViewConstraints constraints = new ThinWebViewConstraints();
        constraints.backgroundColor = Color.BLACK;
        mWebView =
                ThinWebViewFactory.create(
                        activity,
                        constraints,
                        windowAndroid.getIntentRequestTracker(),
                        /* enablePermissionRequests= */ false);
        mWebView.getView()
                .setLayoutParams(
                        new FrameLayout.LayoutParams(
                                ViewGroup.LayoutParams.MATCH_PARENT,
                                ViewGroup.LayoutParams.MATCH_PARENT));
        mWebView.attachWebContents(
                mWebContents, webContentView, new ThinWebViewAttachParams.Builder().build());
        Log.i(TAG, "ThinWebView created and WebContents attached.");

        mWebView.getView()
                .addOnAttachStateChangeListener(
                        new View.OnAttachStateChangeListener() {
                            @Override
                            public void onViewAttachedToWindow(View v) {
                                Log.i(
                                        TAG,
                                        "View attached to window," + " width=%d, height=%d.",
                                        v.getWidth(),
                                        v.getHeight());
                            }

                            @Override
                            public void onViewDetachedFromWindow(View v) {
                                Log.i(TAG, "View detached from window.");
                            }
                        });

        mWebView.getView()
                .addOnLayoutChangeListener(
                        (v, l, t, r, b, ol, ot, or_, ob) -> {
                            final int w = r - l;
                            final int h = b - t;
                            if (w != (or_ - ol) || h != (ob - ot)) {
                                Log.i(TAG, "View layout changed, width=%d, height=%d.", w, h);
                            }
                        });

        mObserver =
                new WebContentsObserver(mWebContents) {
                    @Override
                    public void didStartLoading(GURL url) {
                        Log.i(TAG, "didStartLoading url=%s.", url);
                        if (mDocumentLoaded && !mIframeStarted) {
                            Log.i(TAG, "Iframe navigation detected.");
                            mIframeStarted = true;
                        }
                    }

                    @Override
                    public void didFinishNavigationInPrimaryMainFrame(
                            NavigationHandle navigationHandle) {
                        Log.i(
                                TAG,
                                "didFinishNavigationInPrimaryMainFrame"
                                        + " isErrorPage=%b, hasCommitted=%b.",
                                navigationHandle.isErrorPage(),
                                navigationHandle.hasCommitted());
                    }

                    @Override
                    public void documentLoadedInPrimaryMainFrame(
                            Page page, GlobalRenderFrameHostId rfhId, int rfhLifecycleState) {
                        Log.i(TAG, "documentLoadedInPrimaryMainFrame.");
                        mDocumentLoaded = true;
                    }

                    @Override
                    public void primaryMainFrameRenderProcessGone(int terminationStatus) {
                        Log.w(
                                TAG,
                                "Renderer process gone, terminationStatus=%d.",
                                terminationStatus);
                        notifyFailure();
                    }

                    @Override
                    public void didFailLoad(
                            boolean isInPrimaryMainFrame,
                            int errorCode,
                            GURL failingUrl,
                            int rfhLifecycleState) {
                        if (isInPrimaryMainFrame) {
                            Log.w(
                                    TAG,
                                    "Navigation failed in primary main frame,"
                                            + " errorCode=%d, url=%s.",
                                    errorCode,
                                    failingUrl);
                            notifyFailure();
                        } else {
                            Log.w(
                                    TAG,
                                    "Navigation failed in subframe," + " errorCode=%d, url=%s.",
                                    errorCode,
                                    failingUrl);
                        }
                    }

                    @Override
                    public void didFirstVisuallyNonEmptyPaint() {
                        Log.i(TAG, "First visually non-empty paint received.");
                        mHandler.removeCallbacks(mFirstPaintTimeout);
                        mHandler.removeCallbacks(mAcceptAsRendered);
                    }

                    @Override
                    public void didStopLoading(GURL url, boolean isKnownValid) {
                        Log.i(TAG, "didStopLoading url=%s.", url);
                        if (mIframeStarted) {
                            // All content loaded; arm fallback in case FNVEP never fires.
                            // Cancel the absolute timeout first so the grace period always
                            // wins regardless of how long loading took.
                            mHandler.removeCallbacks(mFirstPaintTimeout);
                            mHandler.removeCallbacks(mAcceptAsRendered);
                            mHandler.postDelayed(mAcceptAsRendered, POST_LOAD_GRACE_PERIOD_MS);
                        }
                    }

                    @Override
                    public void loadProgressChanged(float progress) {
                        Log.i(TAG, "loadProgressChanged progress=%.2f.", progress);
                    }

                    @Override
                    public void titleWasSet(String title) {
                        Log.i(TAG, "titleWasSet title=%s.", title);
                    }

                    @Override
                    public void onVisibilityChanged(@Visibility int visibility) {
                        Log.i(TAG, "WebContents onVisibilityChanged visibility=%d.", visibility);
                    }
                };
        Log.i(TAG, "WebContentsObserver registered.");
    }

    public void maybeLoadSponsoredRichMedia(String placementId, String creativeInstanceId) {
        if (Objects.equals(mPlacementId, placementId)
                && Objects.equals(mCreativeInstanceId, creativeInstanceId)) {
            Log.i(TAG, "Rich media already loaded for placementId=%s, skipping.", placementId);
            return;
        }

        mPlacementId = placementId;
        mCreativeInstanceId = creativeInstanceId;

        mHandler.removeCallbacks(mFirstPaintTimeout);
        mHandler.removeCallbacks(mAcceptAsRendered);
        mDocumentLoaded = false;
        mIframeStarted = false;
        mHandler.postDelayed(mFirstPaintTimeout, FIRST_PAINT_TIMEOUT_MS);

        String url = getNewTabTakeoverUrl(placementId, creativeInstanceId);
        Log.i(TAG, "Loading url=%s, first paint timeout=%dms.", url, FIRST_PAINT_TIMEOUT_MS);
        mWebContents.getNavigationController().loadUrl(new LoadUrlParams(url));
    }

    public void destroy() {
        Log.i(TAG, "Destroying.");
        mHandler.removeCallbacks(mFirstPaintTimeout);
        mHandler.removeCallbacks(mAcceptAsRendered);
        mObserver.observe(null);
        mWebView.destroy();
        mWebContents.destroy();
    }

    public View getView() {
        return mWebView.getView();
    }

    private void notifyFailure() {
        Log.w(
                TAG,
                "Notifying failure, view width=%d, height=%d, attached=%b.",
                mWebView.getView().getWidth(),
                mWebView.getView().getHeight(),
                mWebView.getView().isAttachedToWindow());
        mHandler.removeCallbacks(mFirstPaintTimeout);
        mHandler.removeCallbacks(mAcceptAsRendered);
        mOnFailure.run();
    }

    private String getNewTabTakeoverUrl(String placementId, String creativeInstanceId) {
        Uri.Builder builder = Uri.parse(NEW_TAB_TAKEOVER_URL).buildUpon();
        builder.appendQueryParameter("placementId", placementId);
        builder.appendQueryParameter("creativeInstanceId", creativeInstanceId);
        return builder.build().toString();
    }
}
