/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.text.style.ForegroundColorSpan;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.widget.ImageView;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.IdRes;
import androidx.annotation.StringRes;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.ui.base.ViewUtils;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.widget.ChromeBulletSpan;

/**
 * The New Tab Page for use in the incognito profile.
 */
public class IncognitoNewTabPageView extends FrameLayout {
    private IncognitoNewTabPageManager mManager;
    private boolean mFirstShow = true;
    private NewTabPageScrollView mScrollView;

    private int mSnapshotWidth;
    private int mSnapshotHeight;
    private int mSnapshotScrollY;

    private int mWidthDp;
    private int mHeightDp;

    private static final int WIDE_LAYOUT_THRESHOLD_DP = 720;

    /**
     * Manages the view interaction with the rest of the system.
     */
    interface IncognitoNewTabPageManager {
        /** Loads a page explaining details about incognito mode in the current tab. */
        void loadIncognitoLearnMore();

        /**
         * Enables/disables cookie controls mode as set from incognito NTP. By default
         * nothing happens.
         * @param enable A boolean specifying the state of third party cookie blocking in
         *         incognito. True will enable third-party cookie blocking in incognito and false
         *         will disable this feature.
         * */
        void setThirdPartyCookieBlocking(boolean enable);

        /**
         * Returns whether third-party cookies are currently being blocked.
         * */
        boolean shouldBlockThirdPartyCookies();

        /**
         * Called when the NTP has completely finished loading (all views will be inflated
         * and any dependent resources will have been loaded).
         */
        void onLoadingComplete();
    }

    /** Default constructor needed to inflate via XML. */
    public IncognitoNewTabPageView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mScrollView = (NewTabPageScrollView) findViewById(R.id.ntp_scrollview);
        mScrollView.setBackgroundColor(
                ApiCompatibilityUtils.getColor(getResources(), R.color.ntp_bg_incognito));
        setContentDescription(
                getResources().getText(R.string.accessibility_new_incognito_tab_page));

        // FOCUS_BEFORE_DESCENDANTS is needed to support keyboard shortcuts. Otherwise, pressing
        // any shortcut causes the UrlBar to be focused. See ViewRootImpl.leaveTouchMode().
        mScrollView.setDescendantFocusability(FOCUS_BEFORE_DESCENDANTS);

        View learnMore = findViewById(R.id.learn_more);
        learnMore.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                mManager.loadIncognitoLearnMore();
            }
        });

        mWidthDp = getContext().getResources().getConfiguration().screenWidthDp;
        mHeightDp = getContext().getResources().getConfiguration().screenHeightDp;

        adjustView();
    }

    /**
     * Initialize the incognito New Tab Page.
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
     * @see org.chromium.chrome.browser.compositor.layouts.content.
     *         InvalidationAwareThumbnailProvider#shouldCaptureThumbnail()
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

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        assert mManager != null;
        if (mFirstShow) {
            mManager.onLoadingComplete();
            mFirstShow = false;
        }
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        // View#onConfigurationChanged() doesn't get called when resizing this view in
        // multi-window mode, so #onMeasure() is used instead.
        Configuration config = getContext().getResources().getConfiguration();
        if (mWidthDp != config.screenWidthDp || mHeightDp != config.screenHeightDp) {
            mWidthDp = config.screenWidthDp;
            mHeightDp = config.screenHeightDp;
            adjustView();
        }

        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    private void adjustView() {
        adjustIcon();
        adjustLearnMore();
    }

    /** Adjust the Incognito icon. */
    private void adjustIcon() {
        // The icon resource is 120dp x 120dp (i.e. 120px x 120px at MDPI). This method always
        // resizes the icon view to 120dp x 120dp or smaller, therefore image quality is not lost.

        int sizeDp;
        if (mWidthDp <= WIDE_LAYOUT_THRESHOLD_DP) {
            sizeDp = (mWidthDp <= 240 || mHeightDp <= 480) ? 48 : 72;
        } else {
            sizeDp = mHeightDp <= 480 ? 72 : 120;
        }

        ImageView icon = (ImageView) findViewById(R.id.new_tab_incognito_icon);
        icon.getLayoutParams().width = dpToPx(getContext(), sizeDp);
        icon.getLayoutParams().height = dpToPx(getContext(), sizeDp);
    }

    /** Adjust the "Learn More" link. */
    private void adjustLearnMore() {
        final String subtitleText =
                getContext().getResources().getString(R.string.new_tab_otr_subtitle);
        boolean learnMoreInSubtitle = mWidthDp > WIDE_LAYOUT_THRESHOLD_DP;

        if (!learnMoreInSubtitle) {
            return;
        }

        // Concatenate the original text with a clickable "Learn more" link.
        StringBuilder concatenatedText = new StringBuilder();
        concatenatedText.append(subtitleText);
        concatenatedText.append(" ");
        concatenatedText.append(getContext().getResources().getString(R.string.learn_more));
        SpannableString textWithLearnMoreLink = new SpannableString(concatenatedText.toString());

        NoUnderlineClickableSpan span = new NoUnderlineClickableSpan(getResources(),
                R.color.modern_blue_300, (view) -> getManager().loadIncognitoLearnMore());
        textWithLearnMoreLink.setSpan(
                span, subtitleText.length() + 1, textWithLearnMoreLink.length(), 0 /* flags */);
    }
}
