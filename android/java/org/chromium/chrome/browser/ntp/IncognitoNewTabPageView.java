/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.view.View;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.FrameLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.InternetConnection;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.components.content_settings.CookieControlsEnforcement;
import org.chromium.ui.base.ViewUtils;
import org.chromium.ui.widget.Toast;

/** The New Tab Page to use in the incognito profile. */
public class IncognitoNewTabPageView extends FrameLayout {
    private IncognitoNewTabPageManager mManager;
    private boolean mFirstShow = true;
    private NewTabPageScrollView mScrollView;

    private int mSnapshotWidth;
    private int mSnapshotHeight;
    private int mSnapshotScrollY;
    private TextView mVpnCta;

    /** Manages the view interaction with the rest of the system. */
    interface IncognitoNewTabPageManager {
        /** Loads a page explaining details about incognito mode in the current tab. */
        void loadIncognitoLearnMore();

        /**
         * Initializes the cookie controls manager for interaction with the cookie controls toggle.
         */
        void initCookieControlsManager();

        /** Tells the caller whether a new snapshot is required or not. */
        boolean shouldCaptureThumbnail();

        /** Whether to show the tracking protection UI on the NTP. */
        boolean shouldShowTrackingProtectionNtp();

        /** Cleans up the manager after it is finished being used. */
        void destroy();

        /**
         * Called when the NTP has completely finished loading (all views will be inflated
         * and any dependent resources will have been loaded).
         */
        void onLoadingComplete();
    }

    /**
     * Default constructor needed to inflate via XML.
     *
     * @noinspection ConstantValue
     */
    public IncognitoNewTabPageView(Context context, AttributeSet attrs) {
        super(context, attrs);

        // These checks are just making sure that these values are still used in Chromium to avoid
        // lint issues.
        assert R.string.accessibility_new_incognito_tab_page > 0
                : "Something has changed in the upstream!";

        assert R.drawable.incognito_splash > 0 : "Something has changed in the upstream!";

        // Added this check just to avoid resource is not used warning. We use own own private tab
        // UI.
        assert R.dimen.incognito_ntp_fading_shadow_size > 0
                : "Something has changed in the upstream!";
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mScrollView = (NewTabPageScrollView) findViewById(R.id.ntp_scrollview);

        // FOCUS_BEFORE_DESCENDANTS is needed to support keyboard shortcuts. Otherwise, pressing
        // any shortcut causes the UrlBar to be focused. See ViewRootImpl.leaveTouchMode().
        mScrollView.setDescendantFocusability(FOCUS_BEFORE_DESCENDANTS);

        mVpnCta = findViewById(R.id.tv_try_vpn);
        if (BraveVpnUtils.isVpnFeatureSupported(getContext())
                && !BraveVpnNativeWorker.getInstance().isPurchasedUser()) {
            mVpnCta.setOnClickListener(
                    v -> {
                        if (!InternetConnection.isNetworkAvailable(getContext())) {
                            Toast.makeText(getContext(), R.string.no_internet, Toast.LENGTH_SHORT)
                                    .show();
                        } else {
                            BraveVpnUtils.openBraveVpnPlansActivity(getContext());
                        }
                    });
        } else {
            mVpnCta.setVisibility(View.GONE);
        }
    }

    /**
     * Initialize the incognito New Tab Page.
     *
     * @param manager The manager that handles external dependencies of the view.
     */
    void initialize(IncognitoNewTabPageManager manager) {
        mManager = manager;
    }

    /** @return The IncognitoNewTabPageManager associated with this IncognitoNewTabPageView. */
    protected IncognitoNewTabPageManager getManager() {
        return mManager;
    }

    /**
     * @return The ScrollView of within the page. Used for padding when drawing edge to edge.
     */
    ScrollView getScrollView() {
        return mScrollView;
    }

    /**
     * @see org.chromium.chrome.browser.compositor.layouts.content.
     *     InvalidationAwareThumbnailProvider#shouldCaptureThumbnail()
     */
    boolean shouldCaptureThumbnail() {
        if (getWidth() == 0 || getHeight() == 0) return false;

        return getWidth() != mSnapshotWidth
                || getHeight() != mSnapshotHeight
                || mScrollView.getScrollY() != mSnapshotScrollY;
    }

    /**
     * @see org.chromium.chrome.browser.compositor.layouts.content.
     *         InvalidationAwareThumbnailProvider#captureThumbnail(Canvas)
     */
    void captureThumbnail(Canvas canvas) {
        ViewUtils.captureBitmap(this, canvas);
        mSnapshotWidth = getWidth();
        mSnapshotHeight = getHeight();
        mSnapshotScrollY = mScrollView.getScrollY();
    }

    /**
     * Set the visibility of the cookie controls card on the incognito description.
     * @param isVisible Whether it's visible or not.
     */
    void setIncognitoCookieControlsCardVisibility(boolean isVisible) {}

    /**
     * Set the toggle on the cookie controls card.
     * @param isChecked Whether it's checked or not.
     */
    void setIncognitoCookieControlsToggleChecked(boolean isChecked) {}

    /**
     * Set the incognito cookie controls toggle checked change listener.
     * @param listener The given checked change listener.
     */
    void setIncognitoCookieControlsToggleCheckedListener(OnCheckedChangeListener listener) {}

    /**
     * Set the enforcement rule for the incognito cookie controls toggle.
     * @param enforcement The enforcement enum to set.
     */
    void setIncognitoCookieControlsToggleEnforcement(@CookieControlsEnforcement int enforcement) {}

    /**
     * Set the incognito cookie controls icon click listener.
     * @param listener The given onclick listener.
     */
    void setIncognitoCookieControlsIconOnclickListener(OnClickListener listener) {}

    void setIncognitoNewTabHeader(String newTabPageHeader) {}

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        assert mManager != null;
        if (mFirstShow) {
            mManager.onLoadingComplete();
            mFirstShow = false;
        }
    }
}
