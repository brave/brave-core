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
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.PopupWindow;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.appcompat.widget.SwitchCompat;
import androidx.cardview.widget.CardView;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.content.res.ResourcesCompat;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.tabs.TabLayout;

import org.json.JSONException;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.BuildInfo;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.SysUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsExternalWallet;
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
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.notifications.BraveNotificationWarningDialog;
import org.chromium.chrome.browser.notifications.BravePermissionUtils;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ledger.mojom.UserType;
import org.chromium.ledger.mojom.WalletStatus;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.permissions.PermissionConstants;

import java.math.RoundingMode;
import java.text.DateFormat;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;
import java.util.TreeMap;
import java.util.concurrent.TimeUnit;

public class BraveRewardsPanel
        implements BraveRewardsObserver, BraveRewardsHelper.LargeIconReadyCallback {
    public static final String PREF_WAS_BRAVE_REWARDS_TURNED_ON = "brave_rewards_turned_on";
    public static final String PREF_GRANTS_NOTIFICATION_RECEIVED = "grants_notification_received";
    public static final String PREF_WAS_TOOLBAR_BAT_LOGO_BUTTON_PRESSED =
            "was_toolbar_bat_logo_button_pressed";
    private static final String UNVERIFIED_USER_UNSUPPORTED_REGION_PAGE =
            "https://support.brave.com/hc/en-us/articles/6539887971469";
    private static final String NEW_SIGNUP_DISABLED_URL =
            "https://support.brave.com/hc/en-us/articles/9312922941069";
    private static final String BRAVE_REWARDS_PAGE = "https://brave.com/rewards";
    private static final String BRAVE_REWARDS_CHANGES_PAGE = "https://brave.com/rewards-changes";

    private static final String TAG = "BraveRewards";
    private static final int UPDATE_BALANCE_INTERVAL = 60000; // In milliseconds
    private static final int PUBLISHER_INFO_FETCH_RETRY = 3 * 1000; // In milliseconds
    private static final int PUBLISHER_FETCHES_COUNT = 3;

    private static final String YOUTUBE_TYPE = "youtube#";
    private static final String TWITCH_TYPE = "twitch#";

    private static final String PREF_VERIFY_WALLET_ENABLE = "verify_wallet_enable";

    private static final String SUCCESS = "success";

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
    private static final String ERROR_CONVERT_PROBI = "ERROR";
    private static final String WALLET_GENERATION_DISABLED_ERROR = "wallet-generation-disabled";

    private static final int CLICK_DISABLE_INTERVAL = 1000; // In milliseconds

    public enum NotificationClickAction {
        DO_NOTHING,
        RECONNECT,
        CLAIM,
        TURN_ON_ADS;
    }

    private static final String WHITE_COLOR = "#FFFFFF";
    private static final String DARK_GRAY_COLOR = "#212529";

    private boolean shouldShowOnboardingForConnectAccount;

    private final View mAnchorView;
    private final PopupWindow mPopupWindow;
    private ViewGroup mPopupView;
    private FrameLayout mBravePanelShadow;
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

    private View mBraveRewardsOnboardingModalView;
    private View mRewardsResponseModal;
    private View mConnectAccountModal;
    private View mRewardsVbatExpireNoticeModal;

    private BraveRewardsOnboardingPagerAdapter mBraveRewardsOnboardingPagerAdapter;
    private HeightWrappingViewPager mBraveRewardsViewPager;
    private View mBraveRewardsOnboardingView;
    private ViewGroup mBraveRewardsUnverifiedView;

    private LinearLayout mWalletBalanceLayout;
    private FrameLayout mLoggedOutStateLayout;
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
        setUpViews();
    }

    public void showLikePopDownMenu() {
        mPopupWindow.setTouchable(true);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setOutsideTouchable(true);

        mPopupWindow.setAnimationStyle(R.style.EndIconMenuAnim);

        if (SysUtils.isLowEndDevice()) {
            mPopupWindow.setAnimationStyle(0);
        }

        mPopupWindow.showAsDropDown(mAnchorView, 0, 0);

        mBraveRewardsNativeWorker.GetExternalWallet();
    }

    private void setUpViews() {
        LayoutInflater inflater = (LayoutInflater) mAnchorView.getContext().getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        mPopupView = (ViewGroup) inflater.inflate(R.layout.brave_rewards_panel_layout, null);

        int deviceWidth = ConfigurationUtils.getDisplayMetrics(mActivity).get("width");
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity);

        mPopupWindow.setWidth((int) (isTablet ? (deviceWidth * 0.6) : (deviceWidth * 0.95)));

        mBravePanelShadow = mPopupView.findViewById(R.id.panel_shadow);
        mBraveRewardsOnboardingView =
                mPopupView.findViewById(R.id.brave_rewards_onboarding_layout_id);
        mBraveRewardsOnboardingModalView =
                mPopupView.findViewById(R.id.brave_rewards_onboarding_modal_id);
        mNotificationLayout =
                mPopupView.findViewById(R.id.brave_rewards_panel_notification_layout_id);
        mRewardsResponseModal = mPopupView.findViewById(R.id.rewards_response_modal_id);
        mConnectAccountModal = mPopupView.findViewById(R.id.connect_account_layout_id);
        mBraveRewardsUnverifiedView =
                mPopupView.findViewById(R.id.brave_rewards_panel_unverified_layout_id);
        mRewardsMainLayout = mPopupView.findViewById(R.id.rewards_main_layout);
        mRewardsSummaryDetailLayout =
                mPopupView.findViewById(R.id.brave_rewards_panel_summary_layout_id);
        mRewardsTipLayout = mPopupView.findViewById(R.id.brave_rewards_panel_tip_layout_id);

        mRewardsVbatExpireNoticeModal =
                mPopupView.findViewById(R.id.brave_rewards_vbat_expire_notice_modal_id);
        mPopupWindow.setContentView(mPopupView);
    }

    // Rewards main layout changes
    private void showRewardsMainUI() {
        if (mRewardsMainLayout == null) {
            return;
        }
        mRewardsMainLayout.setVisibility(View.VISIBLE);

        mWalletBalanceLayout = mPopupView.findViewById(R.id.wallet_balance_layout);
        mLoggedOutStateLayout = mPopupView.findViewById(R.id.logged_out_state_layout);
        TextView loggedOutStateText =
                mLoggedOutStateLayout.findViewById(R.id.logged_out_state_text);
        loggedOutStateText.setText(String.format(
                mActivity.getResources().getString(R.string.logged_out_state_dialog_text),
                getWalletString(mBraveRewardsNativeWorker.getExternalWalletType())));
        mWalletBalanceLayout.setVisibility(
                (mExternalWallet != null && mExternalWallet.getStatus() == WalletStatus.LOGGED_OUT)
                        ? View.GONE
                        : View.VISIBLE);
        mLoggedOutStateLayout.setVisibility(
                (mExternalWallet != null && mExternalWallet.getStatus() == WalletStatus.LOGGED_OUT)
                        ? View.VISIBLE
                        : View.GONE);
        mLoggedOutStateLayout.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mExternalWallet != null) {
                    TabUtils.openUrlInNewTab(false, mExternalWallet.getLoginUrl());
                }
                dismiss();
            }
        }));
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

        TextView btnSendTip = mPopupView.findViewById(R.id.btn_send_tip);
        btnSendTip.setOnClickListener(view -> {
            Intent intent = new Intent(
                    ContextUtils.getApplicationContext(), BraveRewardsSiteBannerActivity.class);
            intent.putExtra(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, mCurrentTabId);
            mActivity.startActivityForResult(intent, BraveConstants.SITE_BANNER_REQUEST_CODE);
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

        String monthName = (String) android.text.format.DateFormat.format("MMMM", new Date());
        String monthNameShort = (String) android.text.format.DateFormat.format("MMM", new Date());
        int lastDate = Calendar.getInstance().getActualMaximum(Calendar.DAY_OF_MONTH);
        int currentYear = Calendar.getInstance().get(Calendar.YEAR);

        String adsMonthlyStatement = new StringBuilder("1")
                                             .append(" ")
                                             .append(monthNameShort.replaceAll("\\.", ""))
                                             .append(" - ")
                                             .append(lastDate)
                                             .append(" ")
                                             .append(monthNameShort.replaceAll("\\.", ""))
                                             .toString();
        String monthYear = new StringBuilder(monthName).append(" ").append(currentYear).toString();

        TextView adsMonthlyStatementText = mPopupView.findViewById(R.id.ads_monthly_statement_text);
        adsMonthlyStatementText.setText(adsMonthlyStatement);

        TextView monthYearText = mPopupView.findViewById(R.id.month_year_text);
        monthYearText.setText(monthYear);
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

    // Notification changes
    private void setNotificationsControls() {
        if (mNotificationLayout == null) {
            return;
        }
        ImageView btnCloseNotification =
                mNotificationLayout.findViewById(R.id.btn_close_notification);
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
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_TIPS_PROCESSED:
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_ADS_ONBOARDING:
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_PENDING_NOT_ENOUGH_FUNDS:
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GENERAL_LEDGER:
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

        if (mExternalWallet == null
                || (mExternalWallet.getStatus() == WalletStatus.NOT_CONNECTED
                        && type == BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT)) {
            return;
        }

        if (!isValidNotificationType(type, args.length)) {
            mBraveRewardsNativeWorker.DeleteNotification(id);
            return;
        }

        mCurrentNotificationId = id;
        ImageView notificationClaimImg = mPopupView.findViewById(R.id.notification_claim_img);
        TextView notificationClaimSubText =
                mPopupView.findViewById(R.id.notification_claim_subtitle_text);
        TextView notificationDayText = mPopupView.findViewById(R.id.notification_day_text);
        String currentDay = (String) android.text.format.DateFormat.format("MMM dd", new Date());
        notificationDayText.setText(currentDay);

        TextView notificationTitleText = mPopupView.findViewById(R.id.notification_title_text);
        TextView notificationSubtitleText =
                mPopupView.findViewById(R.id.notification_subtitle_text);
        TextView actionNotificationButton = mPopupView.findViewById(R.id.btn_action_notification);

        String title = "";
        String description = "";
        int notificationIcon = R.drawable.ic_info_rewards;
        View claimProgress = mPopupView.findViewById(R.id.claim_progress);
        NotificationClickAction notificationClickAction = NotificationClickAction.DO_NOTHING;

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
                notificationClaimImg.setVisibility(View.GONE);
                notificationClaimSubText.setVisibility(View.GONE);
                String result = args[1];
                switch (result) {
                    case AUTO_CONTRIBUTE_SUCCESS: // Succcess
                        actionNotificationButton.setText(
                                mPopupView.getResources().getString(R.string.ok));
                        notificationClickAction = NotificationClickAction.DO_NOTHING;
                        title = mPopupView.getResources().getString(
                                R.string.brave_ui_rewards_contribute);
                        notificationIcon = R.drawable.ic_notification_auto_contribute;

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
                    case AUTO_CONTRIBUTE_GENERAL_ERROR: // General error
                        actionNotificationButton.setText(
                                mPopupView.getResources().getString(R.string.ok));
                        title = mPopupView.getResources().getString(
                                R.string.monthly_tip_failed_notification_title);
                        notificationIcon = R.drawable.ic_notification_error;
                        description = mPopupView.getResources().getString(
                                R.string.monthly_tip_failed_notification_text);
                        break;
                    default:
                        title = "";
                        notificationIcon = R.drawable.ic_notification_error;
                        description = mPopupView.getResources().getString(
                                R.string.brave_ui_notification_desc_contr_error);
                }
                if (title.isEmpty()) {
                    actionNotificationButton.setVisibility(View.GONE);
                }
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT: // UGP grant
                long claimableUntilGrant = Long.parseLong(args[2]);
                long grantDays =
                        TimeUnit.MILLISECONDS.toDays(claimableUntilGrant - new Date().getTime());
                notificationClaimImg.setVisibility(View.VISIBLE);
                notificationClaimSubText.setVisibility(View.VISIBLE);
                actionNotificationButton.setText(
                        mPopupView.getResources().getString(R.string.brave_ui_claim));
                notificationClickAction = NotificationClickAction.CLAIM;
                notificationIcon = 0;
                title = mPopupView.getResources().getString(R.string.brave_ui_new_token_grant);
                description = "";
                String grantMessage = mPopupView.getResources().getString(
                        R.string.brave_ads_notification_sub_text, "<b>" + grantDays + "</b>");
                notificationClaimSubText.setText(
                        BraveRewardsHelper.spannedFromHtmlString(grantMessage));
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT_ADS: // Ads grant
                String grantAmount = args[0];
                long createdAtGrantAds = Long.parseLong(args[1]);
                long claimableUntilGrantAds = Long.parseLong(args[2]);
                String createdAtMonthGrantAds = (String) android.text.format.DateFormat.format(
                        "MMMM", new Date(createdAtGrantAds));
                long grantAdsDays =
                        TimeUnit.MILLISECONDS.toDays(claimableUntilGrantAds - new Date().getTime());
                notificationClaimImg.setVisibility(View.VISIBLE);
                notificationClaimSubText.setVisibility(View.VISIBLE);
                actionNotificationButton.setText(
                        mPopupView.getResources().getString(R.string.brave_ui_claim));
                notificationClickAction = NotificationClickAction.CLAIM;
                notificationIcon = 0;
                title = mPopupView.getResources().getString(
                        R.string.brave_ads_notification_title, createdAtMonthGrantAds);
                description = mPopupView.getResources().getString(
                        R.string.brave_ads_notification_text, createdAtMonthGrantAds, grantAmount);
                String grantAdsMessage = mPopupView.getResources().getString(
                        R.string.brave_ads_notification_sub_text, "<b>" + grantAdsDays + "</b>");
                notificationClaimSubText.setText(
                        BraveRewardsHelper.spannedFromHtmlString(grantAdsMessage));
                break;
            case BraveRewardsNativeWorker
                    .REWARDS_NOTIFICATION_TIPS_PROCESSED: // Monthly tip completed
                notificationClaimImg.setVisibility(View.GONE);
                notificationClaimSubText.setVisibility(View.GONE);
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                notificationClickAction = NotificationClickAction.DO_NOTHING;
                title = mPopupView.getResources().getString(R.string.brave_ui_contribution_tips);
                description = mPopupView.getResources().getString(
                        R.string.brave_ui_tips_processed_notification);
                notificationIcon = R.drawable.ic_notification_auto_contribute;
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_ADS_ONBOARDING:
                notificationClaimImg.setVisibility(View.GONE);
                notificationClaimSubText.setVisibility(View.GONE);
                actionNotificationButton.setText(
                        mPopupView.getResources().getString(R.string.brave_ui_turn_on_ads));
                notificationClickAction = NotificationClickAction.TURN_ON_ADS;
                title = mPopupView.getResources().getString(
                        R.string.brave_ui_brave_ads_launch_title);
                description = "";
                notificationIcon = R.drawable.ic_info_rewards;
                break;
            case BraveRewardsNativeWorker
                    .REWARDS_NOTIFICATION_VERIFIED_PUBLISHER: // Pending publisher verified
                notificationClaimImg.setVisibility(View.GONE);
                notificationClaimSubText.setVisibility(View.GONE);
                String pubName = args[0];
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                notificationClickAction = NotificationClickAction.DO_NOTHING;
                title = mPopupView.getResources().getString(
                        R.string.brave_ui_pending_contribution_title);
                description = mPopupView.getResources().getString(
                        R.string.brave_ui_verified_publisher_notification, pubName);
                notificationIcon = R.drawable.ic_notification_pending;
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GENERAL_LEDGER:
                notificationClaimImg.setVisibility(View.GONE);
                notificationClaimSubText.setVisibility(View.GONE);
                String errorType = args[0];
                switch (errorType) {
                    case "wallet_disconnected":
                        actionNotificationButton.setText(mPopupView.getResources().getString(
                                R.string.logged_out_notification_action_text));
                        notificationClickAction = NotificationClickAction.RECONNECT;
                        title = mPopupView.getResources().getString(
                                R.string.logged_out_notification_title);
                        description = mPopupView.getResources().getString(
                                R.string.logged_out_notification_text);
                        notificationIcon = R.drawable.ic_notification_error;
                        break;
                    case "uphold_bat_not_allowed":
                        actionNotificationButton.setText(
                                mPopupView.getResources().getString(R.string.ok));
                        notificationClickAction = NotificationClickAction.DO_NOTHING;
                        title = mPopupView.getResources().getString(
                                R.string.bat_unavailable_notification_title);
                        description = mPopupView.getResources().getString(
                                R.string.bat_unavailable_notification_text);
                        notificationIcon = R.drawable.ic_notification_error;
                        break;
                    case "uphold_insufficient_capabilities":
                        actionNotificationButton.setText(
                                mPopupView.getResources().getString(R.string.ok));
                        notificationClickAction = NotificationClickAction.DO_NOTHING;
                        title = mPopupView.getResources().getString(
                                R.string.limited_functionality_notification_title);
                        description = mPopupView.getResources().getString(
                                R.string.limited_functionality_notification_text);
                        notificationIcon = R.drawable.ic_notification_error;
                        break;
                }
                break;
            case REWARDS_NOTIFICATION_NO_INTERNET:
                notificationClaimImg.setVisibility(View.GONE);
                notificationClaimSubText.setVisibility(View.GONE);
                title = "";
                notificationIcon = R.drawable.ic_notification_error;
                description = "\n"
                        + mPopupView.getResources().getString(R.string.brave_rewards_local_uh_oh)
                        + "\n"
                        + mPopupView.getResources().getString(
                                R.string.brave_rewards_local_server_not_responding);
                actionNotificationButton.setVisibility(View.GONE);
                break;
            case REWARDS_PROMOTION_CLAIM_ERROR:
                notificationClaimImg.setVisibility(View.GONE);
                notificationClaimSubText.setVisibility(View.GONE);
                title = "";
                actionNotificationButton.setText(mPopupView.getResources().getString(R.string.ok));
                notificationClickAction = NotificationClickAction.DO_NOTHING;
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
        notificationTitleText.setVisibility(title.isEmpty() ? View.GONE : View.VISIBLE);
        notificationTitleText.setCompoundDrawablesWithIntrinsicBounds(notificationIcon, 0, 0, 0);
        notificationSubtitleText.setText(description);
        notificationSubtitleText.setVisibility(description.isEmpty() ? View.GONE : View.VISIBLE);

        if (mNotificationLayout != null) {
            mNotificationLayout.setVisibility(View.VISIBLE);
            int foregroundColor = R.color.rewards_panel_foreground_color;
            mRewardsMainLayout.setForeground(
                    new ColorDrawable(ContextCompat.getColor(mActivity, foregroundColor)));
            enableControls(false, mRewardsMainLayout);
        }

        setNotificationButtoClickListener(notificationClickAction);
    }

    private void setNotificationButtoClickListener(
            NotificationClickAction notificationClickAction) {
        TextView actionNotificationButton = mPopupView.findViewById(R.id.btn_action_notification);
        if (notificationClickAction == NotificationClickAction.DO_NOTHING) {
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
        } else if (notificationClickAction == NotificationClickAction.CLAIM) {
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
        } else if (notificationClickAction == NotificationClickAction.RECONNECT) {
            actionNotificationButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    mBraveRewardsNativeWorker.DeleteNotification(mCurrentNotificationId);
                    TabUtils.openUrlInNewTab(false,
                            mExternalWallet != null
                                    ? mExternalWallet.getLoginUrl()
                                    : BraveActivity.BRAVE_REWARDS_SETTINGS_WALLET_VERIFICATION_URL);
                    dismiss();
                }
            });
        } else if (notificationClickAction == NotificationClickAction.TURN_ON_ADS) {
            actionNotificationButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
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

    // Rewards data callbacks
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
        mBraveRewardsNativeWorker.GetExternalWallet();
    }

    @Override
    public void onReconcileComplete(int resultCode, int rewardsType, double amount) {
        mBraveRewardsNativeWorker.GetExternalWallet();
    }

    @Override
    public void onGetAvailableCountries(String[] countries) {
        showDeclareGeoModal(countries);
    }

    @Override
    public void onCreateRewardsWallet(String result) {
        mBraveRewardsOnboardingModalView.setVisibility(View.GONE);
        if (result.equals(SUCCESS)) {
            mBraveRewardsNativeWorker.GetAutoContributeProperties();
            if (!PackageUtils.isFirstInstall(mActivity)) {
                showRewardsResponseModal(true, result);
            } else {
                BraveRewardsHelper.setShowBraveRewardsOnboardingModal(false);
                shouldShowOnboardingForConnectAccount = true;
            }
        } else {
            showRewardsResponseModal(false, result);
        }
        mBraveRewardsNativeWorker.GetExternalWallet();
    }

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
                    ? WHITE_COLOR
                    : DARK_GRAY_COLOR;

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

    private void requestNotificationPermission() {
        if (BravePermissionUtils.isBraveAdsNotificationPermissionBlocked(mAnchorView.getContext())
                || mActivity.shouldShowRequestPermissionRationale(
                        PermissionConstants.NOTIFICATION_PERMISSION)
                || (!BuildInfo.isAtLeastT() || !BuildInfo.targetsAtLeastT())) {
            // other than android 13 redirect to
            // setting page and for android 13 Last time don't allow selected in permission
            // dialog, then enable through setting, this done through this dialog
            showNotificationWarningDialog();
        } else {
            // 1st time request permission
            ActivityCompat.requestPermissions(
                    mActivity, new String[] {PermissionConstants.NOTIFICATION_PERMISSION}, 1);
        }
    }

    private void showNotificationWarningDialog() {
        BraveNotificationWarningDialog notificationWarningDialog =
                BraveNotificationWarningDialog.newInstance(
                        BraveNotificationWarningDialog.FROM_LAUNCHED_BRAVE_PANEL);
        notificationWarningDialog.setCancelable(false);
        notificationWarningDialog.show(mActivity.getSupportFragmentManager(),
                BraveNotificationWarningDialog.NOTIFICATION_WARNING_DIALOG_TAG);
    }

    @Override
    public void OnGetAdsAccountStatement(boolean success, double nextPaymentDate,
            int adsReceivedThisMonth, double earningsThisMonth, double earningsLastMonth) {
        mAdsStatementLayout.setAlpha(1.0f);
        mAdsStatementProgress.setVisibility(View.GONE);
        if (mExternalWallet != null && mExternalWallet.getStatus() == WalletStatus.NOT_CONNECTED
                && !PackageUtils.isFirstInstall(mActivity)) {
            mPopupView.findViewById(R.id.bat_ads_balance_learn_more_layout)
                    .setVisibility(View.VISIBLE);
            mPopupView.findViewById(R.id.bat_ads_balance_layout).setVisibility(View.GONE);
            mPopupView.findViewById(R.id.usd_balance_ads_text).setVisibility(View.GONE);
            mPopupView.findViewById(R.id.bat_ads_balance_learn_more_text)
                    .setOnClickListener((new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            CustomTabActivity.showInfoPage(mActivity, BRAVE_REWARDS_CHANGES_PAGE);
                        }
                    }));
        } else {
            mPopupView.findViewById(R.id.bat_ads_balance_learn_more_layout)
                    .setVisibility(View.GONE);
            mPopupView.findViewById(R.id.bat_ads_balance_layout).setVisibility(View.VISIBLE);
            mPopupView.findViewById(R.id.usd_balance_ads_text).setVisibility(View.VISIBLE);
            DecimalFormat df = new DecimalFormat("#.###");
            df.setRoundingMode(RoundingMode.FLOOR);
            df.setMinimumFractionDigits(3);
            TextView batBalanceAdsText = mPopupView.findViewById(R.id.bat_balance_ads_text);
            batBalanceAdsText.setText(df.format(earningsThisMonth));
            double usdValue = earningsThisMonth * mBraveRewardsNativeWorker.GetWalletRate();
            String usdText = String.format(
                    mPopupView.getResources().getString(R.string.brave_ads_statement_usd),
                    String.format(Locale.getDefault(), "%.2f", usdValue));
            TextView usdBalanceAdsText = mPopupView.findViewById(R.id.usd_balance_ads_text);
            usdBalanceAdsText.setText(usdText);
        }
    }

    @Override
    public void onGetUserType(int userType) {
        switch (userType) {
            case UserType.LEGACY_UNCONNECTED:
                showVbatExpireNotice();
                break;
            case UserType.UNCONNECTED:
                newInstallViewChanges();
                break;
        }
    }

    @Override
    public void onBalance(int errorCode) {
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
    public void OnRewardsParameters() {
        if (shouldShowOnboardingForConnectAccount) {
            shouldShowOnboardingForConnectAccount = false;
            showBraveRewardsOnboarding(true);
        } else if (mExternalWallet != null) {
            if (mBraveRewardsNativeWorker.getVbatDeadline() > 0) {
                mBraveRewardsNativeWorker.getUserType();
            }
            showViewsBasedOnExternalWallet();
        }
    }

    @Override
    public void OnGetLatestNotification(String id, int type, long timestamp, String[] args) {
        if (!mCurrentNotificationId.equals(REWARDS_PROMOTION_CLAIM_ERROR_ID)) {
            showNotification(id, type, timestamp, args);
        }
    }

    @Override
    public void OnNotificationDeleted(String id) {
        dismissNotification(id);
    }

    @Override
    public void OnGetExternalWallet(String externalWallet) {
        if (!TextUtils.isEmpty(externalWallet)) {
            try {
                mExternalWallet = new BraveRewardsExternalWallet(externalWallet);
            } catch (JSONException e) {
                mExternalWallet = null;
            }
        }
        if (mExternalWallet != null) {
            mBraveRewardsNativeWorker.GetRewardsParameters();
        } else {
            newInstallViewChanges();
        }
    }

    private void showViewsBasedOnExternalWallet() {
        int walletStatus = mExternalWallet.getStatus();
        if (PackageUtils.isFirstInstall(mActivity)) {
            if (walletStatus == WalletStatus.CONNECTED || walletStatus == WalletStatus.LOGGED_OUT) {
                existingUserViewChanges();
            } else if (walletStatus == WalletStatus.NOT_CONNECTED) {
                newInstallViewChanges();
            }
        } else {
            existingUserViewChanges();
        }
    }

    // Generic UI changes
    OnCheckedChangeListener autoContributeSwitchListener = new OnCheckedChangeListener() {
        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            mBraveRewardsNativeWorker.IncludeInAutoContribution(mCurrentTabId, !isChecked);
        }
    };

    View.OnClickListener braveRewardsOnboardingClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            int viewId = view.getId();
            if (viewId == R.id.btn_start_quick_tour) {
                if (mBraveRewardsViewPager != null
                        && mBraveRewardsViewPager.getCurrentItem() == 0) {
                    mBraveRewardsViewPager.setCurrentItem(
                            mBraveRewardsViewPager.getCurrentItem() + 1);
                }
            }

            if (viewId == R.id.btn_next) {
                if (mBraveRewardsViewPager != null && mBraveRewardsOnboardingPagerAdapter != null) {
                    if (mBraveRewardsViewPager.getCurrentItem()
                                    == mBraveRewardsOnboardingPagerAdapter.getCount() - 2
                            && mBraveRewardsNativeWorker.canConnectAccount()) {
                        if (mBraveRewardsOnboardingView != null) {
                            mBraveRewardsOnboardingView.setVisibility(View.GONE);
                        }
                        showConnectAccountModal();
                    } else if (mBraveRewardsViewPager.getCurrentItem()
                            == mBraveRewardsOnboardingPagerAdapter.getCount() - 1) {
                        if (mBraveRewardsOnboardingView != null) {
                            mBraveRewardsOnboardingView.setVisibility(View.GONE);

                            if (mPopupView != null && mBraveRewardsNativeWorker.IsSupported()
                                    && BraveRewardsHelper.shouldShowBraveRewardsOnboardingModal()
                                    && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(
                                            Profile.getLastUsedRegularProfile())) {
                                showBraveRewardsOnboardingModal();
                            } else {
                                // fetchRewardsData();
                                mBraveRewardsNativeWorker.GetExternalWallet();
                                panelShadow(false);
                            }
                        }
                    } else {
                        mBraveRewardsViewPager.setCurrentItem(
                                mBraveRewardsViewPager.getCurrentItem() + 1);
                    }
                }
            }

            if (viewId == R.id.btn_skip && mBraveRewardsOnboardingView != null) {
                mBraveRewardsViewPager.setCurrentItem(
                        mBraveRewardsOnboardingPagerAdapter.getCount() - 1);
            }

            if (viewId == R.id.btn_go_back && mBraveRewardsViewPager != null) {
                mBraveRewardsViewPager.setCurrentItem(mBraveRewardsViewPager.getCurrentItem() - 1);
            }
        }
    };

    private void newInstallViewChanges() {
        showUnverifiedLayout();
        checkForRewardsOnboarding();
        String rewardsCountryCode = UserPrefs.get(Profile.getLastUsedRegularProfile())
                                            .getString(BravePref.DECLARED_GEO);

        if (mPopupView != null && mBraveRewardsNativeWorker.IsSupported()) {
            if (!BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedRegularProfile())
                    && BraveRewardsHelper.shouldShowBraveRewardsOnboardingModal()) {
                showBraveRewardsOnboardingModal();
            } else if (TextUtils.isEmpty(rewardsCountryCode)) {
                mBraveRewardsNativeWorker.getAvailableCountries();
            }
        }
    }

    private void existingUserViewChanges() {
        showRewardsMainUI();
        requestPublisherInfo();
        setNotificationsControls();
        createUpdateBalanceTask();
        mWalletBalanceLayout.setAlpha(0.4f);
        mWalletBalanceProgress.setVisibility(View.VISIBLE);
        mBraveRewardsNativeWorker.fetchBalance();
        mBraveRewardsNativeWorker.GetRecurringDonations();
        mBraveRewardsNativeWorker.GetAutoContributeProperties();
        mAdsStatementLayout.setAlpha(0.4f);
        mAdsStatementProgress.setVisibility(View.VISIBLE);
        mBraveRewardsNativeWorker.getAdsAccountStatement();
        mBraveRewardsNativeWorker.GetCurrentBalanceReport();
        mBraveRewardsNativeWorker.GetAllNotifications();
        setVerifyWalletButton();
        showRewardsFromAdsSummary();
    }

    private void showUnverifiedLayout() {
        View unverifiedStateLayout =
                mPopupView.findViewById(R.id.brave_rewards_panel_unverified_layout_id);
        unverifiedStateLayout.setVisibility(View.VISIBLE);
        TextView unverifiedToggleSubText =
                unverifiedStateLayout.findViewById(R.id.unverified_toggle_sub_text);
        SwitchCompat braveRewardsSwitch =
                unverifiedStateLayout.findViewById(R.id.brave_rewards_switch);
        braveRewardsSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                BraveAdsNativeHelper.nativeSetAdsEnabled(
                        Profile.getLastUsedRegularProfile(), isChecked);
                showUnverifiedLayout();
            }
        });
        View rewardsPanelUnverifiedOnSection =
                unverifiedStateLayout.findViewById(R.id.rewards_panel_unverified_on_section);
        TextView connectAccountButton =
                unverifiedStateLayout.findViewById(R.id.connect_account_button);
        connectAccountButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                TabUtils.openUrlInNewTab(
                        false, BraveActivity.BRAVE_REWARDS_SETTINGS_WALLET_VERIFICATION_URL);
                dismiss();
            }
        }));
        View rewardsPanelUnverifiedCreatorSection =
                unverifiedStateLayout.findViewById(R.id.rewards_panel_unverified_creator_section);
        TextView rewardsPanelUnverifiedCreatorCountText = unverifiedStateLayout.findViewById(
                R.id.rewards_panel_unverified_creator_count_text);
        TextView rewardsPanelUnverifiedCreatorText =
                unverifiedStateLayout.findViewById(R.id.rewards_panel_unverified_creator_text);
        View rewardsPanelUnverifiedOffSection =
                unverifiedStateLayout.findViewById(R.id.rewards_panel_unverified_off_section);
        LinearLayout rewardsSettingsButton =
                unverifiedStateLayout.findViewById(R.id.rewards_settings_button);
        rewardsSettingsButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mBraveActivity.openNewOrSelectExistingTab(BraveActivity.BRAVE_REWARDS_SETTINGS_URL);
                dismiss();
            }
        }));
        TextView learnMoreUnverifiedText =
                unverifiedStateLayout.findViewById(R.id.learn_more_unverified_text);
        learnMoreUnverifiedText.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                CustomTabActivity.showInfoPage(mActivity, BRAVE_REWARDS_PAGE);
            }
        }));

        if (BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedRegularProfile())) {
            if (mRewardsMainLayout != null) {
                mRewardsMainLayout.setVisibility(View.GONE);
            }
            unverifiedToggleSubText.setText(mPopupView.getResources().getString(
                    R.string.rewards_panel_unverified_switch_on_sub_text));
            braveRewardsSwitch.setChecked(true);
            rewardsPanelUnverifiedOnSection.setVisibility(View.VISIBLE);
            TextView rewardsPanelUnverifiedOnSectionText = unverifiedStateLayout.findViewById(
                    R.id.rewards_panel_unverified_on_section_text);
            TextView rewardsPanelUnverifiedOnSectionLearnMoreText =
                    unverifiedStateLayout.findViewById(
                            R.id.rewards_panel_unverified_on_section_learn_more_text);
            rewardsPanelUnverifiedOnSectionLearnMoreText.setOnClickListener(
                    (new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            CustomTabActivity.showInfoPage(
                                    mActivity, UNVERIFIED_USER_UNSUPPORTED_REGION_PAGE);
                        }
                    }));
            if (mBraveRewardsNativeWorker.canConnectAccount()) {
                String sectionText = "<b>"
                        + mPopupView.getResources().getString(R.string.ready_to_start_earning_text)
                        + "</b> "
                        + mPopupView.getResources().getString(
                                R.string.rewards_panel_unverified_on_section_text);
                rewardsPanelUnverifiedOnSectionText.setText(Html.fromHtml(sectionText));
                connectAccountButton.setVisibility(View.VISIBLE);
                rewardsPanelUnverifiedOnSectionLearnMoreText.setVisibility(View.GONE);
            } else {
                rewardsPanelUnverifiedOnSectionText.setText(mPopupView.getResources().getString(
                        R.string.rewards_panel_unverified_on_section_unsupported_region_text));
                connectAccountButton.setVisibility(View.GONE);
                rewardsPanelUnverifiedOnSectionLearnMoreText.setVisibility(View.VISIBLE);
            }
            rewardsPanelUnverifiedOffSection.setVisibility(View.GONE);
            mBraveRewardsNativeWorker.getPublishersVisitedCount();
        } else {
            if (mRewardsMainLayout != null) {
                mRewardsMainLayout.setVisibility(View.GONE);
            }
            unverifiedToggleSubText.setText(mPopupView.getResources().getString(
                    R.string.rewards_panel_unverified_switch_off_sub_text));
            braveRewardsSwitch.setChecked(false);
            rewardsPanelUnverifiedOnSection.setVisibility(View.GONE);
            rewardsPanelUnverifiedOffSection.setVisibility(View.VISIBLE);
            rewardsPanelUnverifiedCreatorSection.setVisibility(View.GONE);
        }
    }

    // Onboarding changes
    private void checkForRewardsOnboarding() {
        if (mPopupView != null && mBraveRewardsNativeWorker.IsSupported()
                && (BraveRewardsHelper.shouldShowBraveRewardsOnboardingOnce()
                        || BraveRewardsHelper.shouldShowDeclareGeoModal())) {
            if (BraveRewardsHelper.shouldShowBraveRewardsOnboardingOnce()) {
                showBraveRewardsOnboarding(false);
                BraveRewardsHelper.setShowBraveRewardsOnboardingOnce(false);
            } else if (BraveRewardsHelper.shouldShowDeclareGeoModal()) {
                mBraveRewardsNativeWorker.getAvailableCountries();
                BraveRewardsHelper.setShowDeclareGeoModal(false);
            }
        }
    }

    private void showBraveRewardsOnboardingModal() {
        if (mBraveRewardsOnboardingModalView == null) {
            return;
        }
        mBraveRewardsOnboardingModalView.setVisibility(View.VISIBLE);
        panelShadow(true);

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

        TextView tosAndPpText = mBraveRewardsOnboardingModalView.findViewById(
                R.id.brave_rewards_onboarding_modal_tos_pp_text);
        tosAndPpText.setMovementMethod(LinkMovementMethod.getInstance());
        tosAndPpText.setText(tosTextSS);

        TextView takeQuickTourButton =
                mBraveRewardsOnboardingModalView.findViewById(R.id.take_quick_tour_button);
        takeQuickTourButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mBraveRewardsOnboardingModalView.setVisibility(View.GONE);
                showBraveRewardsOnboarding(false);
            }
        }));

        TextView btnBraveRewards =
                mBraveRewardsOnboardingModalView.findViewById(R.id.start_using_brave_rewards_text);
        btnBraveRewards.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mBraveRewardsNativeWorker.getAvailableCountries();
            }
        }));
    }

    private void showBraveRewardsOnboarding(boolean shouldShowMoreOption) {
        if (mBraveRewardsOnboardingView == null) {
            return;
        }

        panelShadow(true);

        mBraveRewardsOnboardingView.setVisibility(View.VISIBLE);
        final Button btnNext = mBraveRewardsOnboardingView.findViewById(R.id.btn_next);
        btnNext.setOnClickListener(braveRewardsOnboardingClickListener);
        mBraveRewardsOnboardingView.findViewById(R.id.btn_go_back)
                .setOnClickListener(braveRewardsOnboardingClickListener);
        mBraveRewardsOnboardingView.findViewById(R.id.btn_skip)
                .setOnClickListener(braveRewardsOnboardingClickListener);
        mBraveRewardsOnboardingView.findViewById(R.id.btn_start_quick_tour)
                .setOnClickListener(braveRewardsOnboardingClickListener);

        mBraveRewardsViewPager =
                mBraveRewardsOnboardingView.findViewById(R.id.brave_rewards_view_pager);
        mBraveRewardsViewPager.addOnPageChangeListener(new ViewPager.OnPageChangeListener() {
            @Override
            public void onPageScrolled(
                    int position, float positionOffset, int positionOffsetPixels) {
                if (positionOffset == 0 && positionOffsetPixels == 0 && position == 0) {
                    mBraveRewardsOnboardingView.findViewById(R.id.onboarding_first_screen_layout)
                            .setVisibility(View.VISIBLE);
                    mBraveRewardsOnboardingView.findViewById(R.id.onboarding_action_layout)
                            .setVisibility(View.GONE);
                } else {
                    mBraveRewardsOnboardingView.findViewById(R.id.onboarding_action_layout)
                            .setVisibility(View.VISIBLE);
                    mBraveRewardsOnboardingView.findViewById(R.id.onboarding_first_screen_layout)
                            .setVisibility(View.GONE);
                }
            }

            @Override
            public void onPageSelected(int position) {
                if (mBraveRewardsOnboardingPagerAdapter != null
                        && position == mBraveRewardsOnboardingPagerAdapter.getCount() - 1) {
                    btnNext.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
                    btnNext.setText(mActivity.getResources().getString(R.string.done));
                } else {
                    btnNext.setText(mActivity.getResources().getString(R.string.continue_text));
                    btnNext.setCompoundDrawablesWithIntrinsicBounds(
                            0, 0, R.drawable.ic_arrow_circle_right, 0);
                }
            }

            @Override
            public void onPageScrollStateChanged(int state) {}
        });
        mBraveRewardsOnboardingPagerAdapter = new BraveRewardsOnboardingPagerAdapter();
        mBraveRewardsOnboardingPagerAdapter.setOnboardingType(shouldShowMoreOption);
        mBraveRewardsViewPager.setAdapter(mBraveRewardsOnboardingPagerAdapter);
        TabLayout braveRewardsTabLayout =
                mBraveRewardsOnboardingView.findViewById(R.id.brave_rewards_tab_layout);
        braveRewardsTabLayout.setupWithViewPager(mBraveRewardsViewPager, true);
        AppCompatImageView modalCloseButton = mBraveRewardsOnboardingView.findViewById(
                R.id.brave_rewards_onboarding_layout_modal_close);
        modalCloseButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mBraveRewardsOnboardingView.setVisibility(View.GONE);
                panelShadow(false);
                mBraveRewardsNativeWorker.GetExternalWallet();
            }
        }));
        mBraveRewardsOnboardingView.findViewById(R.id.onboarding_first_screen_layout)
                .setVisibility(View.VISIBLE);
        mBraveRewardsOnboardingView.findViewById(R.id.onboarding_action_layout)
                .setVisibility(View.GONE);
    }

    private void panelShadow(boolean isEnable) {
        if (isEnable) {
            if (mBravePanelShadow != null) {
                mBravePanelShadow.setVisibility(View.VISIBLE);
            }

            if (mBraveRewardsUnverifiedView != null) {
                enableControls(false, mBraveRewardsUnverifiedView);
            }
        } else {
            if (mBravePanelShadow != null) {
                mBravePanelShadow.setVisibility(View.GONE);
            }

            if (mBraveRewardsUnverifiedView != null) {
                enableControls(true, mBraveRewardsUnverifiedView);
            }
        }
    }

    // Declare geo changes
    private void showDeclareGeoModal(String[] countries) {
        showBraveRewardsOnboardingModal();
        if (mBraveRewardsOnboardingModalView != null) {
            TextView modalTitle = mBraveRewardsOnboardingModalView.findViewById(R.id.modal_title);
            TextView modalText = mBraveRewardsOnboardingModalView.findViewById(R.id.modal_text);
            TextView btnContinue = mBraveRewardsOnboardingModalView.findViewById(R.id.btn_continue);
            modalTitle.setText(mActivity.getString(R.string.select_your_country_title));
            modalTitle.setCompoundDrawablesWithIntrinsicBounds(R.drawable.ic_location, 0, 0, 0);
            String declareGeoText = String.format(
                    mActivity.getResources().getString(R.string.select_your_country_text),
                    mActivity.getResources().getString(R.string.privacy_policy));
            int privacyPolicyTextIndex = declareGeoText.indexOf(
                    mActivity.getResources().getString(R.string.privacy_policy));
            Spanned declareGeoTextSpanned =
                    BraveRewardsHelper.spannedFromHtmlString(declareGeoText);
            SpannableString declareGeoTextSS =
                    new SpannableString(declareGeoTextSpanned.toString());

            ClickableSpan declareGeoClickableSpan = new ClickableSpan() {
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

            declareGeoTextSS.setSpan(declareGeoClickableSpan, privacyPolicyTextIndex,
                    privacyPolicyTextIndex
                            + mActivity.getResources().getString(R.string.privacy_policy).length(),
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            declareGeoTextSS.setSpan(new ForegroundColorSpan(mActivity.getResources().getColor(
                                             R.color.brave_rewards_modal_theme_color)),
                    privacyPolicyTextIndex,
                    privacyPolicyTextIndex
                            + mActivity.getResources().getString(R.string.privacy_policy).length(),
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            modalText.setMovementMethod(LinkMovementMethod.getInstance());
            modalText.setText(declareGeoTextSS);
            mBraveRewardsOnboardingModalView.findViewById(R.id.take_quick_tour_button)
                    .setVisibility(View.GONE);
            mBraveRewardsOnboardingModalView.findViewById(R.id.start_using_brave_rewards_text)
                    .setVisibility(View.GONE);
            btnContinue.setVisibility(View.VISIBLE);

            TreeMap<String, String> sortedCountryMap = new TreeMap<String, String>();
            for (String countryCode : countries) {
                sortedCountryMap.put(new Locale("", countryCode).getDisplayCountry(), countryCode);
            }

            ArrayList<String> countryList = new ArrayList<String>();
            countryList.add(mActivity.getResources().getString(R.string.select_your_country_title));
            countryList.addAll(sortedCountryMap.keySet());
            String[] countryArray = countryList.toArray(new String[countryList.size()]);
            Spinner countrySpinner;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                countrySpinner =
                        mBraveRewardsOnboardingModalView.findViewById(R.id.country_spinner);
            } else {
                countrySpinner = mBraveRewardsOnboardingModalView.findViewById(
                        R.id.country_spinner_low_device);
            }
            countrySpinner.setVisibility(View.VISIBLE);
            ArrayAdapter countryArrayAdapter =
                    new ArrayAdapter(mActivity, android.R.layout.simple_spinner_item, countryArray);
            countryArrayAdapter.setDropDownViewResource(
                    android.R.layout.simple_spinner_dropdown_item);
            countrySpinner.setAdapter(countryArrayAdapter);
            countrySpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
                    if (pos != 0) {
                        btnContinue.setBackgroundDrawable(ResourcesCompat.getDrawable(
                                ContextUtils.getApplicationContext().getResources(),
                                R.drawable.blue_48_rounded_bg, /* theme= */ null));
                        btnContinue.setEnabled(true);
                    } else {
                        btnContinue.setBackgroundDrawable(ResourcesCompat.getDrawable(
                                ContextUtils.getApplicationContext().getResources(),
                                R.drawable.set_default_rounded_button_disabled, /* theme= */ null));
                        btnContinue.setEnabled(false);
                    }
                }
                @Override
                public void onNothingSelected(AdapterView<?> arg0) {}
            });

            btnContinue.setOnClickListener((new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (!BravePermissionUtils.hasPermission(mAnchorView.getContext(),
                                PermissionConstants.NOTIFICATION_PERMISSION)
                            || BravePermissionUtils.isBraveAdsNotificationPermissionBlocked(
                                    mAnchorView.getContext())) {
                        requestNotificationPermission();
                    }
                    if (countrySpinner != null) {
                        mBraveRewardsNativeWorker.CreateRewardsWallet(
                                sortedCountryMap.get(countrySpinner.getSelectedItem().toString()));
                    }
                }
            }));
        }
    }

    private void showVbatExpireNotice() {
        if (mRewardsVbatExpireNoticeModal == null) {
            return;
        }

        panelShadow(true);

        mRewardsVbatExpireNoticeModal.setVisibility(View.VISIBLE);
        TextView vBatModalTitle = mRewardsVbatExpireNoticeModal.findViewById(R.id.vbat_modal_title);
        TextView vBatModalText = mRewardsVbatExpireNoticeModal.findViewById(R.id.vbat_modal_text);
        FrameLayout vBatConnectButton =
                mRewardsVbatExpireNoticeModal.findViewById(R.id.btn_vbat_connect_account);
        vBatConnectButton.setVisibility(
                mBraveRewardsNativeWorker.canConnectAccount() ? View.VISIBLE : View.GONE);
        vBatConnectButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                TabUtils.openUrlInNewTab(
                        false, BraveActivity.BRAVE_REWARDS_SETTINGS_WALLET_VERIFICATION_URL);
                dismiss();
            }
        }));
        Button vBatLearnMoreButton =
                mRewardsVbatExpireNoticeModal.findViewById(R.id.btn_vbat_learn_more);
        vBatLearnMoreButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                CustomTabActivity.showInfoPage(mActivity, BRAVE_REWARDS_CHANGES_PAGE);
            }
        }));
        AppCompatImageView vbatCloseBtn =
                mRewardsVbatExpireNoticeModal.findViewById(R.id.vbat_modal_close);
        vbatCloseBtn.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mRewardsVbatExpireNoticeModal != null) {
                    mRewardsVbatExpireNoticeModal.setVisibility(View.GONE);
                }
                panelShadow(false);
            }
        }));

        String vbatModalTitleString = "";
        String vbatModalTextString = "";
        double dueDateInMillis = mBraveRewardsNativeWorker.getVbatDeadline();
        SimpleDateFormat sdf = new SimpleDateFormat("MMMM dd, yyyy h:mm a", Locale.getDefault());
        String dueDate = sdf.format(new Date((long) dueDateInMillis));
        if (mBraveRewardsNativeWorker.canConnectAccount()) {
            vbatModalTitleString = mActivity.getString(R.string.vbat_supported_region_title);
            vbatModalTextString = String.format(
                    mActivity.getString(R.string.vbat_supported_region_text), dueDate);
        } else {
            vbatModalTitleString = mActivity.getString(R.string.vbat_unsupported_region_title);
            vbatModalTextString =
                    String.format(mActivity.getString(R.string.vbat_unsupported_region_text),
                            dueDate, mBraveRewardsNativeWorker.getCountryCode());
        }
        vBatModalTitle.setText(vbatModalTitleString);
        vBatModalText.setText(vbatModalTextString);
    }

    private void showConnectAccountModal() {
        if (mConnectAccountModal == null) {
            return;
        }
        mConnectAccountModal.setVisibility(View.VISIBLE);

        Button connectAccountBtn = mConnectAccountModal.findViewById(R.id.btn_connect_account);
        connectAccountBtn.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                TabUtils.openUrlInNewTab(
                        false, BraveActivity.BRAVE_REWARDS_SETTINGS_WALLET_VERIFICATION_URL);
                dismiss();
            }
        }));
        Button dismissBtn = mConnectAccountModal.findViewById(R.id.btn_do_it_later);
        dismissBtn.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mBraveRewardsOnboardingView != null) {
                    mBraveRewardsOnboardingView.setVisibility(View.GONE);
                }
                mConnectAccountModal.setVisibility(View.GONE);
                panelShadow(false);
                mBraveRewardsNativeWorker.GetExternalWallet();
            }
        }));
        AppCompatImageView closeBtn =
                mConnectAccountModal.findViewById(R.id.connect_account_layout_modal_close);
        closeBtn.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mBraveRewardsOnboardingView != null) {
                    mBraveRewardsOnboardingView.setVisibility(View.GONE);
                }
                mConnectAccountModal.setVisibility(View.GONE);
                panelShadow(false);
                mBraveRewardsNativeWorker.GetExternalWallet();
            }
        }));
    }

    private void showRewardsResponseModal(boolean isSuccess, String errorMessage) {
        if (mRewardsResponseModal == null) {
            return;
        }
        mRewardsResponseModal.setVisibility(View.VISIBLE);

        TextView responseModalTitle = mRewardsResponseModal.findViewById(R.id.response_modal_title);
        TextView responseModalText = mRewardsResponseModal.findViewById(R.id.response_modal_text);
        TextView responseErrorText = mRewardsResponseModal.findViewById(R.id.response_error_text);
        TextView responseRewardsBtn = mRewardsResponseModal.findViewById(R.id.response_action_btn);
        AppCompatImageView responseCloseBtn =
                mRewardsResponseModal.findViewById(R.id.response_modal_close);
        if (isSuccess) {
            responseModalTitle.setText(mActivity.getString(R.string.thank_you));
            responseModalTitle.setCompoundDrawablesWithIntrinsicBounds(
                    0, R.drawable.checked_circle_filled, 0, 0);
            responseModalText.setText(
                    String.format(mActivity.getString(R.string.declare_geo_success_response_text),
                            new Locale("", mBraveRewardsNativeWorker.getCountryCode())
                                    .getDisplayCountry()));
            responseRewardsBtn.setText(mActivity.getString(R.string.close_text));
        } else {
            String title = mActivity.getString(R.string.something_went_wrong_text);
            String text = mActivity.getString(R.string.declare_geo_failed_response_text);
            String actionText = mActivity.getString(R.string.retry_text);
            if (errorMessage.equals(WALLET_GENERATION_DISABLED_ERROR)) {
                title = mActivity.getString(R.string.wallet_generation_disabled_error_title);
                text = String.format(
                        mActivity.getString(R.string.wallet_generation_disabled_error_text),
                        mActivity.getResources().getString(R.string.learn_more));
                Spanned textToAgree = BraveRewardsHelper.spannedFromHtmlString(text);
                SpannableString ss = new SpannableString(textToAgree.toString());

                ClickableSpan clickableSpan = new ClickableSpan() {
                    @Override
                    public void onClick(@NonNull View textView) {
                        CustomTabActivity.showInfoPage(mActivity, NEW_SIGNUP_DISABLED_URL);
                    }
                    @Override
                    public void updateDrawState(@NonNull TextPaint ds) {
                        super.updateDrawState(ds);
                        ds.setUnderlineText(false);
                    }
                };
                int learnMoreIndex =
                        text.indexOf(mActivity.getResources().getString(R.string.learn_more));

                ss.setSpan(clickableSpan, learnMoreIndex,
                        learnMoreIndex
                                + mActivity.getResources().getString(R.string.learn_more).length(),
                        Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

                ForegroundColorSpan foregroundSpan = new ForegroundColorSpan(
                        mActivity.getResources().getColor(R.color.brave_rewards_modal_theme_color));
                ss.setSpan(foregroundSpan, learnMoreIndex,
                        learnMoreIndex
                                + mActivity.getResources().getString(R.string.learn_more).length(),
                        Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                responseModalText.setMovementMethod(LinkMovementMethod.getInstance());
                responseModalText.setText(ss);
                actionText = mActivity.getString(R.string.close_text);
            } else {
                responseModalText.setText(text);
            }
            responseModalTitle.setText(title);
            responseModalTitle.setCompoundDrawablesWithIntrinsicBounds(
                    0, R.drawable.ic_warning_circle_filled, 0, 0);
            responseRewardsBtn.setText(actionText);
            responseErrorText.setText(errorMessage);
        }

        responseRewardsBtn.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mRewardsResponseModal.setVisibility(View.GONE);
                mRewardsMainLayout.setForeground(null);
                panelShadow(false);
                if (!isSuccess && !errorMessage.equals(WALLET_GENERATION_DISABLED_ERROR)) {
                    showBraveRewardsOnboardingModal();
                }
            }
        }));

        responseCloseBtn.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mRewardsResponseModal.setVisibility(View.GONE);
                mRewardsMainLayout.setForeground(null);
            }
        }));
    }

    @Override
    public void onGetPublishersVisitedCount(int count) {
        if (mPopupView != null && count > 0) {
            mPopupView.findViewById(R.id.rewards_panel_unverified_creator_section)
                    .setVisibility(View.VISIBLE);
            TextView rewardsPanelUnverifiedCreatorCountText =
                    mPopupView.findViewById(R.id.rewards_panel_unverified_creator_count_text);
            rewardsPanelUnverifiedCreatorCountText.setText(String.valueOf(count));
            TextView rewardsPanelUnverifiedCreatorText =
                    mPopupView.findViewById(R.id.rewards_panel_unverified_creator_text);
            rewardsPanelUnverifiedCreatorText.setText(
                    String.format(mPopupView.getResources().getString(
                                          R.string.rewards_panel_unverified_creator_section_text),
                            count));
        }
    }

    private String getWalletString(String walletType) {
        if (walletType.equals(BraveWalletProvider.UPHOLD)) {
            return mActivity.getResources().getString(R.string.uphold);
        } else if (walletType.equals(BraveWalletProvider.GEMINI)) {
            return mActivity.getResources().getString(R.string.gemini);
        } else {
            return mActivity.getResources().getString(R.string.bitflyer);
        }
    }

    private void showRewardsFromAdsSummary() {
        if (mExternalWallet == null) {
            return;
        }
        int walletStatus = mExternalWallet.getStatus();
        if (mBraveRewardsNativeWorker != null) {
            String walletType = mBraveRewardsNativeWorker.getExternalWalletType();
            if (walletStatus == WalletStatus.CONNECTED
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
            if (walletStatus == WalletStatus.LOGGED_OUT) {
                mPopupView.findViewById(R.id.rewards_from_ads_summary_layout)
                        .setVisibility(View.GONE);
            }
        }
    }

    private void setVerifyWalletButton() {
        if (mExternalWallet == null) {
            return;
        }
        int status = mExternalWallet.getStatus();
        TextView btnVerifyWallet = mPopupView.findViewById(R.id.btn_verify_wallet);
        ImageView verifyWalletArrowImg = mPopupView.findViewById(R.id.verify_wallet_arrow_img);
        SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor editor = sharedPref.edit();
        int rightDrawable = 0;
        int textId = 0;
        String walletType = mBraveRewardsNativeWorker.getExternalWalletType();

        switch (status) {
            case WalletStatus.NOT_CONNECTED:
                rightDrawable = R.drawable.ic_verify_wallet_arrow;
                textId = R.string.brave_ui_wallet_button_connect;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                break;
            case WalletStatus.CONNECTED:
                editor.putBoolean(PREF_VERIFY_WALLET_ENABLE, true);
                editor.apply();

                rightDrawable = getWalletIcon(walletType);
                textId = R.string.brave_ui_wallet_button_connected;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                btnVerifyWallet.setBackgroundColor(Color.TRANSPARENT);
                verifyWalletArrowImg.setVisibility(View.VISIBLE);
                break;
            case WalletStatus.LOGGED_OUT:
                rightDrawable = getWalletIcon(walletType);
                textId = R.string.brave_ui_wallet_button_logged_out;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                btnVerifyWallet.setBackgroundColor(Color.TRANSPARENT);
                verifyWalletArrowImg.setVisibility(View.VISIBLE);
                break;
            default:
                Log.e(TAG, "Unexpected external wallet status");
                return;
        }

        btnVerifyWallet.setVisibility(View.VISIBLE);
        btnVerifyWallet.setText(mPopupView.getResources().getString(textId));
        setVerifyWalletButtonClickEvent(btnVerifyWallet, status);
    }

    private void setVerifyWalletButtonClickEvent(View btnVerifyWallet, final int status) {
        btnVerifyWallet.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                BraveRewardsBalance walletBalanceObject =
                        mBraveRewardsNativeWorker.GetWalletBalance();
                double walletBalance = 0;
                if (walletBalanceObject != null) {
                    walletBalance = walletBalanceObject.getTotal();
                }

                switch (status) {
                    case WalletStatus.NOT_CONNECTED:
                        TabUtils.openUrlInNewTab(false,
                                BraveActivity.BRAVE_REWARDS_SETTINGS_WALLET_VERIFICATION_URL);
                        dismiss();
                        break;
                    case WalletStatus.CONNECTED:
                        openUserWalletActivity(status);
                        break;
                    case WalletStatus.LOGGED_OUT:
                        openUserWalletActivity(status);
                        break;
                    default:
                        Log.e(TAG, "Unexpected external wallet status");
                        return;
                }
            }
        }));
    }

    private void openUserWalletActivity(int walletStatus) {
        int requestCode = BraveConstants.USER_WALLET_ACTIVITY_REQUEST_CODE;
        Intent intent = BuildVerifyWalletActivityIntent(walletStatus);
        if (intent != null) {
            mActivity.startActivityForResult(intent, requestCode);
        }
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
        String url = currentActiveTab != null ? currentActiveTab.getUrl().getSpec() : "";
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
            String amount = String.format(
                    mPopupView.getResources().getString(R.string.brave_rewards_bat_value_text),
                    String.format(Locale.getDefault(), "%.2f", recurrentAmount));
            monthlyTipText.setText(amount);
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
        mActivity.startActivityForResult(intent, BraveConstants.MONTHLY_CONTRIBUTION_REQUEST_CODE);
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

    private Intent BuildVerifyWalletActivityIntent(final int status) {
        Class clazz = null;
        switch (status) {
            case WalletStatus.CONNECTED:
            case WalletStatus.LOGGED_OUT:
                clazz = BraveRewardsUserWalletActivity.class;
                break;
            default:
                Log.e(TAG, "Unexpected external wallet status");
                return null;
        }

        Intent intent = new Intent(ContextUtils.getApplicationContext(), clazz);
        intent.putExtra(BraveRewardsExternalWallet.ACCOUNT_URL, mExternalWallet.getAccountUrl());
        intent.putExtra(BraveRewardsExternalWallet.ADDRESS, mExternalWallet.getAddress());
        intent.putExtra(BraveRewardsExternalWallet.LOGIN_URL, mExternalWallet.getLoginUrl());
        intent.putExtra(BraveRewardsExternalWallet.STATUS, mExternalWallet.getStatus());
        intent.putExtra(BraveRewardsExternalWallet.TOKEN, mExternalWallet.getToken());
        intent.putExtra(BraveRewardsExternalWallet.USER_NAME, mExternalWallet.getUserName());
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

    private int getWalletIcon(String walletType) {
        if (walletType.equals(BraveWalletProvider.UPHOLD)) {
            return R.drawable.uphold_white;
        } else if (walletType.equals(BraveWalletProvider.GEMINI)) {
            return R.drawable.ic_gemini_logo_white;
        } else {
            return R.drawable.ic_logo_bitflyer;
        }
    }
}
