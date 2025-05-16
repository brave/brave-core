/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.tipping;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.PopupWindow;

import androidx.annotation.NonNull;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.chrome.R;
import org.chromium.ui.base.DeviceFormFactor;

public class MonthlyContributionToolTip {
    private PopupWindow mPopupWindow;
    View mContentView;
    private final boolean mIsTablet;

    public MonthlyContributionToolTip(@NonNull Context context) {
        mIsTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(context);
        init(context);
    }

    @SuppressLint("ClickableViewAccessibility")
    private void init(Context context) {
        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mContentView = inflater.inflate(R.layout.monthly_contribution_tooltip, null, false);

        mPopupWindow = new PopupWindow(context);
        mPopupWindow.setContentView(mContentView);
        mPopupWindow.setTouchable(true);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setBackgroundDrawable(AppCompatResources.getDrawable(
                context, R.drawable.tipping_verified_creator_tooltip_background));
        mPopupWindow.setElevation(60);
        mPopupWindow.setWidth((int) dpToPx(context, 280.0f));
        mPopupWindow.setTouchInterceptor((v, event) -> {
            if (event.getAction() == MotionEvent.ACTION_OUTSIDE) {
                mPopupWindow.dismiss();
                return true;
            }
            return false;
        });
    }

    public void show(@NonNull View anchorView) {
        if (mIsTablet) {
            anchorView.post(
                    new Runnable() {
                        @Override
                        public void run() {
                            mPopupWindow.showAtLocation(
                                    anchorView,
                                    Gravity.TOP,
                                    (int) anchorView.getX(),
                                    (int) anchorView.getY());
                        }
                    });
        } else {
            int[] location = new int[2];
            anchorView.getLocationInWindow(location);
            mPopupWindow.showAtLocation(anchorView, Gravity.TOP, 0, location[1] - 250);
        }
    }
}
