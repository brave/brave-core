/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.ToggleButton;

import androidx.fragment.app.FragmentActivity;

import org.json.JSONException;

import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.rewards.BraveRewardsAmountChangeListener;
import org.chromium.chrome.browser.rewards.BraveRewardsBannerInfo;
import org.chromium.chrome.browser.rewards.BraveRewardsCreatorPanelFragment;
import org.chromium.chrome.browser.rewards.BraveRewardsTipConfirmationFragment;
import org.chromium.chrome.browser.rewards.BraveRewardsTipConfirmationListener;
import org.chromium.chrome.browser.rewards.BraveRewardsTippingPanelFragment;

public class BraveRewardsSiteBannerActivity
        extends FragmentActivity implements BraveRewardsTipConfirmationListener,
                                            BraveRewardsAmountChangeListener, BraveRewardsObserver {
    private ToggleButton radio_tip_amount[] = new ToggleButton[3];
    public static final String TAB_ID_EXTRA = "currentTabId";
    public static final String IS_MONTHLY_CONTRIBUTION = "is_monthly_contribution";
    public static final String TIP_AMOUNT_EXTRA="tipAmount";
    public static final String TIP_MONTHLY_EXTRA="tipMonthly";
    public static final String AMOUNTS_ARGS = "amounts_args";
    public static final String AMOUNT_EXTRA = "amount";
    public static final String BANNER_INFO_ARGS = "banner_info_args";
    public static final String STATUS_ARGS = "status_args";
    public static final String NAME_ARGS = "name_args";
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private int currentTabId_ = -1;
    private boolean mIsMonthlyContribution;
    private final int TIP_TIMEOUT = 3000; // 3 seconds
    private Handler tipTimerHandler = new Handler();
    private BraveRewardsBannerInfo bannerInfo;
    private double mAmount;
    private boolean mIsMonthly;
    public static final int TIP_ERROR = 1;
    public static final int TIP_SUCCESS = 2;
    public static final int TIP_PENDING = 3;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        //inflate
        super.onCreate(savedInstanceState);
        setContentView(R.layout.brave_rewards_site_banner);
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mBraveRewardsNativeWorker.AddObserver(this);

        currentTabId_ = IntentUtils.safeGetIntExtra(getIntent(), TAB_ID_EXTRA, -1);
        mIsMonthlyContribution =
                IntentUtils.safeGetBooleanExtra(getIntent(), IS_MONTHLY_CONTRIBUTION, false);
        bannerInfo = null;
        if (savedInstanceState == null) {
            mBraveRewardsNativeWorker.GetPublisherBanner(
                    mBraveRewardsNativeWorker.GetPublisherId(currentTabId_));
        }
    }

    @Override
    public void onPublisherBanner(String jsonBannerInfo) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                double[] amounts = null;

                try {
                    bannerInfo = new BraveRewardsBannerInfo(jsonBannerInfo);
                } catch (JSONException e) {
                }

                View progressBar = findViewById(R.id.progressBar);
                progressBar.setVisibility(View.GONE);

                BraveRewardsCreatorPanelFragment creatorPanelFragment =
                        BraveRewardsCreatorPanelFragment.newInstance(
                                currentTabId_, mIsMonthlyContribution, bannerInfo);
                getSupportFragmentManager()
                        .beginTransaction()
                        .replace(R.id.creatorPanelFragment, creatorPanelFragment)
                        .commit();

                BraveRewardsTippingPanelFragment tippingPanelFragment =
                        BraveRewardsTippingPanelFragment.newInstance(
                                currentTabId_, mIsMonthlyContribution, amounts);
                getSupportFragmentManager()
                        .beginTransaction()
                        .replace(R.id.tippingPanelFragment, tippingPanelFragment,
                                "tipping_panel_fragment")
                        .commit();
            }
        });
    }

    /*----------TIP CHECK start >> ---------------*/
    @Override
    public void onTipConfirmation(double amount, boolean isMonthly) {
        mAmount = amount;
        mIsMonthly = isMonthly;
        enableTimeout(); // wait for 3 seconds
    }

    private void tipConfirmation(int status, double amount, boolean isMonthly) {
        String publisherName = "";
        if (bannerInfo != null) publisherName = bannerInfo.getName();
        BraveRewardsTipConfirmationFragment tipConfirmationFragment =
                BraveRewardsTipConfirmationFragment.newInstance(
                        status, publisherName, amount, isMonthly);
        getSupportFragmentManager()
                .beginTransaction()
                .replace(R.id.tippingPanelFragment, tipConfirmationFragment)
                .commit();
    }

    @Override
    public void OnOneTimeTip(int resultCode) {
        if (resultCode == BraveRewardsNativeWorker.LEDGER_ERROR) {
            // tip error
            tipConfirmation(TIP_ERROR, mAmount, mIsMonthly);
        }
    }

    @Override
    public void onReconcileComplete(int resultCode, int rewardsType, double amount) {
        if (resultCode == BraveRewardsNativeWorker.LEDGER_OK) {
            // tip success
            tipConfirmation(TIP_SUCCESS, mAmount, mIsMonthly);
        }
    }

    @Override
    public void OnPendingContributionSaved(int resultCode) {
        if (resultCode == BraveRewardsNativeWorker.LEDGER_OK) {
            // tip pending
            tipConfirmation(TIP_PENDING, mAmount, mIsMonthly);
        }
    }

    private Runnable tipRunnable = new Runnable() {
        @Override
        public void run() {
            // if called means success
            tipConfirmation(TIP_SUCCESS, mAmount, mIsMonthly);
        }
    };

    public void removeTimeout() {
        tipTimerHandler.removeCallbacks(tipRunnable);
    }

    public void enableTimeout() {
        tipTimerHandler.postDelayed(tipRunnable, TIP_TIMEOUT);
    }

    /*----------TIP CHECK End >> ---------------*/

    @Override
    public void onAmountChange(double batValue, double usdValue) {
        BraveRewardsTippingPanelFragment tippingPanelFragment =
                (BraveRewardsTippingPanelFragment) getSupportFragmentManager().findFragmentById(
                        R.id.tippingPanelFragment);
        tippingPanelFragment.updateAmount(batValue, usdValue);
    }

    @Override
    public void onBackPressed() {
        int count = getSupportFragmentManager().getBackStackEntryCount();

        if (count == 0) {
            super.onBackPressed();
        } else {
            resetUpdateLayout();
            getSupportFragmentManager().popBackStack();
        }
    }

    public void resetUpdateLayout() {
        BraveRewardsTippingPanelFragment tippingPanelFragment =
                (BraveRewardsTippingPanelFragment) getSupportFragmentManager().findFragmentByTag(
                        "tipping_panel_fragment");

        if (tippingPanelFragment != null) {
            tippingPanelFragment.resetSendLayoutText();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (null != mBraveRewardsNativeWorker) {
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
    }
}
