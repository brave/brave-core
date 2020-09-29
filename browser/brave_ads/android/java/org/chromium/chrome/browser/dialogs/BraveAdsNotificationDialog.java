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
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.notifications.BraveOnboardingNotification;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.TabLaunchType;

public class BraveAdsNotificationDialog {

    static AlertDialog mAdsDialog;
    static String mNotificationId;

    public static void displayAdsNotification(Activity activity, final String notificationId,
            final String origin, final String title, final String body) {
        try {
            if (mAdsDialog != null) {
                mAdsDialog.dismiss();
            }
        } catch (IllegalArgumentException e) {
          mAdsDialog = null;
        }
        AlertDialog.Builder b = new AlertDialog.Builder(activity);

        LayoutInflater inflater = (LayoutInflater) activity.getLayoutInflater();
        b.setView(inflater.inflate(R.layout.brave_ads_custom_notification, null));
        mAdsDialog = b.create();

        mAdsDialog.show();

        Window window = mAdsDialog.getWindow();
        WindowManager.LayoutParams wlp = window.getAttributes();

        wlp.gravity = Gravity.TOP;
        wlp.dimAmount = 0.0f;
        wlp.flags |= WindowManager.LayoutParams.FLAG_DIM_BEHIND;

        mAdsDialog.setCanceledOnTouchOutside(false);
        mAdsDialog.setCancelable(false);

        window.setAttributes(wlp);

        window.setFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL);

        ImageView closeButton = mAdsDialog.findViewById(R.id.brave_ads_custom_notification_close_button);

        ((TextView) mAdsDialog.findViewById(R.id.brave_ads_custom_notification_header)).setText(title);
        ((TextView) mAdsDialog.findViewById(R.id.brave_ads_custom_notification_body)).setText(body);

        mNotificationId = notificationId;
        closeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mAdsDialog.dismiss();
                BraveAdsNativeHelper.nativeAdNotificationDismissed(Profile.getLastUsedRegularProfile(), mNotificationId);
            }
        });

        mAdsDialog.findViewById(R.id.brave_ads_custom_notification_popup).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // We don't take the user to the page in this class, native code handles opening a new tab for us.
                if (mNotificationId.equals(BraveOnboardingNotification.BRAVE_ONBOARDING_NOTIFICATION_TAG)) {
                    mAdsDialog.dismiss();
                    ChromeTabbedActivity chromeTabbedActivity = BraveActivity.getChromeTabbedActivity();
                    if (chromeTabbedActivity != null) {
                        chromeTabbedActivity.getTabCreator(false).launchUrl(origin, TabLaunchType.FROM_CHROME_UI);
                    }
                } else {
                    mAdsDialog.dismiss();
                    BraveAdsNativeHelper.nativeAdNotificationClicked(Profile.getLastUsedRegularProfile(), mNotificationId);
                }
            }
        });
    }


    @CalledByNative
    public static void displayAdsNotification(final String notificationId,
            final String origin, final String title, final String body) {
        BraveAdsNotificationDialog.displayAdsNotification(
            BraveActivity.getBraveActivity(),
            notificationId,
            origin,
            title,
            body
        );
    }

    @CalledByNative
    private static void closeAdsNotification(final String notificationId) {
        try {
            if (mNotificationId != null && mNotificationId.equals(notificationId) && mAdsDialog != null) {
                mAdsDialog.dismiss();
            }
        } catch (IllegalArgumentException e) {
            mAdsDialog = null;
        }

    }
}
