/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.SharedPreferences;
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
import android.text.Html;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
import android.view.ContextThemeWrapper;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SwitchCompat;
import androidx.cardview.widget.CardView;
import androidx.core.content.res.ResourcesCompat;

import org.json.JSONException;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.SysUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsExternalWallet;
import org.chromium.chrome.browser.BraveRewardsExternalWallet.WalletStatus;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsPublisher;
import org.chromium.chrome.browser.BraveRewardsSiteBannerActivity;
import org.chromium.chrome.browser.BraveRewardsUserWalletActivity;
import org.chromium.chrome.browser.BraveRewardsVerifyWalletActivity;
import org.chromium.chrome.browser.BraveWalletProvider;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;

import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

public class BraveRewardsPanel
        implements BraveRewardsObserver, BraveRewardsHelper.LargeIconReadyCallback {
    private static final String TAG = "BraveRewards";
    private static final int UPDATE_BALANCE_INTERVAL = 60000; // In milliseconds
    private static final int PUBLISHER_INFO_FETCH_RETRY = 3 * 1000; // In milliseconds
    private static final int PUBLISHER_FETCHES_COUNT = 3;

    private static final String YOUTUBE_TYPE = "youtube#";
    private static final String TWITCH_TYPE = "twitch#";

    private static final String PREF_VERIFY_WALLET_ENABLE = "verify_wallet_enable";

    private static final int WALLET_BALANCE_LIMIT = 15;

    // Balance report codes
    private static final int BALANCE_REPORT_GRANTS = 0;
    private static final int BALANCE_REPORT_EARNING_FROM_ADS = 1;
    private static final int BALANCE_REPORT_AUTO_CONTRIBUTE = 2;
    private static final int BALANCE_REPORT_RECURRING_DONATION = 3;
    private static final int BALANCE_REPORT_ONE_TIME_DONATION = 4;

    // Custom Android notification
    private static final char NOTIFICATION_PROMID_SEPARATOR = '_';
    private static final int REWARDS_NOTIFICATION_NO_INTERNET = 1000;
    private static final String REWARDS_NOTIFICATION_NO_INTERNET_ID =
            "29d835c2-5752-4152-93c3-8a1ded9dd4ec";
    private static final int REWARDS_PROMOTION_CLAIM_ERROR = REWARDS_NOTIFICATION_NO_INTERNET + 1;
    private static final String REWARDS_PROMOTION_CLAIM_ERROR_ID =
            "rewards_promotion_claim_error_id";

    // Auto contribute results
    private static final String AUTO_CONTRIBUTE_SUCCESS = "0";
    private static final String AUTO_CONTRIBUTE_GENERAL_ERROR = "1";
    private static final String AUTO_CONTRIBUTE_NOT_ENOUGH_FUNDS = "15";
    private static final String AUTO_CONTRIBUTE_TIPPING_ERROR = "16";
    private static final String ERROR_CONVERT_PROBI = "ERROR";

    private static final int CLICK_DISABLE_INTERVAL = 1000; // In milliseconds

    private final View mAnchorView;
    private final PopupWindow mPopupWindow;
    private ViewGroup mPopupView;
    private final BraveActivity mBraveActivity;
    private final ChromeTabbedActivity mActivity;
    private BraveRewardsHelper mIconFetcher;

    private LinearLayout mRewardsSummaryDetailLayout;
    private CardView mRewardsTipLayout;
    private LinearLayout mBtnTip;
    private ImageView mImgTip;
    private TextView mTextTip;
    private LinearLayout mBtnSummary;
    private ImageView mImgSummary;
    private TextView mTextSummary;
    private TextView mBtnAddFunds;
    private SwitchCompat mSwitchAutoContribute;

    private Timer mBalanceUpdater;

    private Timer mPublisherFetcher;
    private int mPublisherFetchesCount;
    private boolean mPublisherExist;

    private String mCurrentNotificationId;

    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;

    private int mCurrentTabId;
    private boolean mAutoContributeEnabled;
    private TextView mPublisherAttention;

    private BraveRewardsExternalWallet mExternalWallet;

    private View mNotificationLayout;
    private boolean mClaimInProcess;

    public BraveRewardsPanel(View anchorView) {
        mCurrentNotificationId = "";
        mPublisherExist = false;
        mPublisherFetchesCount = 0;
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

                if (mBraveRewardsNativeWorker != null) {
                    mBraveRewardsNativeWorker.RemoveObserver(BraveRewardsPanel.this);
                }

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
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.AddObserver(this);
        }
        mBalanceUpdater = new Timer();
        createUpdateBalanceTask();
        setUpViews();
    }

    private void setUpViews() {
        LayoutInflater inflater = (LayoutInflater) mAnchorView.getContext().getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        mPopupView = (ViewGroup) inflater.inflate(R.layout.brave_rewards_panel_layout, null);

        ImageView btnRewardsSettings = mPopupView.findViewById(R.id.btn_rewards_settings);
        btnRewardsSettings.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mBraveActivity.openNewOrSelectExistingTab(BraveActivity.REWARDS_SETTINGS_URL);
                dismiss();
            }
        }));

        mRewardsSummaryDetailLayout =
                mPopupView.findViewById(R.id.brave_rewards_panel_summary_layout_id);
        mRewardsTipLayout = mPopupView.findViewById(R.id.brave_rewards_panel_tip_layout_id);

        TextView btnSendTip = mPopupView.findViewById(R.id.btn_send_tip);
        btnSendTip.setOnClickListener(view -> {
            // if (mTippingInProgress) {
            //             return;
            //         }
            //         mTippingInProgress = true;

            Intent intent = new Intent(
                    ContextUtils.getApplicationContext(), BraveRewardsSiteBannerActivity.class);
            intent.putExtra(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, mCurrentTabId);
            mActivity.startActivityForResult(intent, BraveActivity.SITE_BANNER_REQUEST_CODE);

            // BraveRewardsPanelPopup is not an Activity and onActivityResult is not available
            // to dismiss mTippingInProgress. Post a delayed task to flip a mTippingInProgress flag.
            // mHandler.postDelayed(new Runnable() {
            //     @Override
            //     public void run() {
            //         mTippingInProgress = false;
            //     }
            // }, CLICK_DISABLE_INTERVAL);
            // }
        });

        mBtnAddFunds = mPopupView.findViewById(R.id.btn_add_funds);
        mBtnAddFunds.setOnClickListener(view -> {
            dismiss();
            mBraveActivity.openNewOrSelectExistingTab(mExternalWallet.getAddUrl());
        });

        mBtnTip = mPopupView.findViewById(R.id.tip_btn);
        mImgTip = mPopupView.findViewById(R.id.tip_img);
        mTextTip = mPopupView.findViewById(R.id.tip_text);
        mBtnTip.setOnClickListener(view -> { showTipSection(); });

        mBtnSummary = mPopupView.findViewById(R.id.summary_btn);
        mImgSummary = mPopupView.findViewById(R.id.summary_img);
        mTextSummary = mPopupView.findViewById(R.id.summary_text);
        mBtnSummary.setOnClickListener(view -> { showSummarySection(); });

        mSwitchAutoContribute = mPopupView.findViewById(R.id.auto_contribution_switch);
        mSwitchAutoContribute.setOnCheckedChangeListener(autoContributeSwitchListener);

        showSummarySection();
        mBtnTip.setEnabled(false);

        String monthName = (String) android.text.format.DateFormat.format("MMM", new Date());
        int lastDate = Calendar.getInstance().getActualMaximum(Calendar.DAY_OF_MONTH);
        int currentYear = Calendar.getInstance().get(Calendar.YEAR);

        String adsMonthlyStatement = new StringBuilder(monthName)
                                             .append(" 1 - ")
                                             .append(monthName)
                                             .append(" ")
                                             .append(lastDate)
                                             .toString();
        String monthYear = new StringBuilder(monthName).append(" ").append(currentYear).toString();

        TextView adsMonthlyStatementText = mPopupView.findViewById(R.id.ads_monthly_statement_text);
        adsMonthlyStatementText.setText(adsMonthlyStatement);

        TextView monthYearText = mPopupView.findViewById(R.id.month_year_text);
        monthYearText.setText(monthYear);

        mPopupWindow.setContentView(mPopupView);
    }

    private void showSummarySection() {
        mTextTip.setTextColor(Color.parseColor("#868E96"));
        mImgTip.setColorFilter(
                new PorterDuffColorFilter(Color.parseColor("#868E96"), PorterDuff.Mode.SRC_IN));

        mTextSummary.setTextColor(Color.parseColor("#4C54D2"));
        mImgSummary.setColorFilter(
                new PorterDuffColorFilter(Color.parseColor("#4C54D2"), PorterDuff.Mode.SRC_IN));

        mRewardsSummaryDetailLayout.setVisibility(View.VISIBLE);
        mRewardsTipLayout.setVisibility(View.GONE);
    }

    private void showTipSection() {
        mTextTip.setTextColor(Color.parseColor("#4C54D2"));
        mImgTip.setColorFilter(
                new PorterDuffColorFilter(Color.parseColor("#4C54D2"), PorterDuff.Mode.SRC_IN));

        mTextSummary.setTextColor(Color.parseColor("#868E96"));
        mImgSummary.setColorFilter(
                new PorterDuffColorFilter(Color.parseColor("#868E96"), PorterDuff.Mode.SRC_IN));
        mRewardsSummaryDetailLayout.setVisibility(View.GONE);
        mRewardsTipLayout.setVisibility(View.VISIBLE);
    }

    private void setNotificationsControls() {
        // Check for notifications

        mNotificationLayout =
                mPopupView.findViewById(R.id.brave_rewards_panel_notification_layout_id);

        Log.e(TAG, "setNotificationsControls");
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.GetAllNotifications();
        }

        ImageView btnCloseNotification = mPopupView.findViewById(R.id.btn_close_notification);
        if (btnCloseNotification != null) {
            btnCloseNotification.setOnClickListener((new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mCurrentNotificationId.isEmpty()) {
                        assert false;

                        return;
                    }
                    if (mBraveRewardsNativeWorker != null) {
                        mBraveRewardsNativeWorker.DeleteNotification(mCurrentNotificationId);
                    }
                }
            }));
        }
    }

    /**
     * Validates if a notification can be processed and has an expected
     * number of arguments
     */
    private boolean isValidNotificationType(int type, int argsNum) {
        Log.e(TAG, "isValidNotificationType : " + type);
        boolean valid = false;

        switch (type) {
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_AUTO_CONTRIBUTE:
                valid = (argsNum >= 4) ? true : false;
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_VERIFIED_PUBLISHER:
                valid = (argsNum >= 1) ? true : false;
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT:
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT_ADS:
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS:
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_BACKUP_WALLET:
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_TIPS_PROCESSED:
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_ADS_ONBOARDING:
            case REWARDS_NOTIFICATION_NO_INTERNET:
            case REWARDS_PROMOTION_CLAIM_ERROR:
                valid = true;
                break;
            default:
                valid = false;
        }
        Log.i(TAG, "IsValidNotificationType: type %d argnum %d ", type, argsNum);
        return valid;
    }

    private void showNotification(String id, int type, long timestamp, String[] args) {
        Log.e(TAG, "showNotification");
        if (mBraveRewardsNativeWorker == null) {
            return;
        }

        // don't process unknown notifications
        if (!isValidNotificationType(type, args.length) && mBraveRewardsNativeWorker != null) {
            Log.e(TAG, "showNotification 1");
            mBraveRewardsNativeWorker.DeleteNotification(id);
            return;
        }

        mCurrentNotificationId = id;
        TextView notificationDayText = mPopupView.findViewById(R.id.notification_day_text);
        String currentDay = (String) android.text.format.DateFormat.format("MMM dd", new Date());
        notificationDayText.setText(currentDay);

        TextView notificationTitleText = mPopupView.findViewById(R.id.notification_title_text);
        TextView notificationSubtitleText =
                mPopupView.findViewById(R.id.notification_subtitle_text);
        TextView actionNotificationButton = mPopupView.findViewById(R.id.btn_action_notification);

        // LinearLayout hl = (LinearLayout)root.findViewById(R.id.header_layout);
        // hl.setBackgroundResource(R.drawable.notification_header);
        // GridLayout gl = (GridLayout)root.findViewById(R.id.wallet_info_gridlayout);
        // gl.setVisibility(View.GONE);
        // TimeZone utc = TimeZone.getTimeZone("UTC");
        // Calendar calTime = Calendar.getInstance(utc);
        // calTime.setTimeInMillis(timestamp * 1000);
        // String currentMonth = BraveRewardsHelper.getCurrentMonth(calTime,
        //                       root.getResources(), false);
        // String currentDay = Integer.toString(calTime.get(Calendar.DAY_OF_MONTH));
        // String notificationTime = currentMonth + " " + currentDay;
        String title = "";
        String description = "";
        // Button btClaimOk = (Button)root.findViewById(R.id.br_claim_button);
        View claimProgress = mPopupView.findViewById(R.id.claim_progress);

        // hide or show 'Claim/OK' button if Grant claim is (not) in process
        mClaimInProcess = mBraveRewardsNativeWorker.IsGrantClaimInProcess();
        if (mClaimInProcess) {
            BraveRewardsHelper.crossfade(actionNotificationButton, claimProgress, View.GONE, 1f,
                    BraveRewardsHelper.CROSS_FADE_DURATION);
        } else {
            actionNotificationButton.setEnabled(true);
            BraveRewardsHelper.crossfade(claimProgress, actionNotificationButton, View.GONE, 1f,
                    BraveRewardsHelper.CROSS_FADE_DURATION);
        }

        // TextView notificationClose = (TextView)root.findViewById(R.id.br_notification_close);
        // notificationClose.setVisibility(View.VISIBLE);
        // ImageView notification_icon = (ImageView)root.findViewById(R.id.br_notification_icon);
        // LinearLayout nit = (LinearLayout)root.findViewById(R.id.notification_image_text);
        // nit.setOrientation(LinearLayout.VERTICAL);
        // LinearLayout.LayoutParams params = (LinearLayout.LayoutParams)nit.getLayoutParams();
        // params.setMargins(params.leftMargin, 5, params.rightMargin, params.bottomMargin);
        // nit.setLayoutParams(params);
        // TextView tv = (TextView)root.findViewById(R.id.br_notification_description);
        // tv.setGravity(Gravity.CENTER);

        // LinearLayout ll = (LinearLayout)root.findViewById(R.id.notification_info_layout);
        // ll.setVisibility(View.VISIBLE);

        // TODO other types of notifications
        switch (type) {
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_AUTO_CONTRIBUTE:
                // TODO find the case where it is used
                // notification_icon.setImageResource(R.drawable.icon_validated_notification);
                Log.e(TAG, "showNotification 2");
                String result = args[1];
                switch (result) {
                    case AUTO_CONTRIBUTE_SUCCESS:
                        actionNotificationButton.setText(
                                mPopupView.getResources().getString(R.string.ok));
                        title = mPopupView.getResources().getString(
                                R.string.brave_ui_rewards_contribute);
                        // notification_icon.setImageResource(R.drawable.contribute_icon);
                        // hl.setBackgroundResource(R.drawable.notification_header_normal);

                        double value = 0;
                        String valueString = "";
                        String[] splittedValue = args[3].split("\\.", 0);
                        // 18 digits is a probi min digits count
                        if (splittedValue.length != 0 && splittedValue[0].length() >= 18) {
                            value = BraveRewardsHelper.probiToDouble(args[3]);
                            valueString = Double.isNaN(value)
                                    ? ERROR_CONVERT_PROBI
                                    : String.format(Locale.getDefault(), "%.3f", value);
                        } else {
                            value = Double.parseDouble(args[3]);
                            valueString = String.format(Locale.getDefault(), "%.3f", value);
                        }

                        description = String.format(
                                mPopupView.getResources().getString(
                                        R.string.brave_ui_rewards_contribute_description),
                                valueString);
                        break;
                    case AUTO_CONTRIBUTE_NOT_ENOUGH_FUNDS:
                        Log.e(TAG, "showNotification 3");
                        title = "";
                        // notification_icon.setImageResource(R.drawable.icon_warning_notification);
                        // hl.setBackgroundResource(R.drawable.notification_header_warning);
                        description = mPopupView.getResources().getString(
                                R.string.brave_ui_notification_desc_no_funds);
                        break;
                    case AUTO_CONTRIBUTE_TIPPING_ERROR:
                        Log.e(TAG, "showNotification 4");
                        title = "";
                        // notification_icon.setImageResource(R.drawable.icon_error_notification);
                        // hl.setBackgroundResource(R.drawable.notification_header_error);
                        description = mPopupView.getResources().getString(
                                R.string.brave_ui_notification_desc_tip_error);
                        break;
                    default:
                        title = "";
                        // notification_icon.setImageResource(R.drawable.icon_error_notification);
                        // hl.setBackgroundResource(R.drawable.notification_header_error);
                        description = mPopupView.getResources().getString(
                                R.string.brave_ui_notification_desc_contr_error);
                }
                if (title.isEmpty()) {
                    actionNotificationButton.setVisibility(View.GONE);
                    // nit.setOrientation(LinearLayout.HORIZONTAL);
                    // params.setMargins(params.leftMargin, 35, params.rightMargin,
                    // params.bottomMargin); nit.setLayoutParams(params);
                    // tv.setGravity(Gravity.START);
                }
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT:
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT_ADS:
                Log.e(TAG, "showNotification 5");
                actionNotificationButton.setText(
                        mPopupView.getResources().getString(R.string.brave_ui_claim));

                // int grant_icon_id = (BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT == type
                // ) ?
                //                     R.drawable.grant_icon : R.drawable.notification_icon;
                // notification_icon.setImageResource(grant_icon_id);

                title = (BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT == type)
                        ? mPopupView.getResources().getString(R.string.brave_ui_new_token_grant)
                        : mPopupView.getResources().getString(
                                R.string.notification_category_group_brave_ads);

                description = (BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT == type)
                        ? String.format(
                                mPopupView.getResources().getString(R.string.brave_ui_new_grant),
                                mPopupView.getResources().getString(R.string.token))
                        : mPopupView.getResources().getString(R.string.brave_ads_you_earned);
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS:
                Log.e(TAG, "showNotification 6");
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                // notification_icon.setImageResource(R.drawable.notification_icon);
                title = mPopupView.getResources().getString(
                        R.string.brave_ui_insufficient_funds_msg);
                description = mPopupView.getResources().getString(
                        R.string.brave_ui_insufficient_funds_desc);
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_BACKUP_WALLET:
                Log.e(TAG, "showNotification 7");
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                // notification_icon.setImageResource(R.drawable.notification_icon);
                title = mPopupView.getResources().getString(R.string.brave_ui_backup_wallet_msg);
                description =
                        mPopupView.getResources().getString(R.string.brave_ui_backup_wallet_desc);
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_TIPS_PROCESSED:
                Log.e(TAG, "showNotification 8");
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                title = mPopupView.getResources().getString(R.string.brave_ui_contribution_tips);
                description = mPopupView.getResources().getString(
                        R.string.brave_ui_tips_processed_notification);
                // notification_icon.setImageResource(R.drawable.contribute_icon);
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_ADS_ONBOARDING:
                Log.e(TAG, "showNotification 9");
                actionNotificationButton.setText(
                        mPopupView.getResources().getString(R.string.brave_ui_turn_on_ads));
                title = mPopupView.getResources().getString(
                        R.string.brave_ui_brave_ads_launch_title);
                description = ""; // TODO verify the text
                // notification_icon.setImageResource(R.drawable.notification_icon);
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_VERIFIED_PUBLISHER:
                Log.e(TAG, "showNotification 10");
                String pubName = args[0];
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                title = mPopupView.getResources().getString(
                        R.string.brave_ui_pending_contribution_title);
                description = mPopupView.getResources().getString(
                        R.string.brave_ui_verified_publisher_notification, pubName);
                // notification_icon.setImageResource(R.drawable.contribute_icon);
                break;
            case REWARDS_NOTIFICATION_NO_INTERNET:
                Log.e(TAG, "showNotification 11");
                title = "";
                // notification_icon.setImageResource(R.drawable.icon_error_notification);
                // hl.setBackgroundResource(R.drawable.notification_header_error);
                description = "<b>"
                        + mPopupView.getResources().getString(R.string.brave_rewards_local_uh_oh)
                        + "</b> "
                        + mPopupView.getResources().getString(
                                R.string.brave_rewards_local_server_not_responding);
                actionNotificationButton.setVisibility(View.GONE);
                // notificationClose.setVisibility(View.GONE);
                // nit.setOrientation(LinearLayout.HORIZONTAL);
                // params.setMargins(params.leftMargin, 180, params.rightMargin,
                // params.bottomMargin); nit.setLayoutParams(params); tv.setGravity(Gravity.START);
                break;
            case REWARDS_PROMOTION_CLAIM_ERROR:
                Log.e(TAG, "showNotification 12");
                title = "";
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                description = "<b>"
                        + mPopupView.getResources().getString(
                                R.string.brave_rewards_local_general_grant_error_title)
                        + "</b>";
                // notification_icon.setImageResource(R.drawable.coin_stack);
                // hl.setBackgroundResource(R.drawable.notification_header_error);
                // notificationClose.setVisibility(View.GONE);
                // nit.setOrientation(LinearLayout.HORIZONTAL);
                // params.setMargins(params.leftMargin, 180, params.rightMargin,
                // params.bottomMargin); nit.setLayoutParams(params); tv.setGravity(Gravity.START);
                break;
            default:
                Log.e(TAG, "This notification type is either invalid or not handled yet: " + type);
                assert false;
                return;
        }
        // String stringToInsert = (title.isEmpty() ? "" : ("<b>" + title + "</b>" + " | ")) +
        // description +
        //                         (title.isEmpty() ? "" : ("  <font color=#a9aab4>" +
        //                         notificationTime + "</font>"));
        // Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(stringToInsert);
        // tv.setText(toInsert);
        notificationTitleText.setText(title);
        notificationSubtitleText.setText(description);

        if (mNotificationLayout != null) {
            Log.e(TAG, "mNotificationLayout visible");
            mNotificationLayout.setVisibility(View.VISIBLE);
        }

        setNotificationButtoClickListener();
    }

    // TODO needs to be changed
    private void setNotificationButtoClickListener() {
        Log.e(TAG, "setNotificationButtoClickListener");
        TextView actionNotificationButton = mPopupView.findViewById(R.id.btn_action_notification);
        String strAction = (actionNotificationButton != null && mBraveRewardsNativeWorker != null)
                ? actionNotificationButton.getText().toString()
                : "";
        if (strAction.equals(mPopupView.getResources().getString(R.string.ok))) {
            actionNotificationButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Log.e(TAG, "setNotificationButtoClickListener 1");
                    // This is custom Android notification and thus should be dismissed intead of
                    // deleting
                    if (mCurrentNotificationId.equals(REWARDS_PROMOTION_CLAIM_ERROR_ID)) {
                        dismissNotification(mCurrentNotificationId);
                        return;
                    }
                    mBraveRewardsNativeWorker.DeleteNotification(mCurrentNotificationId);
                }
            });
        } else if (strAction.equals(mPopupView.getResources().getString(R.string.brave_ui_claim))) {
            actionNotificationButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Log.e(TAG, "setNotificationButtoClickListener 2");
                    // disable and hide CLAIM button
                    // actionNotificationButton.setEnabled(false);

                    if (mClaimInProcess || mCurrentNotificationId.isEmpty()) {
                        return;
                    }

                    int promIdSeparator =
                            mCurrentNotificationId.lastIndexOf(NOTIFICATION_PROMID_SEPARATOR);
                    String promId = "";
                    if (-1 != promIdSeparator) {
                        promId = mCurrentNotificationId.substring(promIdSeparator + 1);
                    }
                    if (promId.isEmpty()) {
                        return;
                    }

                    mClaimInProcess = true;

                    View fadein = mPopupView.findViewById(R.id.claim_progress);
                    BraveRewardsHelper.crossfade(actionNotificationButton, fadein, View.GONE, 1f,
                            BraveRewardsHelper.CROSS_FADE_DURATION);

                    mBraveRewardsNativeWorker.GetGrant(promId);
                    // walletDetailsReceived = false; //re-read wallet status
                    // EnableWalletDetails(false);
                }
            });
        } else if (strAction.equals(
                           mPopupView.getResources().getString(R.string.brave_ui_turn_on_ads))) {
            actionNotificationButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Log.e(TAG, "setNotificationButtoClickListener 3");
                    mBraveRewardsNativeWorker.DeleteNotification(mCurrentNotificationId);
                    assert (BraveReflectionUtil.EqualTypes(
                            mActivity.getClass(), BraveActivity.class));
                    BraveActivity.class.cast(mActivity).openNewOrSelectExistingTab(
                            BraveActivity.REWARDS_SETTINGS_URL);
                    dismiss();
                }
            });
        }
    }

    private void dismissNotification(String id) {
        Log.e(TAG, "dismissNotification");
        if (!mCurrentNotificationId.equals(id)) {
            return;
        }
        hideNotifications();
        mCurrentNotificationId = "";
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.GetAllNotifications();
        }
    }

    private void hideNotifications() {
        Log.e(TAG, "hideNotifications");
        if (mNotificationLayout != null) {
            mNotificationLayout.setVisibility(View.GONE);
        }
    }

    @Override
    public void OnClaimPromotion(int responseCode) {
        if (responseCode != BraveRewardsNativeWorker.LEDGER_OK) {
            String args[] = {};
            showNotification(
                    REWARDS_PROMOTION_CLAIM_ERROR_ID, REWARDS_PROMOTION_CLAIM_ERROR, 0, args);
        }
        // TODO add logic for balance refresh
        else if (responseCode == BraveRewardsNativeWorker.LEDGER_OK) {
            Log.e(TAG, "BraveRewardsNativeWorker.LEDGER_OK");
            // mBraveRewardsNativeWorker.GetRewardsParameters();
            // mBraveRewardsNativeWorker.GetExternalWallet();
            // mBraveRewardsNativeWorker.getAdsAccountStatement();
            // mBraveRewardsNativeWorker.GetCurrentBalanceReport();
        }
    }

    @Override
    public void OnGrantFinish(int result) {
        Log.e(TAG, "OnGrantFinish");
        mBraveRewardsNativeWorker.GetAllNotifications();
        mBraveRewardsNativeWorker.GetRewardsParameters();
        mBraveRewardsNativeWorker.GetExternalWallet();
        mBraveRewardsNativeWorker.getAdsAccountStatement();
        mBraveRewardsNativeWorker.GetCurrentBalanceReport();
    }

    OnCheckedChangeListener autoContributeSwitchListener = new OnCheckedChangeListener() {
        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            mBraveRewardsNativeWorker.IncludeInAutoContribution(mCurrentTabId, !isChecked);
        }
    };

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
        mBraveRewardsNativeWorker.getInstance().StartProcess();
    }

    @Override
    public void OnStartProcess() {
        requestPublisherInfo();
        mBraveRewardsNativeWorker.GetRewardsParameters();
        mBraveRewardsNativeWorker.GetExternalWallet();
        mBraveRewardsNativeWorker.getAdsAccountStatement();
        mBraveRewardsNativeWorker.GetCurrentBalanceReport();
        setNotificationsControls();
        // if (root != null && PackageUtils.isFirstInstall(mActivity)
        //         && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)) {
        //     if (BraveRewardsHelper.getBraveRewardsAppOpenCount() == 0
        //             && BraveRewardsHelper.shouldShowBraveRewardsOnboardingModal()
        //             && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(
        //                     Profile.getLastUsedRegularProfile())) {
        //         showBraveRewardsOnboardingModal(root);
        //         BraveRewardsHelper.updateBraveRewardsAppOpenCount();
        //         BraveRewardsHelper.setShowBraveRewardsOnboardingModal(false);
        //     } else if (SharedPreferencesManager.getInstance().readInt(
        //                        BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
        //                     > BraveRewardsHelper.getBraveRewardsAppOpenCount()
        //             && BraveRewardsHelper.shouldShowMiniOnboardingModal()) {
        //         if
        //         (BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedRegularProfile()))
        //         {
        //             showBraveRewardsWelcomeLayout(root);
        //         } else {
        //             showBraveRewardsOptInLayout(root);
        //         }
        //         BraveRewardsHelper.setShowMiniOnboardingModal(false);
        //     }
        // }
    }

    @Override
    public void OnGetCurrentBalanceReport(double[] report) {
        Log.e(TAG, "OnGetCurrentBalanceReport : " + report);
        if (report == null) {
            return;
        }
        String batText = BraveRewardsHelper.BAT_TEXT;
        for (int i = 0; i < report.length; i++) {
            TextView tvTitle = null;
            TextView tv = null;
            TextView tvUSD = null;
            String text = "";
            String textUSD = "";

            double probiDouble = report[i];
            boolean hideControls = (probiDouble == 0);
            String value = Double.isNaN(probiDouble)
                    ? "0.000" + batText
                    : String.format(Locale.getDefault(), "%.3f", probiDouble);

            String usdValue = "0.00 USD";
            if (!Double.isNaN(probiDouble)) {
                double usdValueDouble = probiDouble * mBraveRewardsNativeWorker.GetWalletRate();
                usdValue = String.format(Locale.getDefault(), "%.2f USD", usdValueDouble);
            }

            switch (i) {
                case BALANCE_REPORT_GRANTS:
                    // tvTitle = (TextView)root.findViewById(R.id.br_grants_claimed_title);
                    // tvTitle.setText(BraveRewardsPanelPopup.this.root.getResources().getString(
                    //         R.string.brave_ui_token_grant_claimed));
                    tv = mPopupView.findViewById(R.id.total_grants_claimed_bat_text);
                    tvUSD = mPopupView.findViewById(R.id.total_grants_claimed_usd_text);
                    text = "<font color=#8E2995>" + value + "</font><font color=#000000> " + batText
                            + "</font>";
                    textUSD = usdValue;
                    break;
                case BALANCE_REPORT_EARNING_FROM_ADS:
                    // tvTitle = (TextView)root.findViewById(R.id.br_earnings_ads_title);
                    tv = mPopupView.findViewById(R.id.rewards_from_ads_bat_text);
                    tvUSD = mPopupView.findViewById(R.id.rewards_from_ads_usd_text);
                    text = "<font color=#8E2995>" + value + "</font><font color=#000000> " + batText
                            + "</font>";
                    textUSD = usdValue;
                    break;
                case BALANCE_REPORT_AUTO_CONTRIBUTE:
                    // tvTitle = (TextView)root.findViewById(R.id.br_auto_contribute_title);
                    tv = mPopupView.findViewById(R.id.auto_contribute_bat_text);
                    tvUSD = mPopupView.findViewById(R.id.auto_contribute_usd_text);
                    text = "<font color=#6537AD>" + value + "</font><font color=#000000> " + batText
                            + "</font>";
                    textUSD = usdValue;
                    break;
                case BALANCE_REPORT_ONE_TIME_DONATION:
                    // tvTitle = (TextView)root.findViewById(R.id.br_recurring_donation_title);
                    tv = mPopupView.findViewById(R.id.one_time_tip_bat_text);
                    tvUSD = mPopupView.findViewById(R.id.one_time_tip_usd_text);
                    text = "<font color=#392DD1>" + value + "</font><font color=#000000> " + batText
                            + "</font>";
                    textUSD = usdValue;
                    break;
                case BALANCE_REPORT_RECURRING_DONATION:
                    // tvTitle = (TextView)root.findViewById(R.id.br_one_time_donation_title);
                    tv = mPopupView.findViewById(R.id.monthly_tips_bat_text);
                    tvUSD = mPopupView.findViewById(R.id.monthly_tips_usd_text);
                    text = "<font color=#392DD1>" + value + "</font><font color=#000000> " + batText
                            + "</font>";
                    textUSD = usdValue;
                    break;
            }
            Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(text);
            tv.setText(toInsert);
            tvUSD.setText(textUSD);
            // if (tv != null && tvUSD != null &&
            //         !text.isEmpty() && !textUSD.isEmpty()) {
            //     // tvTitle.setVisibility(hideControls ? View.GONE : View.VISIBLE);
            //     tv.setVisibility(hideControls ? View.GONE : View.VISIBLE);
            //     tvUSD.setVisibility(hideControls ? View.GONE : View.VISIBLE);
            //     Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(text);
            //     tv.setText(toInsert);
            //     tvUSD.setText(textUSD);
            // }
        }

        mBraveRewardsNativeWorker.GetPendingContributionsTotal();
    }

    @Override
    public void OnGetPendingContributionsTotal(double amount) {
        // if (amount > 0.0) {
        //     String non_verified_summary =
        //             String.format(
        //                     root.getResources().getString(R.string.brave_ui_reserved_amount_text),
        //                     String.format(Locale.getDefault(), "%.3f", amount))
        //             + " <font color=#73CBFF>" +
        //             root.getResources().getString(R.string.learn_more)
        //             + ".</font>";
        //     Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(non_verified_summary);
        //     tvPublisherNotVerifiedSummary.setText(toInsert);
        //     tvPublisherNotVerifiedSummary.setVisibility(View.VISIBLE);
        // } else {
        //     tvPublisherNotVerifiedSummary.setVisibility(View.GONE);
        // }
    }

    @Override
    public void OnGetAdsAccountStatement(boolean success, double nextPaymentDate,
            int adsReceivedThisMonth, double earningsThisMonth, double earningsLastMonth) {
        DecimalFormat df = new DecimalFormat("#.###");
        df.setRoundingMode(RoundingMode.FLOOR);
        df.setMinimumFractionDigits(3);
        TextView batBalanceAdsText = mPopupView.findViewById(R.id.bat_balance_ads_text);
        batBalanceAdsText.setText(df.format(earningsThisMonth));
        double usdValue = earningsThisMonth * mBraveRewardsNativeWorker.GetWalletRate();
        String usdText =
                String.format(mPopupView.getResources().getString(R.string.brave_ads_statement_usd),
                        String.format(Locale.getDefault(), "%.2f", usdValue));
        TextView usdBalanceAdsText = mPopupView.findViewById(R.id.usd_balance_ads_text);
        usdBalanceAdsText.setText(usdText);
    }

    @Override
    public void OnRewardsParameters(int errorCode) {
        Log.e(TAG, "OnRewardsParameters");
        // boolean formerWalletDetailsReceived = walletDetailsReceived;
        if (errorCode == BraveRewardsNativeWorker.LEDGER_OK) {
            // DismissNotification(REWARDS_NOTIFICATION_NO_INTERNET_ID);
            Log.e(TAG, "LEDGER_OK");
            if (mBraveRewardsNativeWorker != null) {
                Log.e(TAG, "mBraveRewardsNativeWorker != null");
                BraveRewardsBalance walletBalanceObject =
                        mBraveRewardsNativeWorker.GetWalletBalance();
                double walletBalance = 0;
                if (walletBalanceObject != null) {
                    walletBalance = walletBalanceObject.getTotal();
                }
                Log.e(TAG, "Wallet balance : " + walletBalance);
                // if (walletBalance > 0 && braveRewardsWelcomeView != null) {
                //     braveRewardsWelcomeView.setVisibility(View.GONE);
                // }

                DecimalFormat df = new DecimalFormat("#.###");
                df.setRoundingMode(RoundingMode.FLOOR);
                df.setMinimumFractionDigits(3);
                TextView batBalanceText = mPopupView.findViewById(R.id.bat_balance_text);
                batBalanceText.setText(df.format(walletBalance));
                double usdValue = walletBalance * mBraveRewardsNativeWorker.GetWalletRate();
                String usdText =
                        String.format(mPopupView.getResources().getString(R.string.brave_ui_usd),
                                String.format(Locale.getDefault(), "%.2f", usdValue));
                Log.e(TAG, "USd amount : " + usdText);
                TextView usdBalanceText = mPopupView.findViewById(R.id.usd_balance_text);
                usdBalanceText.setText(usdText);

                // Button btnVerifyWallet = (Button) root.findViewById(R.id.btn_verify_wallet);
                // btnVerifyWallet.setBackgroundResource(R.drawable.wallet_verify_button);
                // if (mExternalWallet != null
                //         && mExternalWallet.getType().equals(BraveWalletProvider.BITFLYER)) {
                //     btnVerifyWallet.setVisibility(View.INVISIBLE);
                // }
            }
            // walletDetailsReceived = true;
        } else if (errorCode == BraveRewardsNativeWorker.LEDGER_ERROR) { // No Internet connection
            String args[] = {};
            Log.e(TAG, "LEDGER_ERROR");
            // ShowNotification(REWARDS_NOTIFICATION_NO_INTERNET_ID,
            // REWARDS_NOTIFICATION_NO_INTERNET, 0, args);
        }
        Log.e(TAG, "Outside check");
        // else {
        //     walletDetailsReceived = false;
        // }

        // if (formerWalletDetailsReceived != walletDetailsReceived) {
        //     EnableWalletDetails(walletDetailsReceived);
        // }
    }

    @Override
    public void OnNotificationAdded(String id, int type, long timestamp, String[] args) {
        // Do nothing here as we will receive the most recent notification
        // in OnGetLatestNotification
    }

    @Override
    public void OnNotificationsCount(int count) {}

    @Override
    public void OnGetLatestNotification(String id, int type, long timestamp, String[] args) {
        if (type == BraveRewardsNativeWorker.REWARDS_NOTIFICATION_BACKUP_WALLET) {
            if (mBraveRewardsNativeWorker != null) {
                mBraveRewardsNativeWorker.DeleteNotification(id);
                mBraveRewardsNativeWorker.GetAllNotifications();
            }
            return;
        }

        // This is to make sure that user saw promotion error message before showing the
        // rest of messages
        if (!mCurrentNotificationId.equals(REWARDS_PROMOTION_CLAIM_ERROR_ID)) {
            showNotification(id, type, timestamp, args);
        }
    }

    @Override
    public void OnNotificationDeleted(String id) {
        dismissNotification(id);
    }

    @Override
    public void OnGetExternalWallet(int errorCode, String externalWallet) {
        Log.e(TAG, "OnGetExternalWallet");
        int walletStatus = BraveRewardsExternalWallet.NOT_CONNECTED;
        if (!TextUtils.isEmpty(externalWallet)) {
            try {
                mExternalWallet = new BraveRewardsExternalWallet(externalWallet);
                walletStatus = mExternalWallet.getStatus();
            } catch (JSONException e) {
                Log.e(TAG, "Error parsing external wallet status");
                mExternalWallet = null;
            }
        }
        setVerifyWalletButton(walletStatus);
    }

    private void setVerifyWalletButton(@WalletStatus final int status) {
        Log.e(TAG, "Status : " + status);
        TextView btnVerifyWallet = mPopupView.findViewById(R.id.btn_verify_wallet);
        SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor editor = sharedPref.edit();
        int rightDrawable = 0;
        int leftDrawable = 0;
        int text = 0;

        switch (status) {
            case BraveRewardsExternalWallet.NOT_CONNECTED:
                rightDrawable = R.drawable.disclosure;
                text = R.string.brave_ui_wallet_button_unverified;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                Log.e(TAG, "BraveRewardsExternalWallet.NOT_CONNECTED");
                break;
            case BraveRewardsExternalWallet.CONNECTED:
                rightDrawable = R.drawable.verified_disclosure;
                text = R.string.brave_ui_wallet_button_unverified;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                break;
            case BraveRewardsExternalWallet.PENDING:
                editor.putBoolean(PREF_VERIFY_WALLET_ENABLE, true);
                editor.apply();

                rightDrawable = R.drawable.verified_disclosure;
                text = R.string.brave_ui_wallet_button_unverified;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                break;
            case BraveRewardsExternalWallet.VERIFIED:
                editor.putBoolean(PREF_VERIFY_WALLET_ENABLE, true);
                editor.apply();

                leftDrawable = R.drawable.uphold_white;
                rightDrawable = R.drawable.verified_disclosure;
                text = R.string.brave_ui_wallet_button_verified;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(
                        leftDrawable, 0, rightDrawable, 0);
                btnVerifyWallet.setBackgroundColor(Color.TRANSPARENT);

                // show Add funds button
                if (mBtnAddFunds != null) {
                    mBtnAddFunds.setVisibility(View.VISIBLE);
                }
                break;
            case BraveRewardsExternalWallet.DISCONNECTED_NOT_VERIFIED:
            case BraveRewardsExternalWallet.DISCONNECTED_VERIFIED:
                leftDrawable = R.drawable.uphold_white;
                text = R.string.brave_ui_wallet_button_disconnected;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(leftDrawable, 0, 0, 0);
                btnVerifyWallet.setBackgroundDrawable(ResourcesCompat.getDrawable(
                        ContextUtils.getApplicationContext().getResources(),
                        R.drawable.wallet_disconnected_button, /* theme= */ null));
                break;
            default:
                Log.e(TAG, "Unexpected external wallet status");
                return;
        }

        // tvYourWalletTitle.setVisibility(View.GONE);
        btnVerifyWallet.setVisibility(View.VISIBLE);
        btnVerifyWallet.setText(mPopupView.getResources().getString(text));
        Log.e(TAG, "before onClick");
        setVerifyWalletButtonClickEvent(btnVerifyWallet, status);

        // Update add funds button based on status
        if (status != BraveRewardsExternalWallet.VERIFIED) {
            mBtnAddFunds.setEnabled(false);
            return;
        }
    }

    private void setVerifyWalletButtonClickEvent(
            View btnVerifyWallet, @WalletStatus final int status) {
        btnVerifyWallet.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.e(TAG, "onClick");
                BraveRewardsBalance walletBalanceObject =
                        mBraveRewardsNativeWorker.GetWalletBalance();
                double walletBalance = 0;
                if (walletBalanceObject != null) {
                    walletBalance = walletBalanceObject.getTotal();
                }

                switch (status) {
                    case BraveRewardsExternalWallet.NOT_CONNECTED:
                    case BraveRewardsExternalWallet.CONNECTED:
                    case BraveRewardsExternalWallet.PENDING:
                    case BraveRewardsExternalWallet.VERIFIED:
                        if (walletBalance < WALLET_BALANCE_LIMIT
                                && mExternalWallet.getType().equals(BraveWalletProvider.UPHOLD)
                                && !isVerifyWalletEnabled()) {
                            showUpholdLoginPopupWindow(btnVerifyWallet);
                        } else {
                            int requestCode = (status == BraveRewardsExternalWallet.NOT_CONNECTED)
                                    ? BraveActivity.VERIFY_WALLET_ACTIVITY_REQUEST_CODE
                                    : BraveActivity.USER_WALLET_ACTIVITY_REQUEST_CODE;
                            Intent intent = BuildVerifyWalletActivityIntent(status);
                            mActivity.startActivityForResult(intent, requestCode);
                        }
                        break;
                    case BraveRewardsExternalWallet.DISCONNECTED_NOT_VERIFIED:
                    case BraveRewardsExternalWallet.DISCONNECTED_VERIFIED:
                        if (walletBalance < WALLET_BALANCE_LIMIT
                                && mExternalWallet.getType().equals(BraveWalletProvider.UPHOLD)
                                && !isVerifyWalletEnabled()) {
                            showUpholdLoginPopupWindow(btnVerifyWallet);
                        } else {
                            if (!TextUtils.isEmpty(mExternalWallet.getVerifyUrl())) {
                                dismiss();
                                mBraveActivity.openNewOrSelectExistingTab(
                                        mExternalWallet.getLoginUrl());
                            }
                        }
                        break;
                    default:
                        Log.e(TAG, "Unexpected external wallet status");
                        return;
                }
            }
        }));
    }

    private void requestPublisherInfo() {
        Tab currentActiveTab = BraveRewardsHelper.currentActiveChromeTabbedActivityTab();
        if (currentActiveTab != null && !currentActiveTab.isIncognito()) {
            String url = currentActiveTab.getUrl().getSpec();
            if (URLUtil.isValidUrl(url)) {
                mBraveRewardsNativeWorker.GetPublisherInfo(currentActiveTab.getId(), url);
                mPublisherFetcher = new Timer();
                mPublisherFetcher.schedule(new PublisherFetchTimer(currentActiveTab.getId(), url),
                        PUBLISHER_INFO_FETCH_RETRY, PUBLISHER_INFO_FETCH_RETRY);
                showTipSection();
                mBtnTip.setEnabled(true);
            } else {
                showSummarySection();
                mBtnTip.setEnabled(false);
            }
        } else {
            showSummarySection();
            mBtnTip.setEnabled(false);
        }
    }

    @Override
    public void OnPublisherInfo(int tabId) {
        mPublisherExist = true;
        mCurrentTabId = tabId;
        // RemoveRewardsSummaryMonthYear();
        // if (btRewardsSummary != null) {
        //     btRewardsSummary.setClickable(true);
        // }

        String publisherFavIconURL =
                mBraveRewardsNativeWorker.GetPublisherFavIconURL(mCurrentTabId);
        Tab currentActiveTab = BraveRewardsHelper.currentActiveChromeTabbedActivityTab();
        String url = currentActiveTab.getUrl().getSpec();
        final String favicon_url = (publisherFavIconURL.isEmpty()) ? url : publisherFavIconURL;

        mIconFetcher.retrieveLargeIcon(favicon_url, this);

        String pubName = mBraveRewardsNativeWorker.GetPublisherName(mCurrentTabId);
        String pubId = mBraveRewardsNativeWorker.GetPublisherId(mCurrentTabId);
        String pubSuffix = "";
        if (pubId.startsWith(YOUTUBE_TYPE)) {
            pubSuffix = mPopupView.getResources().getString(R.string.brave_ui_on_youtube);
        } else if (pubName.startsWith(TWITCH_TYPE)) {
            pubSuffix = mPopupView.getResources().getString(R.string.brave_ui_on_twitch);
        }
        pubName = "<b>" + pubName + "</b> " + pubSuffix;
        TextView publisherName = mPopupView.findViewById(R.id.publisher_name);
        publisherName.setText(Html.fromHtml(pubName));
        mPublisherAttention = mPopupView.findViewById(R.id.attention_value_text);
        String percent =
                Integer.toString(mBraveRewardsNativeWorker.GetPublisherPercent(mCurrentTabId))
                + "%";
        mPublisherAttention.setText(percent);
        if (mSwitchAutoContribute != null) {
            mSwitchAutoContribute.setOnCheckedChangeListener(null);
            mSwitchAutoContribute.setChecked(
                    mBraveRewardsNativeWorker.GetPublisherExcluded(mCurrentTabId));
            mSwitchAutoContribute.setOnCheckedChangeListener(autoContributeSwitchListener);
        }

        updatePublisherStatus(mBraveRewardsNativeWorker.GetPublisherStatus(mCurrentTabId));

        // tv = (TextView) root.findViewById(R.id.br_no_activities_yet);
        // gl = (GridLayout) thisObject.root.findViewById(R.id.br_activities);
        // if (tv != null && gl != null) {
        //     tv.setVisibility(View.GONE);
        //     gl.setVisibility(View.GONE);
        // }
        mBraveRewardsNativeWorker.GetRecurringDonations();

        mBraveRewardsNativeWorker.GetAutoContributeProperties();
    }

    @Override
    public void OnGetAutoContributeProperties() {
        if (mBraveRewardsNativeWorker != null
                && mBraveRewardsNativeWorker.IsAutoContributeEnabled()) {
            mPopupView.findViewById(R.id.attention_layout).setVisibility(View.VISIBLE);
            mPopupView.findViewById(R.id.auto_contribution_layout).setVisibility(View.VISIBLE);
        }
        mBraveRewardsNativeWorker.GetRecurringDonations();
        mBraveRewardsNativeWorker.getAutoContributionAmount();
    }

    @Override
    public void onGetAutoContributionAmount(double amount) {
        Log.e(TAG, "Auto contribution amount : " + amount);
        TextView monthlyContributionText =
                mPopupView.findViewById(R.id.monthly_contribution_set_text);
        monthlyContributionText.setText(String.format(
                mPopupView.getResources().getString(R.string.brave_rewards_bat_value_text),
                (int) amount));
        monthlyContributionText.setOnClickListener(view -> {
            Intent intent = new Intent(
                    ContextUtils.getApplicationContext(), BraveRewardsSiteBannerActivity.class);
            intent.putExtra(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, mCurrentTabId);
            intent.putExtra(BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION, true);
            dismiss();
            mActivity.startActivityForResult(intent, BraveActivity.SITE_BANNER_REQUEST_CODE);
        });
    }

    @Override
    public void OnOneTimeTip() {
        // TODO add logic for refresh balance
        // mBraveRewardsNativeWorker.GetExternalWallet();
    }

    /**
     * OnRecurringDonationUpdated is fired after a publisher was added or removed to/from
     * recurrent donation list
     */
    @Override
    public void OnRecurringDonationUpdated() {
        String pubId = mBraveRewardsNativeWorker.GetPublisherId(mCurrentTabId);
        // mPubInReccuredDonation =
        // mBraveRewardsNativeWorker.IsCurrentPublisherInRecurrentDonations(pubId);

        // all (mPubInReccuredDonation, mAutoContributeEnabled) are false: exit
        // one is true: ac_enabled_controls on
        // mAutoContributeEnabled: attention_layout and include_in_ac_layout on
        // mPubInReccuredDonation: auto_tip_layout is on

        // if (mAutoContributeEnabled || mPubInReccuredDonation) {
        //     root.findViewById(R.id.ac_enabled_controls).setVisibility(View.VISIBLE);

        //     if (mAutoContributeEnabled) {
        //         root.findViewById(R.id.attention_layout).setVisibility(View.VISIBLE);
        //         root.findViewById(R.id.include_in_ac_layout).setVisibility(View.VISIBLE);
        //         root.findViewById(R.id.brave_ui_auto_contribute_separator_top).setVisibility(View.VISIBLE);
        //         root.findViewById(R.id.brave_ui_auto_contribute_separator_bottom).setVisibility(View.VISIBLE);
        //     }

        // Temporary commented out due to dropdown spinner inflating issue on PopupWindow (API 24)
        /*
        if (mPubInReccuredDonation){
            double amount  = mBraveRewardsNativeWorker.GetPublisherRecurrentDonationAmount(pubId);
            UpdateRecurentDonationSpinner(amount);
            root.findViewById(R.id.auto_tip_layout).setVisibility(View.VISIBLE);
        }*/
        // }
    }

    private void updatePublisherStatus(int pubStatus) {
        // Set publisher verified/unverified status
        String verifiedText = "";
        TextView publisherVerified = mPopupView.findViewById(R.id.publisher_verified);
        publisherVerified.setAlpha(1f);
        TextView refreshPublisher = mPopupView.findViewById(R.id.refresh_publisher);
        refreshPublisher.setAlpha(1f);
        refreshPublisher.setEnabled(true);
        View refreshStatusProgress = mPopupView.findViewById(R.id.progress_refresh_status);
        refreshStatusProgress.setVisibility(View.GONE);
        refreshPublisher.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String pubId = mBraveRewardsNativeWorker.GetPublisherId(mCurrentTabId);
                refreshStatusProgress.setVisibility(View.VISIBLE);
                refreshPublisher.setEnabled(false);
                publisherVerified.setAlpha(.3f);
                mBraveRewardsNativeWorker.RefreshPublisher(pubId);
            }
        }));
        if (pubStatus == BraveRewardsPublisher.CONNECTED
                || pubStatus == BraveRewardsPublisher.UPHOLD_VERIFIED) {
            verifiedText =
                    mPopupView.getResources().getString(R.string.brave_ui_verified_publisher);
            publisherVerified.setCompoundDrawablesWithIntrinsicBounds(
                    R.drawable.bat_verified, 0, 0, 0);
        } else {
            verifiedText =
                    mPopupView.getResources().getString(R.string.brave_ui_not_verified_publisher);
            publisherVerified.setCompoundDrawablesWithIntrinsicBounds(
                    R.drawable.bat_unverified, 0, 0, 0);
        }
        publisherVerified.setText(verifiedText);
        publisherVerified.setVisibility(View.VISIBLE);

        // show |brave_ui_panel_connected_text| text if
        // publisher is CONNECTED and user doesn't have any Brave funds (anonymous or
        // blinded wallets)
        // String verified_description = "";
        // if (pubStatus == BraveRewardsPublisher.CONNECTED) {
        //     BraveRewardsBalance balance_obj = mBraveRewardsNativeWorker.GetWalletBalance();
        //     if (balance_obj != null) {
        //         double braveFunds =
        //             ((balance_obj.mWallets.containsKey(BraveRewardsBalance.WALLET_ANONYMOUS)
        //               && balance_obj.mWallets.get(BraveRewardsBalance.WALLET_ANONYMOUS)
        //               != null)
        //              ? balance_obj.mWallets.get(
        //                  BraveRewardsBalance.WALLET_ANONYMOUS)
        //              : .0)
        //             + ((balance_obj.mWallets.containsKey(BraveRewardsBalance.WALLET_BLINDED)
        //                 && balance_obj.mWallets.get(BraveRewardsBalance.WALLET_BLINDED)
        //                 != null)
        //                ? balance_obj.mWallets.get(
        //                    BraveRewardsBalance.WALLET_BLINDED)
        //                : .0);
        //         if (braveFunds <= 0) {
        //             verified_description =
        //                 root.getResources().getString(R.string.brave_ui_panel_connected_text);
        //         }
        //     }
        // } else if (pubStatus == BraveRewardsPublisher.NOT_VERIFIED) {
        //     verified_description = root.getResources().getString(
        //                                R.string.brave_ui_not_verified_publisher_description);
        // }

        // if (!TextUtils.isEmpty(verified_description)) {
        //     verified_description += "<br/><font color=#73CBFF>"
        //                             + root.getResources().getString(R.string.learn_more) +
        //                             ".</font>";
        //     Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(verified_description);
        //     TextView tv_note = (TextView) root.findViewById(R.id.publisher_not_verified);
        //     tv_note.setText(toInsert);
        //     tv_note.setVisibility(View.VISIBLE);
        // }
    }

    @Override
    public void OnRefreshPublisher(int status, String publisherKey) {
        String pubName = mBraveRewardsNativeWorker.GetPublisherName(mCurrentTabId);
        if (pubName.equals(publisherKey)) {
            updatePublisherStatus(status);
        }
    };

    class PublisherFetchTimer extends TimerTask {
        private final int tabId;
        private final String url;

        PublisherFetchTimer(int tabId, String url) {
            this.tabId = tabId;
            this.url = url;
        }

        @Override
        public void run() {
            if (mPublisherExist || mPublisherFetchesCount >= PUBLISHER_FETCHES_COUNT) {
                if (mPublisherFetcher != null) {
                    mPublisherFetcher.cancel();
                    mPublisherFetcher = null;
                }

                return;
            }
            if (mBraveRewardsNativeWorker == null) {
                return;
            }
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mBraveRewardsNativeWorker.GetPublisherInfo(tabId, url);
                }
            });
            mPublisherFetchesCount++;
        }
    }

    public void dismiss() {
        mPopupWindow.dismiss();
    }

    public boolean isShowing() {
        return mPopupWindow.isShowing();
    }

    private void createUpdateBalanceTask() {
        mBalanceUpdater.schedule(new TimerTask() {
            @Override
            public void run() {
                if (mBraveRewardsNativeWorker == null) {
                    return;
                }
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mBraveRewardsNativeWorker.FetchGrants();
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

    private boolean isVerifyWalletEnabled() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(PREF_VERIFY_WALLET_ENABLE, false);
    }

    private void showUpholdLoginPopupWindow(final View view) {
        PopupWindow loginPopupWindow = new PopupWindow(mActivity);

        LayoutInflater inflater =
                (LayoutInflater) mActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        ViewGroup rewardsMainLayout = mPopupView.findViewById(R.id.rewards_main_layout);
        View loginPopupView = inflater.inflate(R.layout.uphold_login_popup_window, mPopupView);

        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity);
        if (isTablet) {
            ViewGroup.LayoutParams params = loginPopupView.getLayoutParams();
            params.width = dpToPx(mActivity, 320);
            params.height = ViewGroup.LayoutParams.WRAP_CONTENT;
            loginPopupView.setLayoutParams(params);
        }

        TextView loginActionButton = loginPopupView.findViewById(R.id.login_action_button);

        String verifiedAccountUpholdText =
                String.format(mActivity.getResources().getString(R.string.verified_account_uphold),
                        mActivity.getResources().getString(R.string.continue_to_login));
        int countinueToLoginIndex = verifiedAccountUpholdText.indexOf(
                mActivity.getResources().getString(R.string.continue_to_login));
        assert countinueToLoginIndex != 0;
        Spanned verifiedAccountUpholdTextSpanned =
                BraveRewardsHelper.spannedFromHtmlString(verifiedAccountUpholdText);
        SpannableString verifiedAccountUpholdTextSS =
                new SpannableString(verifiedAccountUpholdTextSpanned.toString());

        ClickableSpan verifiedAccountUpholdClickableSpan = new ClickableSpan() {
            @Override
            public void onClick(@NonNull View textView) {
                dismiss();
                loginPopupWindow.dismiss();
                mBraveActivity.openNewOrSelectExistingTab(mExternalWallet.getLoginUrl());
            }
            @Override
            public void updateDrawState(@NonNull TextPaint ds) {
                super.updateDrawState(ds);
                ds.setUnderlineText(false);
            }
        };

        verifiedAccountUpholdTextSS.setSpan(verifiedAccountUpholdClickableSpan,
                countinueToLoginIndex,
                countinueToLoginIndex
                        + mActivity.getResources().getString(R.string.continue_to_login).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        verifiedAccountUpholdTextSS.setSpan(
                new ForegroundColorSpan(
                        mActivity.getResources().getColor(R.color.brave_rewards_modal_theme_color)),
                countinueToLoginIndex,
                countinueToLoginIndex
                        + mActivity.getResources().getString(R.string.continue_to_login).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        loginActionButton.setMovementMethod(LinkMovementMethod.getInstance());
        loginActionButton.setText(verifiedAccountUpholdTextSS);

        // Rect bgPadding = new Rect();
        // int popupWidth = mActivity.getResources().getDimensionPixelSize(R.dimen.menu_width)
        //                  + bgPadding.left + bgPadding.right;
        mPopupView.measure(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        int popupWidth = mPopupView.getMeasuredWidth();
        loginPopupWindow.setBackgroundDrawable(
                new ColorDrawable(android.graphics.Color.TRANSPARENT));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            loginPopupWindow.setElevation(20);
        }

        loginPopupWindow.setTouchable(true);
        loginPopupWindow.setFocusable(true);
        loginPopupWindow.setOutsideTouchable(true);
        loginPopupWindow.setTouchInterceptor(new View.OnTouchListener() {
            @SuppressLint("ClickableViewAccessibility")
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_OUTSIDE) {
                    loginPopupWindow.dismiss();
                    return true;
                }
                return false;
            }
        });

        view.post(new Runnable() {
            @Override
            public void run() {
                if (SysUtils.isLowEndDevice()) {
                    loginPopupWindow.setAnimationStyle(0);
                }

                if (android.os.Build.VERSION.SDK_INT
                        < android.os.Build.VERSION_CODES.N) { // Before 7.0
                    loginPopupWindow.showAsDropDown(view, 0, 0);
                } else {
                    int[] location = new int[2];
                    view.getLocationOnScreen(location);
                    int x = location[0];
                    int y = location[1];
                    loginPopupWindow.showAtLocation(view, Gravity.NO_GRAVITY, 0, y);
                }
            }
        });
    }

    private Intent BuildVerifyWalletActivityIntent(@WalletStatus final int status) {
        Class clazz = null;
        switch (status) {
            case BraveRewardsExternalWallet.NOT_CONNECTED:
                clazz = BraveRewardsVerifyWalletActivity.class;
                break;
            case BraveRewardsExternalWallet.CONNECTED:
            case BraveRewardsExternalWallet.PENDING:
            case BraveRewardsExternalWallet.VERIFIED:
                clazz = BraveRewardsUserWalletActivity.class;
                break;
            default:
                Log.e(TAG, "Unexpected external wallet status");
                return null;
        }

        Intent intent = new Intent(ContextUtils.getApplicationContext(), clazz);
        intent.putExtra(BraveRewardsExternalWallet.ACCOUNT_URL, mExternalWallet.getAccountUrl());
        intent.putExtra(BraveRewardsExternalWallet.ADD_URL, mExternalWallet.getAddUrl());
        intent.putExtra(BraveRewardsExternalWallet.ADDRESS, mExternalWallet.getAddress());
        intent.putExtra(BraveRewardsExternalWallet.STATUS, mExternalWallet.getStatus());
        intent.putExtra(BraveRewardsExternalWallet.TOKEN, mExternalWallet.getToken());
        intent.putExtra(BraveRewardsExternalWallet.USER_NAME, mExternalWallet.getUserName());
        intent.putExtra(BraveRewardsExternalWallet.VERIFY_URL, mExternalWallet.getVerifyUrl());
        intent.putExtra(BraveRewardsExternalWallet.WITHDRAW_URL, mExternalWallet.getWithdrawUrl());
        return intent;
    }
}
