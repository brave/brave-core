/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.tipping;

import android.view.View;

import org.chromium.base.IntentUtils;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.init.ActivityProfileProvider;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.profiles.ProfileProvider;

public class RewardsTippingBannerActivity extends AsyncInitializationActivity
        implements BraveRewardsObserver {
    public static final String TAB_ID_EXTRA = "currentTabId";
    public static final String TIP_MONTHLY_EXTRA = "tipMonthly";
    public static final String TIP_AMOUNT_EXTRA = "tipAmount";
    private int mCurrentTabId = -1;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_tipping_banner_mobile);

        mCurrentTabId = IntentUtils.safeGetIntExtra(getIntent(), TAB_ID_EXTRA, -1);
        showCustomUI();
        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        RewardsTippingBannerFragment tippingBannerFragment =
                RewardsTippingBannerFragment.newInstance(mCurrentTabId);
        getSupportFragmentManager()
                .beginTransaction()
                .replace(R.id.tippingBannerFragment, tippingBannerFragment)
                .commit();
    }

    private void showCustomUI() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    protected OneshotSupplier<ProfileProvider> createProfileProvider() {
        return new ActivityProfileProvider(getLifecycleDispatcher());
    }
}
