/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.app.Activity;
import android.net.Uri;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.chromium.base.version_info.VersionInfo;
import org.chromium.chrome.browser.content.WebContentsFactory;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.embedder_support.view.ContentView;
import org.chromium.components.thinwebview.ThinWebView;
import org.chromium.components.thinwebview.ThinWebViewConstraints;
import org.chromium.components.thinwebview.ThinWebViewFactory;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.net.NetId;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;

import java.util.Objects;

public class SponsoredRichMediaWebView {
    private static final String NEW_TAB_TAKEOVER_URL = "chrome://new-tab-takeover";

    private final WebContents mWebContents;
    private final ThinWebView mWebView;
    private String mPlacementId;
    private String mCreativeInstanceId;

    public SponsoredRichMediaWebView(
            Activity activity, WindowAndroid windowAndroid, Profile profile) {
        mWebContents =
                WebContentsFactory.createWebContentsWithWarmRenderer(
                        profile,
                        /* initiallyHidden= */ false,
                        /* usesPlatformAutofill= */ true,
                        /* targetNetwork= */ NetId.INVALID);

        final ContentView webContentView = ContentView.createContentView(activity, mWebContents);
        mWebContents.setDelegates(
                VersionInfo.getProductVersion(),
                ViewAndroidDelegate.createBasicDelegate(webContentView),
                webContentView,
                windowAndroid,
                WebContents.createDefaultInternalsHolder());

        mWebView =
                ThinWebViewFactory.create(
                        activity,
                        new ThinWebViewConstraints(),
                        windowAndroid.getIntentRequestTracker());
        mWebView.getView()
                .setLayoutParams(
                        new FrameLayout.LayoutParams(
                                ViewGroup.LayoutParams.MATCH_PARENT,
                                ViewGroup.LayoutParams.MATCH_PARENT));
        mWebView.attachWebContents(mWebContents, webContentView, null);
    }

    public void maybeLoadSponsoredRichMedia(String placementId, String creativeInstanceId) {
        if (Objects.equals(mPlacementId, placementId)
                && Objects.equals(mCreativeInstanceId, creativeInstanceId)) {
            return;
        }

        mPlacementId = placementId;
        mCreativeInstanceId = creativeInstanceId;

        mWebContents
                .getNavigationController()
                .loadUrl(new LoadUrlParams(getNewTabTakeoverUrl(placementId, creativeInstanceId)));
    }

    public View getView() {
        return mWebView.getView();
    }

    private String getNewTabTakeoverUrl(String placementId, String creativeInstanceId) {
        Uri.Builder builder = Uri.parse(NEW_TAB_TAKEOVER_URL).buildUpon();
        builder.appendQueryParameter("placementId", placementId);
        builder.appendQueryParameter("creativeInstanceId", creativeInstanceId);
        return builder.build().toString();
    }
}
