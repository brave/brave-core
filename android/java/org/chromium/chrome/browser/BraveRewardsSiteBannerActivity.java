/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.view.animation.TranslateAnimation;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.ToggleButton;
import android.text.Spanned;
import android.content.Context;
import android.view.MotionEvent;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import android.graphics.Bitmap;
import android.widget.ImageView;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.widget.RoundedIconGenerator;
import org.chromium.base.task.AsyncTask;
import android.graphics.BitmapFactory;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.ContextUtils;

import java.io.IOException;
import java.math.RoundingMode;
import java.net.MalformedURLException;
import java.net.URL;
import java.text.DecimalFormat;
import java.util.Locale;

public class BraveRewardsSiteBannerActivity extends Activity implements BraveRewardsHelper.LargeIconReadyCallback, BraveRewardsObserver {

    private  ToggleButton radio_tip_amount[] = new ToggleButton [3];
    private final int TIP_SENT_REQUEST_CODE = 2;
    private final int FADE_OUT_DURATION = 500;
    private final float LANDSCAPE_HEADER_WEIGHT = 2.0f;
    public static final String TAB_ID_EXTRA = "currentTabId";
    public static final String TIP_AMOUNT_EXTRA="tipAmount";
    public static final String TIP_MONTHLY_EXTRA="tipMonthly";


    private int currentTabId_ = -1;
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private final int PUBLISHER_ICON_SIDE_LEN= 70; //dp
    private final int TOUCH_PADDING_MONTHLY= 32; //dp
    private final int ACTIVITY_HEADER_PADDING = 10; //dp
    private final int ACTIVITY_HEADER_CONTENT_SIDE_MARGIN = 22; //dp
    private BraveRewardsHelper mIconFetcher;
    private boolean mTippingInProgress; //flag preventing multiple tipping processes

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        //inflate
        super.onCreate(savedInstanceState);
        setContentView(R.layout.brave_rewards_site_banner);

        //change weight of the footer in landscape mode
        int orientation = this.getResources().getConfiguration().orientation;
        if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
            LinearLayout.LayoutParams param = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    0,
                    LANDSCAPE_HEADER_WEIGHT
            );
            findViewById(R.id.site_banner_header).setLayoutParams(param);
        }

        //bind tip amount custom radio buttons
        radio_tip_amount[0] = findViewById(R.id.one_bat_option);
        radio_tip_amount[1] = findViewById(R.id.five_bat_option);
        radio_tip_amount[2] = findViewById(R.id.ten_bat_option);


        //radio buttons behaviour
        View.OnClickListener radio_clicker = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ToggleButton tb_pressed = (ToggleButton)view;
                if ( tb_pressed.isChecked() == false){
                    tb_pressed.setChecked(true);
                    return;
                }

                int id = view.getId();
                for (ToggleButton tb : radio_tip_amount){
                    if (tb.getId() == id){
                        continue;
                    }
                    tb.setChecked(false);
                }
            }
        };

        for (ToggleButton tb : radio_tip_amount){
            tb.setOnClickListener(radio_clicker);
        }

        //set default tip amount
        radio_tip_amount[1].setChecked(true);

        currentTabId_ = IntentUtils.safeGetIntExtra(getIntent(), TAB_ID_EXTRA, -1);
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mBraveRewardsNativeWorker.AddObserver(this);

        String publisherName = mBraveRewardsNativeWorker.GetPublisherName(currentTabId_);
        TextView publisher = (TextView)findViewById(R.id.publisher_name);
        publisher.setText(publisherName);

        String publisherFavIconURL = mBraveRewardsNativeWorker.GetPublisherFavIconURL(currentTabId_);
        Tab currentActiveTab = BraveRewardsHelper.currentActiveTab();
        String url = currentActiveTab.getUrl();
        String favicon_url = (publisherFavIconURL.isEmpty()) ? url : publisherFavIconURL;
        mIconFetcher = new BraveRewardsHelper();
        mIconFetcher.retrieveLargeIcon(favicon_url, this);

        double balance = mBraveRewardsNativeWorker.GetWalletBalance();

        DecimalFormat df = new DecimalFormat("#.#");
        df.setRoundingMode(RoundingMode.FLOOR);
        df.setMinimumFractionDigits(1);
        String walletAmount = df.format(balance) + " BAT";

        ((TextView)findViewById(R.id.wallet_amount_text)).setText(walletAmount);

        double usdValue = mBraveRewardsNativeWorker.GetWalletRate("USD");
        double fiveBat = 5 * usdValue;
        double tenBat = 10 * usdValue;
        String oneBatRate = String.format(Locale.getDefault(), "%.2f", usdValue) + " USD";
        String fiveBatRate = String.format(Locale.getDefault(), "%.2f", fiveBat) + " USD";
        String tenBatRate = String.format(Locale.getDefault(), "%.2f", tenBat) + " USD";
        ((TextView)findViewById(R.id.one_bat_rate)).setText(oneBatRate);
        ((TextView)findViewById(R.id.five_bat_rate)).setText(fiveBatRate);
        ((TextView)findViewById(R.id.ten_bat_rate)).setText(tenBatRate);

        //set tip button onClick
        View.OnClickListener send_tip_clicker = new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                double balance = mBraveRewardsNativeWorker.GetWalletBalance();
                int amount = 0;
                for (ToggleButton tb : radio_tip_amount){
                    if (tb.isChecked()) {
                        int id = tb.getId();
                        if (id == R.id.one_bat_option) {
                            amount = 1;
                        } else if (id == R.id.five_bat_option) {
                            amount = 5;
                        } else if (id == R.id.ten_bat_option) {
                            amount = 10;
                        }

                        break;
                    }
                }
                boolean enough_funds = ((balance - amount) >= 0);

                //proceed to tipping
                if (true == enough_funds) {
                    if (mTippingInProgress){
                        return;
                    }
                    mTippingInProgress = true;

                    CheckBox monthly = (CheckBox)findViewById(R.id.make_monthly_checkbox);
                    boolean monthly_bool = monthly.isChecked();
                    mBraveRewardsNativeWorker.Donate(mBraveRewardsNativeWorker.GetPublisherId(currentTabId_),
                        amount, monthly_bool);
                    Intent intent = new Intent(getApplicationContext(), BraveRewardsDonationSentActivity.class);
                    intent.putExtra(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, currentTabId_);
                    intent.putExtra(BraveRewardsSiteBannerActivity.TIP_AMOUNT_EXTRA, amount);
                    intent.putExtra(BraveRewardsSiteBannerActivity.TIP_MONTHLY_EXTRA, monthly_bool);
                    startActivityForResult(intent,TIP_SENT_REQUEST_CODE);
                }
                //not enough funds
                else {
                    View send_tip = findViewById(R.id.send_donation_button);
                    send_tip.setVisibility(View.INVISIBLE);
                    View animatedView = findViewById(R.id.not_enough_funds_button);
                    int fromY = findViewById(R.id.send_donation_button).getHeight();

                    TranslateAnimation animate = new TranslateAnimation(0,0,fromY,0);
                    animate.setDuration(FADE_OUT_DURATION);
                    animatedView.startAnimation(animate);
                    animatedView.setVisibility(View.VISIBLE);
                }
            }
        };

        findViewById(R.id.send_donation_button).setOnClickListener (send_tip_clicker);


        //set 'add funds' button onClick
         View.OnClickListener add_funds_clicker = new View.OnClickListener() {
             @Override
             public void onClick(View view) {
                 /*
                 //Temporary disable Add Funds functionality and add return to `Send Tip` button
                 Intent returnIntent = new Intent();
                 setResult(ChromeTabbedActivity.SITE_BANNER_ADD_FUNDS_RESULT_CODE, returnIntent);
                 finish();
                 */

                 View add_funds_btn = findViewById(R.id.not_enough_funds_button);
                 add_funds_btn.setVisibility(View.INVISIBLE);

                 View animatedView = findViewById(R.id.send_donation_button);
                 int fromY = findViewById(R.id.send_donation_button).getHeight();
                 TranslateAnimation animate = new TranslateAnimation(0,0,fromY,0);
                 animate.setDuration(FADE_OUT_DURATION);
                 animatedView.startAnimation(animate);
                 animatedView.setVisibility(View.VISIBLE);
             }
         };

        View not_enough_funds_btn = findViewById(R.id.not_enough_funds_button);
        not_enough_funds_btn.setOnClickListener (add_funds_clicker);

        String part1 = getResources().getString(R.string.brave_ui_not_enough_tokens);
        String part2 = getResources().getString(R.string.brave_ui_please);
        String part3 = getResources().getString(R.string.brave_ui_add_funds);

        part1 = part1.substring(0,1).toUpperCase(Locale.getDefault()) + part1.substring(1);
        part2 = part2.substring(0,1).toUpperCase(Locale.getDefault()) + part2.substring(1);

        Spanned toInsert;
        //Temporary disabled
        /*
        StringBuilder sb = new StringBuilder();
        sb.append(part1);
        sb.append(". ");
        sb.append(part2);
        sb.append(" <u>");
        sb.append(part3);
        sb.append("</u>.");
        toInsert = BraveRewardsHelper.spannedFromHtmlString(sb.toString());
        TextView tv = (TextView)findViewById(R.id.not_enough_funds_text);
        tv.setText(toInsert);
        */

        //Once Add Funds is enabled: uncomment above block and delete next 2 lines
        TextView tv = (TextView)findViewById(R.id.not_enough_funds_text);
        tv.setText(part1);

        ///////////////////////////////////////////////////////////////////////////////////////
        boolean verified = mBraveRewardsNativeWorker.GetPublisherVerified(currentTabId_);
        if (!verified) {
            findViewById(R.id.not_verified_warning_layout ).setVisibility(View.VISIBLE);

            part1 = getResources().getString(R.string.brave_ui_site_banner_notice_text);
            part2 = getResources().getString(R.string.learn_more);

            final StringBuilder sb1 = new StringBuilder();
            sb1.append(part1);
            sb1.append(" <br><font color=#00afff>");
            sb1.append(part2);
            sb1.append("</font></br>");
            toInsert = BraveRewardsHelper.spannedFromHtmlString(sb1.toString());
            TextView not_verified_warning_text = (TextView )findViewById(R.id.not_verified_warning_text );
            not_verified_warning_text.setText(toInsert);

            not_verified_warning_text.setOnTouchListener(new View.OnTouchListener() {
                @Override
                public boolean onTouch(View view, MotionEvent motionEvent) {
                    if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
                        int offset = not_verified_warning_text.getOffsetForPosition(
                                motionEvent.getX(), motionEvent.getY());

                        String learn_more = getResources().getString(R.string.learn_more);
                        if (BraveRewardsHelper.subtextAtOffset(not_verified_warning_text.getText().toString(), learn_more, offset) ){
                            Intent returnIntent = new Intent();
                            // TODO
                            // setResult(ChromeTabbedActivity.SITE_BANNER_NOT_VERIFIED_LEARN_MORE_RESULT_CODE, returnIntent);
                            finish();
                        }
                    }
                    return false;
                }
            });
        }


        RelativeLayout monthly_layout = ((RelativeLayout)findViewById(R.id.monthly_contribution));
        if (mBraveRewardsNativeWorker.IsCurrentPublisherInRecurrentDonations(mBraveRewardsNativeWorker.GetPublisherId(currentTabId_))) {
            monthly_layout.setVisibility(View.INVISIBLE);
        } else {
            // `monthly_contribution` is VISIBLE by default
            CheckBox monthly_chk = (CheckBox) findViewById(R.id.make_monthly_checkbox);
            BraveRewardsHelper.expandTouchArea (monthly_layout, monthly_chk, TOUCH_PADDING_MONTHLY);
        }


        //put icon `publisher_favicon_holder` at the middle of the position of `header_scrollView`
        final View layout = (View) findViewById(R.id.header_scrollView);
        ViewTreeObserver vto = layout.getViewTreeObserver();
        vto.addOnGlobalLayoutListener (new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                layout.getViewTreeObserver().removeOnGlobalLayoutListener(this);

                int top = layout.getTop();
                int newY = top - BraveRewardsHelper.dp2px( PUBLISHER_ICON_SIDE_LEN / 2 + ACTIVITY_HEADER_PADDING);
                int minTopPosition = BraveRewardsHelper.dp2px(ACTIVITY_HEADER_CONTENT_SIDE_MARGIN);

                if (newY < minTopPosition){
                    newY = minTopPosition;
                }

                RelativeLayout icon_layout  = (RelativeLayout)findViewById(R.id.publisher_favicon_layout);
                FrameLayout.LayoutParams params = (FrameLayout.LayoutParams)icon_layout.getLayoutParams();
                params.setMargins(params.leftMargin, newY, params.rightMargin, params.bottomMargin);
                icon_layout.setLayoutParams(params);
                icon_layout.requestLayout();
            }
        });
    }


    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);
        if(TIP_SENT_REQUEST_CODE == requestCode)
        {
            //disable the activity while fading it out
            getWindow().setFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE, WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
            finish();
            overridePendingTransition(R.anim.activity_fade_in, R.anim.activity_fade_out);
            mTippingInProgress = false;

        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (isFinishing()){
            overridePendingTransition(R.anim.activity_fade_in, R.anim.activity_fade_out);
        }
    }

    @Override
    protected void onDestroy(){
        super.onDestroy();

        if (mIconFetcher != null) {
            mIconFetcher.detach();
        }

        if (null != mBraveRewardsNativeWorker){
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
    }


    private void SetFavIcon(Bitmap bmp) {
        if (bmp != null){
            runOnUiThread(
                new Runnable(){
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
    public void onLargeIconReady(Bitmap icon){
        SetFavIcon(icon);
    }



    //`make_monthly_checkbox` checkbox click handler
    public void onMonthlyCheckboxClicked(View view) {
        boolean checked = ((CheckBox) view).isChecked();
        TextView send_btn = (TextView) findViewById(R.id.send_donation_text);

        int string_id = (checked)? R.string.brave_ui_do_monthly : R.string.brave_ui_send_tip;
        String btn_text = getResources().getString(string_id);
        send_btn.setText(btn_text);
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        super.onSaveInstanceState(savedInstanceState);

        boolean monthly = ((CheckBox)findViewById(R.id.make_monthly_checkbox)).isChecked();
        savedInstanceState.putBoolean(TIP_MONTHLY_EXTRA, monthly);
    }


    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);

        boolean myBoolean = savedInstanceState.getBoolean(TIP_MONTHLY_EXTRA);
        CheckBox cb = (CheckBox)findViewById(R.id.make_monthly_checkbox);
        cb.setChecked(myBoolean);

        //run init logic in onMonthlyCheckboxClicked
        onMonthlyCheckboxClicked(cb);
    }



    // BraveRewardsObserver/////////////////////////////////////
    @Override
    public void OnWalletInitialized(int error_code){}

    @Override
    public void OnWalletProperties(int error_code){
        if (error_code == 0) {
            if (mBraveRewardsNativeWorker != null) {
                double balance = mBraveRewardsNativeWorker.GetWalletBalance();
                DecimalFormat df = new DecimalFormat("#.#");
                df.setRoundingMode(RoundingMode.FLOOR);
                df.setMinimumFractionDigits(1);
                String walletAmount = df.format(balance) + " BAT";
                ((TextView)findViewById(R.id.wallet_amount_text)).setText(walletAmount);
            }
        }
    }

    @Override
    public void OnPublisherInfo(int tabId){}

    @Override
    public void OnGetCurrentBalanceReport(String[] report){}

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
    public void OnGetReconcileStamp(long timestamp) {}

    @Override
    public void OnRecurringDonationUpdated() {}

    @Override
    public void OnResetTheWholeState(boolean success) {}

    @Override
    public void OnRewardsMainEnabled(boolean enabled) {}
}
