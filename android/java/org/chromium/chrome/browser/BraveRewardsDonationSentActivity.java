/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import android.app.Activity;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsSiteBannerActivity;

import static java.util.Locale.getDefault;
import java.text.SimpleDateFormat;

public class BraveRewardsDonationSentActivity extends Activity implements BraveRewardsHelper.LargeIconReadyCallback, BraveRewardsObserver  {

    private final int SLIDE_UP_DURATION = 2000;
    private final int PUBLISHER_ICON_SIDE_LEN= 70;
    private int currentTabId_ = -1;
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private BraveRewardsHelper mIconFetcher;
    private int mAmount_;
    private boolean mMonthly_tip_;
    private String mPublisher_name_;
    private java.util.Date mReconcileStamp_;
    private final Handler mHandler = new Handler(); //fadeout handler

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.brave_rewards_donation_sent);
        currentTabId_ = IntentUtils.safeGetIntExtra(getIntent(), BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, -1);
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mBraveRewardsNativeWorker.AddObserver(this);

        String publisherFavIconURL = mBraveRewardsNativeWorker.GetPublisherFavIconURL(currentTabId_);
        Tab currentActiveTab = BraveRewardsHelper.currentActiveTab();
        String url = currentActiveTab.getUrl();
        String favicon_url = (publisherFavIconURL.isEmpty()) ? url : publisherFavIconURL;
        mIconFetcher = new org.chromium.chrome.browser.BraveRewardsHelper();
        mIconFetcher.retrieveLargeIcon(favicon_url, this);
        SetData();
        SetAnimation();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if (null != mBraveRewardsNativeWorker) {
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
    }


    private void SetAnimation() {
        DisplayMetrics metrics = getResources().getDisplayMetrics();
        int height = metrics.heightPixels;
        View floater = findViewById(R.id.floater);


        Animation fadeIn = new AlphaAnimation(0, 1);
        fadeIn.setInterpolator(new AccelerateInterpolator());
        fadeIn.setStartOffset(0);
        fadeIn.setDuration(BraveRewardsHelper.THANKYOU_FADE_IN_DURATION);


        fadeIn.setAnimationListener(new Animation.AnimationListener() {
            @Override
            public void onAnimationStart(Animation arg0) { }
            @Override
            public void onAnimationRepeat(Animation arg0) {}
            @Override
            public void onAnimationEnd(Animation arg0) {
                //stay and then fade out
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        runOnUiThread(new Runnable() {
                              @Override
                              public void run() {
                                  //fade out animation
                                  Animation fadeOut = new AlphaAnimation(1, 0);
                                  fadeOut.setInterpolator(new AccelerateInterpolator());
                                  fadeOut.setStartOffset(0);
                                  fadeOut.setDuration(BraveRewardsHelper.THANKYOU_FADE_OUT_DURATION);

                                  fadeOut.setAnimationListener(new Animation.AnimationListener() {
                                      @Override
                                      public void onAnimationStart(Animation arg0) {}
                                      @Override
                                      public void onAnimationRepeat(Animation arg0) {}
                                      @Override
                                      public void onAnimationEnd(Animation arg0) {
                                          finish();
                                      }
                                  });

                                  findViewById(R.id.floater).startAnimation(fadeOut);

                              }
                          }
                        );

                    }
                }, BraveRewardsHelper.THANKYOU_STAY_DURATION);

            }
        });
        floater.startAnimation(fadeIn);
    }


    private void SetData() {
        Intent intent = getIntent();
        if (-1 == currentTabId_) {
            currentTabId_ = IntentUtils.safeGetIntExtra(intent, BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, -1);
        }

        mPublisher_name_ = mBraveRewardsNativeWorker.GetPublisherName(currentTabId_);
        mAmount_ = IntentUtils.safeGetIntExtra (intent, BraveRewardsSiteBannerActivity.TIP_AMOUNT_EXTRA, 0);
        mMonthly_tip_ = IntentUtils.safeGetBooleanExtra (intent, BraveRewardsSiteBannerActivity.TIP_MONTHLY_EXTRA, false);

        //set the data
        String strAmount = String.format(getDefault(), "%.1f BAT", (float)mAmount_);
        ((TextView)findViewById(R.id.txt_pub_name)).setText(mPublisher_name_);

        if (true == mMonthly_tip_) {
            //change `txt_you_sent` message
            String monthly_send_msg = getResources().getString(R.string.brave_ui_auto_tip_text);
            ((TextView)findViewById(R.id.txt_you_sent)).setText(monthly_send_msg);

            //add 'Monthly' to amount
            strAmount += ", ";
            String monthly = getResources().getString(R.string.brave_ui_monthly_text);
            strAmount += monthly;

            //get next reconcile stamp
            //the `txt_tip_will_be_sent` and `txt_send_date` will be made visible in OnGetReconcileStamp
            mBraveRewardsNativeWorker.GetReconcileStamp();

        }
        ((TextView)findViewById(R.id.txt_amount)).setText(strAmount);
    }


    private void SetFavIcon(Bitmap bmp) {
        if (bmp != null) {
            runOnUiThread(
                    new Runnable() {
                        @Override
                        public void run() {
                            ImageView iv = (ImageView) findViewById(R.id.publisher_favicon);
                            int nPx = BraveRewardsHelper.dp2px(PUBLISHER_ICON_SIDE_LEN);
                            Bitmap resized = Bitmap.createScaledBitmap(bmp, nPx, nPx, true);

                            View fadeout  = findViewById(R.id.publisher_favicon_update);
                            BraveRewardsHelper.crossfade(fadeout, iv, View.GONE, 1f, BraveRewardsHelper.CROSS_FADE_DURATION);
                            iv.setImageBitmap(BraveRewardsHelper.getCircularBitmap(resized));
                        }
                    });
        }
    }

    @Override
    public void onLargeIconReady(Bitmap icon) {
        SetFavIcon(icon);
    }


    // BraveRewardsObserver/////////////////////////////////////
    @Override
    public void OnWalletInitialized(int error_code) {}

    @Override
    public void OnWalletProperties(int error_code) {}

    @Override
    public void OnPublisherInfo(int tabId) {}

    @Override
    public void OnGetCurrentBalanceReport(String[] report) {}

    @Override
    public void OnNotificationAdded(String id, int type, long timestamp, String[] args) {}

    @Override
    public void OnNotificationsCount(int count) {}

    @Override
    public void OnGetLatestNotification(String id, int type, long timestamp, String[] args) {}

    @Override
    public void OnNotificationDeleted(String id) {}

    @Override
    public void OnIsWalletCreated(boolean created) {}

    @Override
    public void OnGetPendingContributionsTotal(double amount) {}

    @Override
    public void OnGetRewardsMainEnabled(boolean enabled) {}

    @Override
    public void OnGetAutoContributeProps() {}

    @Override
    public void OnRecurringDonationUpdated() {}

    @Override
    public void OnGetReconcileStamp(long timestamp) {
        //make reconcile date views visible
        if (true == mMonthly_tip_) {
            findViewById(R.id.txt_tip_will_be_sent).setVisibility(View.VISIBLE);
            
            TextView send_date = (TextView) findViewById(R.id.txt_send_date);
            send_date.setVisibility(View.VISIBLE);
            mReconcileStamp_ = new java.util.Date(timestamp * 1000); //ms
            SimpleDateFormat formatter = new SimpleDateFormat("dd/MM/yyyy", getDefault());
            send_date.setText(formatter.format (mReconcileStamp_));
        }
    }

    @Override
    public void OnResetTheWholeState(boolean success) {}

    @Override
    public void OnRewardsMainEnabled(boolean enabled) {}
}
