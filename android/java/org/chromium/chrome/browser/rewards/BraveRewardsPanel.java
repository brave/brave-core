/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Bundle;
import android.view.ContextThemeWrapper;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.cardview.widget.CardView;

import org.chromium.base.SysUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.util.ConfigurationUtils;

import java.util.Timer;
import java.util.TimerTask;

public class BraveRewardsPanel implements BraveRewardsHelper.LargeIconReadyCallback {
    private static final int UPDATE_BALANCE_INTERVAL = 60000; // In milliseconds

    private final View mAnchorView;
    private final PopupWindow mPopupWindow;
    private View mPopupView;
    private final BraveActivity mBraveActivity;
    private final ChromeTabbedActivity mActivity;
    private BraveRewardsHelper mIconFetcher;

    private Timer mBalanceUpdater;
    private Timer mPublisherFetcher;

    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;

    private int mCurrentTabId;

    public BraveRewardsPanel(View anchorView) {
        // currentNotificationId = "";
        // publisherExist = false;
        // publisherFetchesCount = 0;
        mCurrentTabId = -1;
        mAnchorView = anchorView;
        mPopupWindow = new PopupWindow(anchorView.getContext());
        mPopupWindow.setWidth(ViewGroup.LayoutParams.WRAP_CONTENT);
        mPopupWindow.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        mPopupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            mPopupWindow.setElevation(20);
        }
        mIconFetcher =
                new BraveRewardsHelper(BraveRewardsHelper.currentActiveChromeTabbedActivityTab());

        mPopupWindow.setTouchInterceptor(new View.OnTouchListener() {
            @SuppressLint("ClickableViewAccessibility")
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_OUTSIDE) {
                    dismiss();
                    return true;
                }
                return false;
            }
        });
        mPopupWindow.setOnDismissListener(new PopupWindow.OnDismissListener() {
            @Override
            public void onDismiss() {
                if (mBalanceUpdater != null) {
                    mBalanceUpdater.cancel();
                }

                if (mPublisherFetcher != null) {
                    mPublisherFetcher.cancel();
                }

                if (mIconFetcher != null) {
                    mIconFetcher.detach();
                }

                // if (mBraveRewardsNativeWorker != null) {
                //     mBraveRewardsNativeWorker.RemoveObserver(this);
                // }

                if (mCurrentTabId != -1 && mBraveRewardsNativeWorker != null) {
                    mBraveRewardsNativeWorker.RemovePublisherFromMap(mCurrentTabId);
                }

                if (mBraveActivity != null) {
                    mBraveActivity.OnRewardsPanelDismiss();
                }
            }
        });
        mBraveActivity = BraveRewardsHelper.getBraveActivity();
        mActivity = BraveRewardsHelper.getChromeTabbedActivity();
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        // if (mBraveRewardsNativeWorker != null) {
        //     mBraveRewardsNativeWorker.AddObserver(this);
        // }
        mBalanceUpdater = new Timer();
        setUpViews();
    }

    private void setUpViews() {
        LayoutInflater inflater = (LayoutInflater) mAnchorView.getContext().getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        mPopupView = inflater.inflate(R.layout.brave_rewards_panel_layout, null);

        LinearLayout rewardsSummaryDetailLayout =
                mPopupView.findViewById(R.id.brave_rewards_panel_summary_layout_id);
        CardView rewardsTipLayout = mPopupView.findViewById(R.id.brave_rewards_panel_tip_layout_id);

        TextView btnAddFunds = mPopupView.findViewById(R.id.btn_add_funds);
        btnAddFunds.setOnClickListener(view
                -> {

                });

        TextView btnTip = mPopupView.findViewById(R.id.btn_tip);
        TextView btnSummary = mPopupView.findViewById(R.id.btn_summary);
        btnTip.setOnClickListener(view -> {
            btnTip.setTextColor(Color.parseColor("#4C54D2"));
            for (Drawable drawable : btnTip.getCompoundDrawables()) {
                if (drawable != null) {
                    drawable.setColorFilter(new PorterDuffColorFilter(
                            Color.parseColor("#4C54D2"), PorterDuff.Mode.SRC_IN));
                }
            }

            btnSummary.setTextColor(Color.parseColor("#868E96"));
            for (Drawable drawable : btnSummary.getCompoundDrawables()) {
                if (drawable != null) {
                    drawable.setColorFilter(new PorterDuffColorFilter(
                            Color.parseColor("#868E96"), PorterDuff.Mode.SRC_IN));
                }
            }
            rewardsSummaryDetailLayout.setVisibility(View.GONE);
            rewardsTipLayout.setVisibility(View.VISIBLE);
        });

        btnSummary.setOnClickListener(view -> {
            btnTip.setTextColor(Color.parseColor("#868E96"));
            for (Drawable drawable : btnTip.getCompoundDrawables()) {
                if (drawable != null) {
                    drawable.setColorFilter(new PorterDuffColorFilter(
                            Color.parseColor("#868E96"), PorterDuff.Mode.SRC_IN));
                }
            }

            btnSummary.setTextColor(Color.parseColor("#4C54D2"));
            for (Drawable drawable : btnSummary.getCompoundDrawables()) {
                if (drawable != null) {
                    drawable.setColorFilter(new PorterDuffColorFilter(
                            Color.parseColor("#4C54D2"), PorterDuff.Mode.SRC_IN));
                }
            }

            rewardsSummaryDetailLayout.setVisibility(View.VISIBLE);
            rewardsTipLayout.setVisibility(View.GONE);
        });

        mPopupWindow.setContentView(mPopupView);
    }

    public void showLikePopDownMenu() {
        mPopupWindow.setTouchable(true);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setOutsideTouchable(true);

        // mPopupWindow.setContentView(this.root);

        mPopupWindow.setAnimationStyle(R.style.OverflowMenuAnim);

        if (SysUtils.isLowEndDevice()) {
            mPopupWindow.setAnimationStyle(0);
        }

        mPopupWindow.showAsDropDown(mAnchorView, 0, 0);
        // checkForRewardsOnboarding();
        // BraveRewardsNativeWorker.getInstance().StartProcess();
    }

    public void dismiss() {
        mPopupWindow.dismiss();
    }

    public boolean isShowing() {
        return mPopupWindow.isShowing();
    }

    private void CreateUpdateBalanceTask() {
        mBalanceUpdater.schedule(new TimerTask() {
            @Override
            public void run() {
                if (mBraveRewardsNativeWorker == null) {
                    return;
                }
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        // mBraveRewardsNativeWorker.FetchGrants();
                    }
                });
            }
        }, 0, UPDATE_BALANCE_INTERVAL);
    }

    @Override
    public void onLargeIconReady(Bitmap icon) {
        setFavIcon(icon);
    }

    private void setFavIcon(Bitmap bitmap) {
        if (bitmap != null) {
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    ImageView publisherFaviconIcon =
                            mPopupView.findViewById(R.id.publisher_favicon);
                    publisherFaviconIcon.setImageBitmap(
                            BraveRewardsHelper.getCircularBitmap(bitmap));

                    View fadeout = mPopupView.findViewById(R.id.publisher_favicon_update);
                    BraveRewardsHelper.crossfade(fadeout, publisherFaviconIcon, View.GONE, 1f,
                            BraveRewardsHelper.CROSS_FADE_DURATION);
                }
            });
        }
    }
}
