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
import android.util.DisplayMetrics;
import android.view.ContextThemeWrapper;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.PopupWindow;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.appcompat.widget.SwitchCompat;
import androidx.cardview.widget.CardView;
import androidx.core.content.ContextCompat;
import androidx.core.content.res.ResourcesCompat;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.tabs.TabLayout;

import org.json.JSONException;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.SysUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsExternalWallet;
import org.chromium.chrome.browser.BraveRewardsExternalWallet.WalletStatus;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsOnboardingPagerAdapter;
import org.chromium.chrome.browser.BraveRewardsPublisher;
import org.chromium.chrome.browser.BraveRewardsSiteBannerActivity;
import org.chromium.chrome.browser.BraveRewardsUserWalletActivity;
import org.chromium.chrome.browser.BraveRewardsVerifyWalletActivity;
import org.chromium.chrome.browser.BraveWalletProvider;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.custom_layout.HeightWrappingViewPager;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.base.DeviceFormFactor;

import java.math.RoundingMode;
import java.text.DateFormat;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
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
    private LinearLayout mRewardsMainLayout;
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

    private View braveRewardsOnboardingModalView;

    private BraveRewardsOnboardingPagerAdapter braveRewardsOnboardingPagerAdapter;
    private HeightWrappingViewPager braveRewardsViewPager;
    private View braveRewardsOnboardingView;

    private LinearLayout mWalletBalanceLayout;
    private LinearLayout mAdsStatementLayout;
    private View mWalletBalanceProgress;
    private View mAdsStatementProgress;

    public BraveRewardsPanel(View anchorView) {
        mCurrentNotificationId = "";
        mPublisherExist = false;
        mPublisherFetchesCount = 0;
        mCurrentTabId = -1;
        mAnchorView = anchorView;
        mPopupWindow = new PopupWindow(anchorView.getContext());
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

        int deviceWidth = ConfigurationUtils.getDisplayMetrics(mActivity).get("width");
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity);

        mPopupWindow.setWidth((int) (isTablet ? (deviceWidth * 0.6) : (deviceWidth * 0.95)));

        mRewardsMainLayout = mPopupView.findViewById(R.id.rewards_main_layout);

        mWalletBalanceLayout = mPopupView.findViewById(R.id.wallet_balance_layout);
        mAdsStatementLayout = mPopupView.findViewById(R.id.ads_statement_layout);
        mWalletBalanceProgress = mPopupView.findViewById(R.id.wallet_balance_progress);
        mAdsStatementProgress = mPopupView.findViewById(R.id.ads_statement_progress);

        ImageView btnRewardsSettings = mPopupView.findViewById(R.id.btn_rewards_settings);
        btnRewardsSettings.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mBraveActivity.openNewOrSelectExistingTab(BraveActivity.BRAVE_REWARDS_SETTINGS_URL);
                dismiss();
            }
        }));

        mRewardsSummaryDetailLayout =
                mPopupView.findViewById(R.id.brave_rewards_panel_summary_layout_id);
        mRewardsTipLayout = mPopupView.findViewById(R.id.brave_rewards_panel_tip_layout_id);

        TextView btnSendTip = mPopupView.findViewById(R.id.btn_send_tip);
        btnSendTip.setOnClickListener(view -> {
            Intent intent = new Intent(
                    ContextUtils.getApplicationContext(), BraveRewardsSiteBannerActivity.class);
            intent.putExtra(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, mCurrentTabId);
            mActivity.startActivityForResult(intent, BraveActivity.SITE_BANNER_REQUEST_CODE);
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

        if (mBraveRewardsNativeWorker != null) {
            String walletType = mBraveRewardsNativeWorker.getExternalWalletType();
            TextView mywalletText = mPopupView.findViewById(R.id.my_wallet_text);
            mywalletText.setCompoundDrawablesWithIntrinsicBounds(
                    0, 0, getWalletIcon(walletType), 0);
        }

        mPopupWindow.setContentView(mPopupView);
    }

    private int getWalletIcon(String walletType) {
        if (walletType.equals(BraveWalletProvider.UPHOLD)) {
            return R.drawable.uphold_white;
        } else if (walletType.equals(BraveWalletProvider.GEMINI)) {
            return R.drawable.ic_gemini_logo_white;
        } else {
            return R.drawable.ic_logo_bitflyer;
        }
    }

    private void showSummarySection() {
        mTextTip.setTextColor(
                mActivity.getResources().getColor(R.color.rewards_panel_secondary_text_color));
        mImgTip.setColorFilter(new PorterDuffColorFilter(
                mActivity.getResources().getColor(R.color.rewards_panel_secondary_text_color),
                PorterDuff.Mode.SRC_IN));

        mTextSummary.setTextColor(
                mActivity.getResources().getColor(R.color.rewards_panel_action_color));
        mImgSummary.setColorFilter(new PorterDuffColorFilter(
                mActivity.getResources().getColor(R.color.rewards_panel_action_color),
                PorterDuff.Mode.SRC_IN));

        mRewardsSummaryDetailLayout.setVisibility(View.VISIBLE);
        mRewardsTipLayout.setVisibility(View.GONE);
    }

    private void showTipSection() {
        mTextTip.setTextColor(
                mActivity.getResources().getColor(R.color.rewards_panel_action_color));
        mImgTip.setColorFilter(new PorterDuffColorFilter(
                mActivity.getResources().getColor(R.color.rewards_panel_action_color),
                PorterDuff.Mode.SRC_IN));

        mTextSummary.setTextColor(
                mActivity.getResources().getColor(R.color.rewards_panel_secondary_text_color));
        mImgSummary.setColorFilter(new PorterDuffColorFilter(
                mActivity.getResources().getColor(R.color.rewards_panel_secondary_text_color),
                PorterDuff.Mode.SRC_IN));
        mRewardsSummaryDetailLayout.setVisibility(View.GONE);
        mRewardsTipLayout.setVisibility(View.VISIBLE);
    }

    private void setNotificationsControls() {
        mNotificationLayout =
                mPopupView.findViewById(R.id.brave_rewards_panel_notification_layout_id);

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

    private boolean isValidNotificationType(int type, int argsNum) {
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
        if (mBraveRewardsNativeWorker == null) {
            return;
        }

        if (!isValidNotificationType(type, args.length) && mBraveRewardsNativeWorker != null) {
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

        String title = "";
        String description = "";
        int notificationIcon;
        View claimProgress = mPopupView.findViewById(R.id.claim_progress);

        mClaimInProcess = mBraveRewardsNativeWorker.IsGrantClaimInProcess();
        if (mClaimInProcess) {
            BraveRewardsHelper.crossfade(actionNotificationButton, claimProgress, View.GONE, 1f,
                    BraveRewardsHelper.CROSS_FADE_DURATION);
        } else {
            actionNotificationButton.setEnabled(true);
            BraveRewardsHelper.crossfade(claimProgress, actionNotificationButton, View.GONE, 1f,
                    BraveRewardsHelper.CROSS_FADE_DURATION);
        }

        switch (type) {
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_AUTO_CONTRIBUTE:
                notificationIcon = R.drawable.icon_validated_notification;
                String result = args[1];
                switch (result) {
                    case AUTO_CONTRIBUTE_SUCCESS:
                        actionNotificationButton.setText(
                                mPopupView.getResources().getString(R.string.ok));
                        title = mPopupView.getResources().getString(
                                R.string.brave_ui_rewards_contribute);
                        notificationIcon = R.drawable.ic_hearts_rewards;

                        double value = 0;
                        String valueString = "";
                        String[] splittedValue = args[3].split("\\.", 0);
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
                        title = "";
                        notificationIcon = R.drawable.ic_error_notification;
                        description = mPopupView.getResources().getString(
                                R.string.brave_ui_notification_desc_no_funds);
                        break;
                    case AUTO_CONTRIBUTE_TIPPING_ERROR:
                        title = "";
                        notificationIcon = R.drawable.ic_error_notification;
                        description = mPopupView.getResources().getString(
                                R.string.brave_ui_notification_desc_tip_error);
                        break;
                    default:
                        title = "";
                        notificationIcon = R.drawable.ic_error_notification;
                        description = mPopupView.getResources().getString(
                                R.string.brave_ui_notification_desc_contr_error);
                }
                if (title.isEmpty()) {
                    actionNotificationButton.setVisibility(View.GONE);
                }
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT:
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT_ADS:
                actionNotificationButton.setText(
                        mPopupView.getResources().getString(R.string.brave_ui_claim));

                notificationIcon = R.drawable.ic_money_bag_coins;

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
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                notificationIcon = R.drawable.ic_info_rewards;
                title = mPopupView.getResources().getString(
                        R.string.brave_ui_insufficient_funds_msg);
                description = mPopupView.getResources().getString(
                        R.string.brave_ui_insufficient_funds_desc);
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_BACKUP_WALLET:
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                notificationIcon = R.drawable.ic_info_rewards;
                title = mPopupView.getResources().getString(R.string.brave_ui_backup_wallet_msg);
                description =
                        mPopupView.getResources().getString(R.string.brave_ui_backup_wallet_desc);
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_TIPS_PROCESSED:
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                title = mPopupView.getResources().getString(R.string.brave_ui_contribution_tips);
                description = mPopupView.getResources().getString(
                        R.string.brave_ui_tips_processed_notification);
                notificationIcon = R.drawable.ic_hearts_rewards;
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_ADS_ONBOARDING:
                actionNotificationButton.setText(
                        mPopupView.getResources().getString(R.string.brave_ui_turn_on_ads));
                title = mPopupView.getResources().getString(
                        R.string.brave_ui_brave_ads_launch_title);
                description = "";
                notificationIcon = R.drawable.ic_info_rewards;
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_VERIFIED_PUBLISHER:
                String pubName = args[0];
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                title = mPopupView.getResources().getString(
                        R.string.brave_ui_pending_contribution_title);
                description = mPopupView.getResources().getString(
                        R.string.brave_ui_verified_publisher_notification, pubName);
                notificationIcon = R.drawable.ic_hearts_rewards;
                break;
            case REWARDS_NOTIFICATION_NO_INTERNET:
                title = "";
                notificationIcon = R.drawable.ic_error_notification;
                description = "\n"
                        + mPopupView.getResources().getString(R.string.brave_rewards_local_uh_oh)
                        + "\n"
                        + mPopupView.getResources().getString(
                                R.string.brave_rewards_local_server_not_responding);
                actionNotificationButton.setVisibility(View.GONE);
                break;
            case REWARDS_PROMOTION_CLAIM_ERROR:
                title = "";
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                description = "\n"
                        + mPopupView.getResources().getString(
                                R.string.brave_rewards_local_general_grant_error_title)
                        + "\n";
                notificationIcon = R.drawable.coin_stack;
                mWalletBalanceLayout.setAlpha(1.0f);
                mWalletBalanceProgress.setVisibility(View.GONE);
                break;
            default:
                Log.e(TAG, "This notification type is either invalid or not handled yet: " + type);
                assert false;
                return;
        }
        notificationTitleText.setText(title);
        notificationTitleText.setCompoundDrawablesWithIntrinsicBounds(notificationIcon, 0, 0, 0);
        notificationSubtitleText.setText(description);

        if (mNotificationLayout != null) {
            Log.e(TAG, "mNotificationLayout visible");
            mNotificationLayout.setVisibility(View.VISIBLE);
            int foregroundColor = R.color.rewards_panel_foreground_color;
            mRewardsMainLayout.setForeground(
                    new ColorDrawable(ContextCompat.getColor(mActivity, foregroundColor)));
            enableControls(false, mRewardsMainLayout);
        }

        setNotificationButtoClickListener();
    }

    private void setNotificationButtoClickListener() {
        TextView actionNotificationButton = mPopupView.findViewById(R.id.btn_action_notification);
        String strAction = (actionNotificationButton != null && mBraveRewardsNativeWorker != null)
                ? actionNotificationButton.getText().toString()
                : "";
        if (strAction.equals(mPopupView.getResources().getString(R.string.ok))) {
            actionNotificationButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
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
                    mWalletBalanceLayout.setAlpha(0.4f);
                    mWalletBalanceProgress.setVisibility(View.VISIBLE);
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
                            BraveActivity.BRAVE_REWARDS_SETTINGS_URL);
                    dismiss();
                }
            });
        }
    }

    private void dismissNotification(String id) {
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
        if (mNotificationLayout != null) {
            mNotificationLayout.setVisibility(View.GONE);
            mRewardsMainLayout.setForeground(null);
            enableControls(true, mRewardsMainLayout);
        }
    }

    @Override
    public void OnClaimPromotion(int responseCode) {
        if (responseCode != BraveRewardsNativeWorker.LEDGER_OK) {
            String args[] = {};
            showNotification(
                    REWARDS_PROMOTION_CLAIM_ERROR_ID, REWARDS_PROMOTION_CLAIM_ERROR, 0, args);
        }
    }

    @Override
    public void onUnblindedTokensReady() {
        fetchRewardsData();
    }

    private void fetchRewardsData() {
        mWalletBalanceLayout.setAlpha(0.4f);
        mWalletBalanceProgress.setVisibility(View.VISIBLE);
        mBraveRewardsNativeWorker.GetRecurringDonations();
        mBraveRewardsNativeWorker.GetAutoContributeProperties();
        mBraveRewardsNativeWorker.GetRewardsParameters();
        mBraveRewardsNativeWorker.GetExternalWallet();
        mAdsStatementLayout.setAlpha(0.4f);
        mAdsStatementProgress.setVisibility(View.VISIBLE);
        mBraveRewardsNativeWorker.getAdsAccountStatement();
        mBraveRewardsNativeWorker.GetCurrentBalanceReport();
        mBraveRewardsNativeWorker.GetAllNotifications();
    }

    @Override
    public void onReconcileComplete(int resultCode, int rewardsType, double amount) {
        fetchRewardsData();
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

        mPopupWindow.setAnimationStyle(R.style.EndIconMenuAnim);

        if (SysUtils.isLowEndDevice()) {
            mPopupWindow.setAnimationStyle(0);
        }

        mPopupWindow.showAsDropDown(mAnchorView, 0, 0);

        checkForRewardsOnboarding();
        BraveRewardsNativeWorker.getInstance().StartProcess();
    }

    private void checkForRewardsOnboarding() {
        if (mPopupView != null && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
                && BraveRewardsHelper.shouldShowBraveRewardsOnboardingOnce()) {
            showBraveRewardsOnboarding(mPopupView, false);
            BraveRewardsHelper.setShowBraveRewardsOnboardingOnce(false);
        }
    }

    @Override
    public void OnStartProcess() {
        requestPublisherInfo();
        fetchRewardsData();
        setNotificationsControls();
        if (mPopupView != null && PackageUtils.isFirstInstall(mActivity)
                && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
                && BraveRewardsHelper.shouldShowBraveRewardsOnboardingModal()
                && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(
                        Profile.getLastUsedRegularProfile())) {
            showBraveRewardsOnboardingModal(mPopupView);
            BraveRewardsHelper.updateBraveRewardsAppOpenCount();
        }
    }

    private void showBraveRewardsOnboardingModal(View root) {
        braveRewardsOnboardingModalView = root.findViewById(R.id.brave_rewards_onboarding_modal_id);
        braveRewardsOnboardingModalView.setVisibility(View.VISIBLE);

        int foregroundColor = R.color.rewards_panel_foreground_color;
        mRewardsMainLayout.setForeground(
                new ColorDrawable(ContextCompat.getColor(mActivity, foregroundColor)));
        enableControls(false, mRewardsMainLayout);

        String tosText =
                String.format(mActivity.getResources().getString(R.string.brave_rewards_tos_text),
                        mActivity.getResources().getString(R.string.terms_of_service),
                        mActivity.getResources().getString(R.string.privacy_policy));
        int termsOfServiceIndex =
                tosText.indexOf(mActivity.getResources().getString(R.string.terms_of_service));
        Spanned tosTextSpanned = BraveRewardsHelper.spannedFromHtmlString(tosText);
        SpannableString tosTextSS = new SpannableString(tosTextSpanned.toString());

        ClickableSpan tosClickableSpan = new ClickableSpan() {
            @Override
            public void onClick(@NonNull View textView) {
                CustomTabActivity.showInfoPage(mActivity, BraveActivity.BRAVE_TERMS_PAGE);
            }
            @Override
            public void updateDrawState(@NonNull TextPaint ds) {
                super.updateDrawState(ds);
                ds.setUnderlineText(false);
            }
        };

        tosTextSS.setSpan(tosClickableSpan, termsOfServiceIndex,
                termsOfServiceIndex
                        + mActivity.getResources().getString(R.string.terms_of_service).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        tosTextSS.setSpan(new ForegroundColorSpan(mActivity.getResources().getColor(
                                  R.color.brave_rewards_modal_theme_color)),
                termsOfServiceIndex,
                termsOfServiceIndex
                        + mActivity.getResources().getString(R.string.terms_of_service).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        ClickableSpan privacyProtectionClickableSpan = new ClickableSpan() {
            @Override
            public void onClick(@NonNull View textView) {
                CustomTabActivity.showInfoPage(mActivity, BraveActivity.BRAVE_PRIVACY_POLICY);
            }
            @Override
            public void updateDrawState(@NonNull TextPaint ds) {
                super.updateDrawState(ds);
                ds.setUnderlineText(false);
            }
        };

        int privacyPolicyIndex =
                tosText.indexOf(mActivity.getResources().getString(R.string.privacy_policy));
        tosTextSS.setSpan(privacyProtectionClickableSpan, privacyPolicyIndex,
                privacyPolicyIndex
                        + mActivity.getResources().getString(R.string.privacy_policy).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        tosTextSS.setSpan(new ForegroundColorSpan(mActivity.getResources().getColor(
                                  R.color.brave_rewards_modal_theme_color)),
                privacyPolicyIndex,
                privacyPolicyIndex
                        + mActivity.getResources().getString(R.string.privacy_policy).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        TextView tosAndPpText = braveRewardsOnboardingModalView.findViewById(
                R.id.brave_rewards_onboarding_modal_tos_pp_text);
        tosAndPpText.setMovementMethod(LinkMovementMethod.getInstance());
        tosAndPpText.setText(tosTextSS);

        TextView takeQuickTourButton = root.findViewById(R.id.take_quick_tour_button);
        takeQuickTourButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                braveRewardsOnboardingModalView.setVisibility(View.GONE);
                showBraveRewardsOnboarding(root, false);
            }
        }));
        TextView btnBraveRewards = root.findViewById(R.id.start_using_brave_rewards_text);
        btnBraveRewards.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                braveRewardsOnboardingModalView.setVisibility(View.GONE);
                BraveAdsNativeHelper.nativeSetAdsEnabled(Profile.getLastUsedRegularProfile());
                BraveRewardsNativeWorker.getInstance().SetAutoContributeEnabled(true);
                mBraveRewardsNativeWorker.GetAutoContributeProperties();
                BraveRewardsHelper.setShowBraveRewardsOnboardingModal(false);
                showBraveRewardsOnboarding(root, true);
            }
        }));
    }

    private void showBraveRewardsOnboarding(View root, boolean shouldShowMoreOption) {
        int foregroundColor = R.color.rewards_panel_foreground_color;
        mRewardsMainLayout.setForeground(
                new ColorDrawable(ContextCompat.getColor(mActivity, foregroundColor)));
        enableControls(false, mRewardsMainLayout);

        braveRewardsOnboardingView = root.findViewById(R.id.brave_rewards_onboarding_layout_id);
        braveRewardsOnboardingView.setVisibility(View.VISIBLE);
        final Button btnNext = braveRewardsOnboardingView.findViewById(R.id.btn_next);
        btnNext.setOnClickListener(braveRewardsOnboardingClickListener);
        braveRewardsOnboardingView.findViewById(R.id.btn_go_back)
                .setOnClickListener(braveRewardsOnboardingClickListener);
        braveRewardsOnboardingView.findViewById(R.id.btn_skip)
                .setOnClickListener(braveRewardsOnboardingClickListener);
        braveRewardsOnboardingView.findViewById(R.id.btn_start_quick_tour)
                .setOnClickListener(braveRewardsOnboardingClickListener);

        braveRewardsViewPager =
                braveRewardsOnboardingView.findViewById(R.id.brave_rewards_view_pager);
        braveRewardsViewPager.addOnPageChangeListener(new ViewPager.OnPageChangeListener() {
            @Override
            public void onPageScrolled(
                    int position, float positionOffset, int positionOffsetPixels) {
                if (positionOffset == 0 && positionOffsetPixels == 0 && position == 0) {
                    braveRewardsOnboardingView.findViewById(R.id.onboarding_first_screen_layout)
                            .setVisibility(View.VISIBLE);
                    braveRewardsOnboardingView.findViewById(R.id.onboarding_action_layout)
                            .setVisibility(View.GONE);
                } else {
                    braveRewardsOnboardingView.findViewById(R.id.onboarding_action_layout)
                            .setVisibility(View.VISIBLE);
                    braveRewardsOnboardingView.findViewById(R.id.onboarding_first_screen_layout)
                            .setVisibility(View.GONE);
                }
            }

            @Override
            public void onPageSelected(int position) {
                if (braveRewardsOnboardingPagerAdapter != null
                        && position == braveRewardsOnboardingPagerAdapter.getCount() - 1) {
                    btnNext.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
                    btnNext.setText(mActivity.getResources().getString(R.string.done));
                } else {
                    btnNext.setText(mActivity.getResources().getString(R.string.next));
                    btnNext.setCompoundDrawablesWithIntrinsicBounds(
                            0, 0, R.drawable.ic_chevron_right, 0);
                }
            }

            @Override
            public void onPageScrollStateChanged(int state) {}
        });
        braveRewardsOnboardingPagerAdapter = new BraveRewardsOnboardingPagerAdapter();
        braveRewardsOnboardingPagerAdapter.setOnboardingType(shouldShowMoreOption);
        braveRewardsViewPager.setAdapter(braveRewardsOnboardingPagerAdapter);
        TabLayout braveRewardsTabLayout =
                braveRewardsOnboardingView.findViewById(R.id.brave_rewards_tab_layout);
        braveRewardsTabLayout.setupWithViewPager(braveRewardsViewPager, true);
        AppCompatImageView modalCloseButton = braveRewardsOnboardingView.findViewById(
                R.id.brave_rewards_onboarding_layout_modal_close);
        modalCloseButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                braveRewardsOnboardingView.setVisibility(View.GONE);
                mRewardsMainLayout.setForeground(null);
                enableControls(true, mRewardsMainLayout);
            }
        }));
        braveRewardsOnboardingView.findViewById(R.id.onboarding_first_screen_layout)
                .setVisibility(View.VISIBLE);
        braveRewardsOnboardingView.findViewById(R.id.onboarding_action_layout)
                .setVisibility(View.GONE);
    }

    View.OnClickListener braveRewardsOnboardingClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            int viewId = view.getId();
            if (viewId == R.id.btn_start_quick_tour) {
                if (braveRewardsViewPager != null && braveRewardsViewPager.getCurrentItem() == 0) {
                    braveRewardsViewPager.setCurrentItem(
                            braveRewardsViewPager.getCurrentItem() + 1);
                }
            }

            if (viewId == R.id.btn_next) {
                if (braveRewardsViewPager != null && braveRewardsOnboardingPagerAdapter != null) {
                    if (braveRewardsViewPager.getCurrentItem()
                            == braveRewardsOnboardingPagerAdapter.getCount() - 1) {
                        if (braveRewardsOnboardingView != null) {
                            braveRewardsOnboardingView.setVisibility(View.GONE);

                            if (mPopupView != null
                                    && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
                                    && BraveRewardsHelper.shouldShowBraveRewardsOnboardingModal()
                                    && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(
                                            Profile.getLastUsedRegularProfile())) {
                                showBraveRewardsOnboardingModal(mPopupView);
                            } else {
                                mRewardsMainLayout.setForeground(null);
                                enableControls(true, mRewardsMainLayout);
                            }
                        }
                    } else {
                        braveRewardsViewPager.setCurrentItem(
                                braveRewardsViewPager.getCurrentItem() + 1);
                    }
                }
            }

            if (viewId == R.id.btn_skip && braveRewardsOnboardingView != null) {
                braveRewardsViewPager.setCurrentItem(
                        braveRewardsOnboardingPagerAdapter.getCount() - 1);
            }

            if (viewId == R.id.btn_go_back && braveRewardsViewPager != null) {
                braveRewardsViewPager.setCurrentItem(braveRewardsViewPager.getCurrentItem() - 1);
            }
        }
    };

    @Override
    public void OnGetCurrentBalanceReport(double[] report) {
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
            String batTextColor = GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                    ? "#FFFFFF"
                    : "#212529";

            switch (i) {
                case BALANCE_REPORT_EARNING_FROM_ADS:
                    tv = mPopupView.findViewById(R.id.rewards_from_ads_bat_text);
                    tvUSD = mPopupView.findViewById(R.id.rewards_from_ads_usd_text);
                    text = "<font color=#C12D7C>" + value + "</font><font color=" + batTextColor
                            + "> " + batText + "</font>";
                    textUSD = usdValue;
                    break;
                case BALANCE_REPORT_AUTO_CONTRIBUTE:
                    tv = mPopupView.findViewById(R.id.auto_contribute_bat_text);
                    tvUSD = mPopupView.findViewById(R.id.auto_contribute_usd_text);
                    text = "<font color=#4C54D2>" + value + "</font><font color=" + batTextColor
                            + "> " + batText + "</font>";
                    textUSD = usdValue;
                    break;
                case BALANCE_REPORT_ONE_TIME_DONATION:
                    tv = mPopupView.findViewById(R.id.one_time_tip_bat_text);
                    tvUSD = mPopupView.findViewById(R.id.one_time_tip_usd_text);
                    text = "<font color=#4C54D2>" + value + "</font><font color=" + batTextColor
                            + "> " + batText + "</font>";
                    textUSD = usdValue;
                    break;
                case BALANCE_REPORT_RECURRING_DONATION:
                    tv = mPopupView.findViewById(R.id.monthly_tips_bat_text);
                    tvUSD = mPopupView.findViewById(R.id.monthly_tips_usd_text);
                    text = "<font color=#4C54D2>" + value + "</font><font color=" + batTextColor
                            + "> " + batText + "</font>";
                    textUSD = usdValue;
                    break;
            }
            Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(text);
            if (tv != null) {
                tv.setText(toInsert);
            }
            if (tvUSD != null) {
                tvUSD.setText(textUSD);
            }
        }

        mBraveRewardsNativeWorker.GetPendingContributionsTotal();
    }

    @Override
    public void OnGetAdsAccountStatement(boolean success, double nextPaymentDate,
            int adsReceivedThisMonth, double earningsThisMonth, double earningsLastMonth) {
        mAdsStatementLayout.setAlpha(1.0f);
        mAdsStatementProgress.setVisibility(View.GONE);
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
        mWalletBalanceLayout.setAlpha(1.0f);
        mWalletBalanceProgress.setVisibility(View.GONE);
        if (errorCode == BraveRewardsNativeWorker.LEDGER_OK) {
            if (mBraveRewardsNativeWorker != null) {
                BraveRewardsBalance walletBalanceObject =
                        mBraveRewardsNativeWorker.GetWalletBalance();
                double walletBalance = 0;
                if (walletBalanceObject != null) {
                    walletBalance = walletBalanceObject.getTotal();
                }

                DecimalFormat df = new DecimalFormat("#.###");
                df.setRoundingMode(RoundingMode.FLOOR);
                df.setMinimumFractionDigits(3);
                TextView batBalanceText = mPopupView.findViewById(R.id.bat_balance_text);
                batBalanceText.setText(df.format(walletBalance));
                double usdValue = walletBalance * mBraveRewardsNativeWorker.GetWalletRate();
                String usdText =
                        String.format(mPopupView.getResources().getString(R.string.brave_ui_usd),
                                String.format(Locale.getDefault(), "%.2f", usdValue));
                TextView usdBalanceText = mPopupView.findViewById(R.id.usd_balance_text);
                usdBalanceText.setText(usdText);
            }
        } else if (errorCode == BraveRewardsNativeWorker.LEDGER_ERROR) { // No Internet connection
            String args[] = {};
            Log.e(TAG, "Failed to fetch rewards parameters from server");
            showNotification(
                    REWARDS_NOTIFICATION_NO_INTERNET_ID, REWARDS_NOTIFICATION_NO_INTERNET, 0, args);
        }
    }

    @Override
    public void OnGetLatestNotification(String id, int type, long timestamp, String[] args) {
        if (type == BraveRewardsNativeWorker.REWARDS_NOTIFICATION_BACKUP_WALLET) {
            if (mBraveRewardsNativeWorker != null) {
                mBraveRewardsNativeWorker.DeleteNotification(id);
                mBraveRewardsNativeWorker.GetAllNotifications();
            }
            return;
        }

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
        int walletStatus = BraveRewardsExternalWallet.NOT_CONNECTED;
        if (!TextUtils.isEmpty(externalWallet)) {
            try {
                mExternalWallet = new BraveRewardsExternalWallet(externalWallet);
                walletStatus = mExternalWallet.getStatus();
                mWalletBalanceLayout.setAlpha(0.4f);
                mWalletBalanceProgress.setVisibility(View.VISIBLE);
                mBraveRewardsNativeWorker.GetRewardsParameters();
            } catch (JSONException e) {
                mExternalWallet = null;
            }
        }
        setVerifyWalletButton(walletStatus);
        showRewardsFromAdsSummary(walletStatus);
    }

    private void showRewardsFromAdsSummary(int walletStatus) {
        if (mBraveRewardsNativeWorker != null) {
            String walletType = mBraveRewardsNativeWorker.getExternalWalletType();
            if (walletStatus == BraveRewardsExternalWallet.VERIFIED
                    && (walletType.equals(BraveWalletProvider.UPHOLD)
                            || walletType.equals(BraveWalletProvider.BITFLYER)
                            || walletType.equals(BraveWalletProvider.GEMINI))) {
                mPopupView.findViewById(R.id.auto_contribute_summary_seperator)
                        .setVisibility(View.GONE);
                mPopupView.findViewById(R.id.rewards_from_ads_summary_layout)
                        .setVisibility(View.GONE);
            } else {
                mPopupView.findViewById(R.id.auto_contribute_summary_seperator)
                        .setVisibility(View.VISIBLE);
                mPopupView.findViewById(R.id.rewards_from_ads_summary_layout)
                        .setVisibility(View.VISIBLE);
            }
            // Hide rewards from ads when verified but disconnected from provider
            if (walletStatus == BraveRewardsExternalWallet.DISCONNECTED_VERIFIED) {
                mPopupView.findViewById(R.id.rewards_from_ads_summary_layout)
                        .setVisibility(View.GONE);
            }
        }
    }

    private void setVerifyWalletButton(@WalletStatus final int status) {
        TextView btnVerifyWallet = mPopupView.findViewById(R.id.btn_verify_wallet);
        SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor editor = sharedPref.edit();
        int rightDrawable = 0;
        int leftDrawable = 0;
        int textId = 0;
        String walletType = mBraveRewardsNativeWorker.getExternalWalletType();

        switch (status) {
            case BraveRewardsExternalWallet.NOT_CONNECTED:
                rightDrawable = R.drawable.ic_verify_wallet_arrow;
                textId = R.string.brave_ui_wallet_button_unverified;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                Log.e(TAG, "BraveRewardsExternalWallet.NOT_CONNECTED");
                break;
            case BraveRewardsExternalWallet.CONNECTED:
                rightDrawable = R.drawable.verified_disclosure;
                textId = R.string.brave_ui_wallet_button_unverified;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                break;
            case BraveRewardsExternalWallet.PENDING:
                editor.putBoolean(PREF_VERIFY_WALLET_ENABLE, true);
                editor.apply();

                rightDrawable = R.drawable.verified_disclosure;
                textId = R.string.brave_ui_wallet_button_unverified;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                break;
            case BraveRewardsExternalWallet.VERIFIED:
                editor.putBoolean(PREF_VERIFY_WALLET_ENABLE, true);
                editor.apply();

                leftDrawable = getWalletIcon(walletType);
                rightDrawable = R.drawable.verified_disclosure;
                textId = R.string.brave_ui_wallet_button_verified;
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
                leftDrawable = getWalletIcon(walletType);
                textId = R.string.brave_ui_wallet_button_disconnected;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(leftDrawable, 0, 0, 0);
                btnVerifyWallet.setBackgroundDrawable(ResourcesCompat.getDrawable(
                        ContextUtils.getApplicationContext().getResources(),
                        R.drawable.wallet_disconnected_button, /* theme= */ null));
                break;
            default:
                Log.e(TAG, "Unexpected external wallet status");
                return;
        }

        btnVerifyWallet.setVisibility(View.VISIBLE);
        btnVerifyWallet.setText(mPopupView.getResources().getString(textId));
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
                        if (status == BraveRewardsExternalWallet.NOT_CONNECTED) {
                            TabUtils.openUrlInNewTab(false,
                                    BraveActivity.BRAVE_REWARDS_SETTINGS_WALLET_VERIFICATION_URL);
                            dismiss();
                        } else {
                            int requestCode = (status == BraveRewardsExternalWallet.NOT_CONNECTED)
                                    ? BraveActivity.VERIFY_WALLET_ACTIVITY_REQUEST_CODE
                                    : BraveActivity.USER_WALLET_ACTIVITY_REQUEST_CODE;
                            Intent intent = BuildVerifyWalletActivityIntent(status);
                            if (intent != null) {
                                mActivity.startActivityForResult(intent, requestCode);
                            }
                        }
                        break;
                    case BraveRewardsExternalWallet.DISCONNECTED_NOT_VERIFIED:
                    case BraveRewardsExternalWallet.DISCONNECTED_VERIFIED:
                        TabUtils.openUrlInNewTab(false,
                                BraveActivity.BRAVE_REWARDS_SETTINGS_WALLET_VERIFICATION_URL);
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
                    !mBraveRewardsNativeWorker.GetPublisherExcluded(mCurrentTabId));
            mSwitchAutoContribute.setOnCheckedChangeListener(autoContributeSwitchListener);
        }
        updatePublisherStatus(mBraveRewardsNativeWorker.GetPublisherStatus(mCurrentTabId));
    }

    @Override
    public void OnGetAutoContributeProperties() {
        if (mBraveRewardsNativeWorker != null
                && mBraveRewardsNativeWorker.IsAutoContributeEnabled()) {
            mPopupView.findViewById(R.id.attention_layout).setVisibility(View.VISIBLE);
            mPopupView.findViewById(R.id.auto_contribution_layout).setVisibility(View.VISIBLE);
            mPopupView.findViewById(R.id.auto_contribute_summary_seperator)
                    .setVisibility(View.VISIBLE);
            mPopupView.findViewById(R.id.auto_contribute_summary_layout)
                    .setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void OnRecurringDonationUpdated() {
        updateMonthlyContributionUI();
    }

    private void updateMonthlyContributionUI() {
        String pubId = mBraveRewardsNativeWorker.GetPublisherId(mCurrentTabId);
        TextView monthlyTipText = mPopupView.findViewById(R.id.monthly_contribution_set_text);
        double recurrentAmount =
                mBraveRewardsNativeWorker.GetPublisherRecurrentDonationAmount(pubId);
        if (mBraveRewardsNativeWorker.IsCurrentPublisherInRecurrentDonations(pubId)) {
            monthlyTipText.setText(String.format(
                    mPopupView.getResources().getString(R.string.brave_rewards_bat_value_text),
                    (int) recurrentAmount));
            monthlyTipText.setCompoundDrawablesWithIntrinsicBounds(
                    0, 0, R.drawable.ic_carat_down, 0);
        }
        monthlyTipText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!mBraveRewardsNativeWorker.IsCurrentPublisherInRecurrentDonations(pubId)) {
                    openBannerActivity();
                } else {
                    Context wrapper =
                            new ContextThemeWrapper(mActivity, R.style.BraveRewardsPanelPopupMenu);
                    PopupMenu popup = new PopupMenu(wrapper, v);
                    popup.getMenuInflater().inflate(
                            R.menu.monthly_contribution_popup_menu, popup.getMenu());
                    popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem item) {
                            if (item.getItemId() == R.id.change_amount_menu_id) {
                                openBannerActivity();
                            } else {
                                monthlyTipText.setText(
                                        mPopupView.getResources().getString(R.string.set));
                                monthlyTipText.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
                                mBraveRewardsNativeWorker.RemoveRecurring(pubId);
                            }
                            return true;
                        }
                    });

                    popup.show();
                }
            }
        });
    }

    private void openBannerActivity() {
        Intent intent = new Intent(
                ContextUtils.getApplicationContext(), BraveRewardsSiteBannerActivity.class);
        intent.putExtra(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, mCurrentTabId);
        intent.putExtra(BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION, true);
        mActivity.startActivityForResult(intent, BraveActivity.MONTHLY_CONTRIBUTION_REQUEST_CODE);
    }

    private void updatePublisherStatus(int pubStatus) {
        String verifiedText = "";
        TextView publisherVerified = mPopupView.findViewById(R.id.publisher_verified);
        publisherVerified.setAlpha(1f);
        ImageView refreshPublisher = mPopupView.findViewById(R.id.refresh_publisher);
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
                || pubStatus == BraveRewardsPublisher.UPHOLD_VERIFIED
                || pubStatus == BraveRewardsPublisher.BITFLYER_VERIFIED
                || pubStatus == BraveRewardsPublisher.GEMINI_VERIFIED) {
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

    private Intent BuildVerifyWalletActivityIntent(@WalletStatus final int status) {
        Class clazz = null;
        switch (status) {
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

    private void enableControls(boolean enable, ViewGroup vg) {
        for (int i = 0; i < vg.getChildCount(); i++) {
            View child = vg.getChildAt(i);
            if (child.getId() != R.id.tip_btn) {
                child.setEnabled(enable);
            }
            if (child instanceof ViewGroup) {
                enableControls(enable, (ViewGroup) child);
            }
        }
    }
}
