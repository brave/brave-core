 /* Copyright (c) 2020 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.dialogs;

import android.animation.ValueAnimator;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;

import org.jni_zero.CalledByNative;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.notifications.BraveOnboardingNotification;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.tab.TabLaunchType;

public class BraveAdsNotificationDialog {
    private static AlertDialog mAdsDialog;
    private static String mNotificationId;

    private static class AdsNotificationTouchListener implements View.OnTouchListener {
        private static final int MIN_DISTANCE_FOR_DISMISS = 40;
        private static final int MAX_DISTANCE_FOR_TAP = 5;

        private Context mContext;
        private final String mOrigin;
        private WindowManager.LayoutParams mLayoutParams;
        private ValueAnimator mAnimator;
        // Track when touch events on the dialog are down and when they are up
        private int mXDown;
        private int mWindowInitialPos;

        public AdsNotificationTouchListener(Context context, final String origin) {
            mContext = context;
            mOrigin = origin;
            mLayoutParams = mAdsDialog.getWindow().getAttributes();
            mWindowInitialPos = mLayoutParams.x;
        }

        @Override
        public boolean onTouch(View v, MotionEvent event) {
            float deltaXDp;
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    if (mAnimator != null) mAnimator.cancel();
                    mXDown = mLayoutParams.x - (int) event.getRawX();
                    break;
                case MotionEvent.ACTION_MOVE:
                    updateWindowPosition((int) event.getRawX() + mXDown);
                    break;
                case MotionEvent.ACTION_UP:
                    v.performClick();
                    if (mXDown != 0) {
                        deltaXDp = pxToDp(event.getRawX() + mXDown - mWindowInitialPos,
                                mContext.getResources().getDisplayMetrics());
                    } else {
                        return false;
                    }
                    if (Math.abs(deltaXDp) > MIN_DISTANCE_FOR_DISMISS) {
                        mAdsDialog.dismiss();
                        mAdsDialog = null;
                        BraveAdsNativeHelper.nativeOnNotificationAdClosed(
                                ProfileManager.getLastUsedRegularProfile(), mNotificationId, true);
                        mNotificationId = null;
                    } else if (Math.abs(deltaXDp) <= MAX_DISTANCE_FOR_TAP) {
                        adsDialogTapped(mOrigin);
                    } else {
                        translateWindowToOrigin();
                    }
                    break;
            }
            return true;
        }

        private void updateWindowPosition(int x) {
            mLayoutParams.x = x;
            if (mAdsDialog != null) mAdsDialog.getWindow().setAttributes(mLayoutParams);
        }

        private void translateWindowToOrigin() {
            mAnimator = ValueAnimator.ofInt(mLayoutParams.x, mWindowInitialPos);
            mAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
                @Override
                public void onAnimationUpdate(ValueAnimator valueAnimator) {
                    updateWindowPosition((int) valueAnimator.getAnimatedValue());
                }
            });
            mAnimator.start();
        }

        /**
         * Converts a px value to a dp value.
         */
        private int pxToDp(float value, DisplayMetrics metrics) {
            return Math.round(
                    value / ((float) metrics.densityDpi / DisplayMetrics.DENSITY_DEFAULT));
        }
    };

    public static void showNotificationAd(Context context, final String notificationId,
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
        if (shouldUseDarkModeTheme()) {
            b.setView(inflater.inflate(R.layout.brave_ads_custom_notification_dark, null));
        } else {
            b.setView(inflater.inflate(R.layout.brave_ads_custom_notification, null));
        }
        mAdsDialog = b.create();

        if (mNotificationId != null) {
            BraveAdsNativeHelper.nativeOnNotificationAdShown(
                    ProfileManager.getLastUsedRegularProfile(), mNotificationId);
        }

        mAdsDialog.show();

        Window window = mAdsDialog.getWindow();
        WindowManager.LayoutParams wlp = window.getAttributes();

        wlp.gravity = Gravity.TOP;
        wlp.flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;

        mAdsDialog.setCanceledOnTouchOutside(false);
        mAdsDialog.setCancelable(false);

        window.setAttributes(wlp);

        window.clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);
        window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));

        ((TextView) mAdsDialog.findViewById(R.id.brave_ads_custom_notification_header)).setText(title);
        ((TextView) mAdsDialog.findViewById(R.id.brave_ads_custom_notification_body)).setText(body);

        mNotificationId = notificationId;
        mAdsDialog.findViewById(R.id.brave_ads_custom_notification_popup)
                .setOnTouchListener(new AdsNotificationTouchListener(context, origin));
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
            BraveAdsNativeHelper.nativeOnNotificationAdClicked(
                    ProfileManager.getLastUsedRegularProfile(), mNotificationId);
        }
        mNotificationId = null;
    }

    @CalledByNative
    public static void showNotificationAd(final String notificationId, final String origin,
            final String title, final String body) {
        Activity activity = ApplicationStatus.getLastTrackedFocusedActivity();
        assert activity != null;
        // We want to show ads only when activity is in started or resumed
        // state
        int state = ApplicationStatus.getStateForActivity(activity);
        if (activity == null || (state != ActivityState.STARTED && state != ActivityState.RESUMED))
            return;

        BraveAdsNotificationDialog.showNotificationAd(
                activity, notificationId, origin, title, body);
    }

    @CalledByNative
    private static void closeNotificationAd(final String notificationId) {
        try {
            if (mNotificationId != null && mNotificationId.equals(notificationId) && mAdsDialog != null) {
                mAdsDialog.dismiss();
                mAdsDialog = null;
                BraveAdsNativeHelper.nativeOnNotificationAdClosed(
                        ProfileManager.getLastUsedRegularProfile(), mNotificationId, false);
            }
        } catch (IllegalArgumentException e) {
            mAdsDialog = null;
        }
    }

    private static boolean shouldUseDarkModeTheme() {
        return GlobalNightModeStateProviderHolder.getInstance().isInNightMode();
    }
}
