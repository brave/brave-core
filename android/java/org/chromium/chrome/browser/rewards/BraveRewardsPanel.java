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
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.cardview.widget.CardView;
import androidx.core.content.res.ResourcesCompat;
import androidx.appcompat.widget.SwitchCompat;

import org.json.JSONException;

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
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;
import java.util.Date;
import java.util.Calendar;

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

    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;

    private int mCurrentTabId;
    private boolean mAutoContributeEnabled;
    private TextView mPublisherAttention;

    private BraveRewardsExternalWallet mExternalWallet;

    public BraveRewardsPanel(View anchorView) {
        // currentNotificationId = "";
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
            mActivity.startActivity(intent);
            // mActivity.startActivityForResult(intent, BraveActivity.SITE_BANNER_REQUEST_CODE);

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
        mBtnTip.setOnClickListener(view -> {
            showTipSection();
        });

        mBtnSummary = mPopupView.findViewById(R.id.summary_btn);
        mImgSummary = mPopupView.findViewById(R.id.summary_img);
        mTextSummary = mPopupView.findViewById(R.id.summary_text);
        mBtnSummary.setOnClickListener(view -> {
            showSummarySection();
        });

        mSwitchAutoContribute = mPopupView.findViewById(R.id.auto_contribution_switch);
        mSwitchAutoContribute.setOnCheckedChangeListener(autoContributeSwitchListener);

        showSummarySection();
        mBtnTip.setEnabled(false);

        String monthName=(String)android.text.format.DateFormat.format("MMM", new Date());
        int lastDate = Calendar.getInstance().getActualMaximum(Calendar.DAY_OF_MONTH);
        int currentYear = Calendar.getInstance().get(Calendar.YEAR);

        String adsMonthlyStatement = new StringBuilder(monthName).append(" 1 - ").append(monthName).append(" ").append(lastDate).toString();
        String monthYear = new StringBuilder(monthName).append(" ").append(currentYear).toString();

        TextView adsMonthlyStatementText = mPopupView.findViewById(R.id.ads_monthly_statement_text);
        adsMonthlyStatementText.setText(adsMonthlyStatement);

        TextView monthYearText = mPopupView.findViewById(R.id.month_year_text);
        monthYearText.setText(monthYear);

        mPopupWindow.setContentView(mPopupView);
    }

    private void showSummarySection() {
        mTextTip.setTextColor(Color.parseColor("#868E96"));
        mImgTip.setColorFilter(new PorterDuffColorFilter(
                            Color.parseColor("#868E96"), PorterDuff.Mode.SRC_IN));
            

            mTextSummary.setTextColor(Color.parseColor("#4C54D2"));
            mImgSummary.setColorFilter(new PorterDuffColorFilter(
                            Color.parseColor("#4C54D2"), PorterDuff.Mode.SRC_IN));

            mRewardsSummaryDetailLayout.setVisibility(View.VISIBLE);
            mRewardsTipLayout.setVisibility(View.GONE);
    }

    private void showTipSection() {
        mTextTip.setTextColor(Color.parseColor("#4C54D2"));
        mImgTip.setColorFilter(new PorterDuffColorFilter(
                            Color.parseColor("#4C54D2"), PorterDuff.Mode.SRC_IN));

            mTextSummary.setTextColor(Color.parseColor("#868E96"));
            mImgSummary.setColorFilter(new PorterDuffColorFilter(
                            Color.parseColor("#868E96"), PorterDuff.Mode.SRC_IN));
            mRewardsSummaryDetailLayout.setVisibility(View.GONE);
            mRewardsTipLayout.setVisibility(View.VISIBLE);
    }



    OnCheckedChangeListener autoContributeSwitchListener = new OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView,
                                             boolean isChecked) {
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
    public void OnGetAdsAccountStatement(boolean success, double nextPaymentDate,
          int adsReceivedThisMonth, double earningsThisMonth, double earningsLastMonth){
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
                Log.e(TAG, "Wallet balance : "+walletBalance);
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
                Log.e(TAG, "USd amount : "+ usdText);
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
        if (mBraveRewardsNativeWorker !=null && mBraveRewardsNativeWorker.IsAutoContributeEnabled()) {
            mPopupView.findViewById(R.id.attention_layout).setVisibility(View.VISIBLE);
            mPopupView.findViewById(R.id.auto_contribution_layout).setVisibility(View.VISIBLE);
        }
        mBraveRewardsNativeWorker.GetRecurringDonations();
    }

    @Override
    public void OnOneTimeTip() {
        //TODO add logic for refresh balance
        // mBraveRewardsNativeWorker.GetExternalWallet();
    }

    /**
     * OnRecurringDonationUpdated is fired after a publisher was added or removed to/from
     * recurrent donation list
     */
    @Override
    public void OnRecurringDonationUpdated() {
        String pubId = mBraveRewardsNativeWorker.GetPublisherId(mCurrentTabId);
        // mPubInReccuredDonation = mBraveRewardsNativeWorker.IsCurrentPublisherInRecurrentDonations(pubId);

        //all (mPubInReccuredDonation, mAutoContributeEnabled) are false: exit
        //one is true: ac_enabled_controls on
        //mAutoContributeEnabled: attention_layout and include_in_ac_layout on
        //mPubInReccuredDonation: auto_tip_layout is on

        // if (mAutoContributeEnabled || mPubInReccuredDonation) {
        //     root.findViewById(R.id.ac_enabled_controls).setVisibility(View.VISIBLE);

        //     if (mAutoContributeEnabled) {
        //         root.findViewById(R.id.attention_layout).setVisibility(View.VISIBLE);
        //         root.findViewById(R.id.include_in_ac_layout).setVisibility(View.VISIBLE);
        //         root.findViewById(R.id.brave_ui_auto_contribute_separator_top).setVisibility(View.VISIBLE);
        //         root.findViewById(R.id.brave_ui_auto_contribute_separator_bottom).setVisibility(View.VISIBLE);
        //     }

            //Temporary commented out due to dropdown spinner inflating issue on PopupWindow (API 24)
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

    private boolean isVerifyWalletEnabled() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(PREF_VERIFY_WALLET_ENABLE, false);
    }

    private void showUpholdLoginPopupWindow(final View view) {
        PopupWindow loginPopupWindow = new PopupWindow(mActivity);

        LayoutInflater inflater =
                (LayoutInflater) mActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        ViewGroup rewardsMainLayout = mPopupView.findViewById(R.id.rewards_main_layout);
        View loginPopupView =
                inflater.inflate(R.layout.uphold_login_popup_window, mPopupView);

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
