/** Copyright (c) 2020 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */

package org.chromium.chrome.browser.dialogs;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.notifications.BraveOnboardingNotification;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.TabLaunchType;

public class BraveAdsNotificationDialog {

    static AlertDialog mAdsDialog;
    static String mNotificationId;
    static final int MIN_DISTANCE_FOR_DISMISS = 20;
    static final int MAX_DISTANCE_FOR_TAP = 5;

    // Track when touch events on the dialog are down and when they are up
    static float mYDown;

    public static void showAdNotification(Context context, final String notificationId,
            final String origin, final String title, final String body) {
        try {
            if (mAdsDialog != null) {
                mAdsDialog.dismiss();
            }
        } catch (IllegalArgumentException e) {
          mAdsDialog = null;
        }
        AlertDialog.Builder b = new AlertDialog.Builder(context);

        mNotificationId = notificationId;

        LayoutInflater inflater = LayoutInflater.from(context);
        b.setView(inflater.inflate(R.layout.brave_ads_custom_notification, null));
        mAdsDialog = b.create();

        if (mNotificationId != null) {
            BraveAdsNativeHelper.nativeOnShowAdNotification(
                    Profile.getLastUsedRegularProfile(), mNotificationId);
        }

        mAdsDialog.show();

        Window window = mAdsDialog.getWindow();
        WindowManager.LayoutParams wlp = window.getAttributes();

        wlp.gravity = Gravity.TOP;
        wlp.flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;

        mAdsDialog.setCanceledOnTouchOutside(false);
        mAdsDialog.setCancelable(false);

        window.setAttributes(wlp);

        window.clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);
        window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));

        ((TextView) mAdsDialog.findViewById(R.id.brave_ads_custom_notification_header)).setText(title);
        ((TextView) mAdsDialog.findViewById(R.id.brave_ads_custom_notification_body)).setText(body);

        mNotificationId = notificationId;
        mAdsDialog.findViewById(R.id.brave_ads_custom_notification_popup)
                .setOnTouchListener(new View.OnTouchListener() {
                    @Override
                    public boolean onTouch(View v, MotionEvent event) {
                        float deltaY;
                        float deltaYDp;
                        float y;
                        switch (event.getAction()) {
                            case MotionEvent.ACTION_DOWN:
                                mYDown = v.getY() - event.getRawY();
                                break;
                            case MotionEvent.ACTION_MOVE:
                                deltaY = event.getRawY() + mYDown;
                                if (deltaY > 0) {
                                    deltaY = 0;
                                }
                                v.animate().y(deltaY).setDuration(0).start();
                                break;
                            case MotionEvent.ACTION_UP:
                                if (mYDown != 0.0f) {
                                    deltaYDp = pxToDp(event.getRawY() + mYDown,
                                            context.getResources().getDisplayMetrics());
                                } else {
                                    return false;
                                }
                                if (deltaYDp < -1 * MIN_DISTANCE_FOR_DISMISS) {
                                    mAdsDialog.dismiss();
                                    mAdsDialog = null;
                                    BraveAdsNativeHelper.nativeOnCloseAdNotification(
                                            Profile.getLastUsedRegularProfile(), mNotificationId,
                                            false);
                                    mNotificationId = null;
                                } else if (deltaYDp <= MAX_DISTANCE_FOR_TAP
                                        && deltaYDp >= (-1 * MAX_DISTANCE_FOR_TAP)) {
                                    adsDialogTapped(origin);
                                } else {
                                    v.animate().translationY(0);
                                }
                                break;
                        }
                        return true;
                    }
                });
    }

    private static void adsDialogTapped(final String origin) {
        if (mNotificationId.equals(BraveOnboardingNotification.BRAVE_ONBOARDING_NOTIFICATION_TAG)) {
            mAdsDialog.dismiss();
            mAdsDialog = null;
            ChromeTabbedActivity chromeTabbedActivity = BraveActivity.getChromeTabbedActivity();
            if (chromeTabbedActivity != null) {
                chromeTabbedActivity.getTabCreator(false).launchUrl(
                        origin, TabLaunchType.FROM_CHROME_UI);
            }
        } else {
            mAdsDialog.dismiss();
            mAdsDialog = null;
            BraveAdsNativeHelper.nativeOnClickAdNotification(
                    Profile.getLastUsedRegularProfile(), mNotificationId);
        }
        mNotificationId = null;
    }

    @CalledByNative
    public static void showAdNotification(final String notificationId,
            final String origin, final String title, final String body) {
        Activity activity = ApplicationStatus.getLastTrackedFocusedActivity();
        assert activity != null;
        // We want to show ads only when activity is in started or resumed
        // state
        int state = ApplicationStatus.getStateForActivity(activity);
        if (activity == null || (state != ActivityState.STARTED &&
                    state != ActivityState.RESUMED))
            return;

        BraveAdsNotificationDialog.showAdNotification(activity, notificationId,
            origin, title, body);
    }

    @CalledByNative
    private static void closeAdsNotification(final String notificationId) {
        try {
            if (mNotificationId != null && mNotificationId.equals(notificationId) && mAdsDialog != null) {
                mAdsDialog.dismiss();
                BraveAdsNativeHelper.nativeOnCloseAdNotification(
                        Profile.getLastUsedRegularProfile(), mNotificationId, false);
                mAdsDialog = null;
            }
        } catch (IllegalArgumentException e) {
            mAdsDialog = null;
        }
    }

    /**
     * Converts a px value to a dp value.
     */
    private static int pxToDp(float value, DisplayMetrics metrics) {
        return Math.round(value / ((float) metrics.densityDpi / DisplayMetrics.DENSITY_DEFAULT));
    }
}
