/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.profiles.Profile;

public class BraveRewardsService extends Service implements BraveRewardsObserver {
    private static String TAG = "BraveRewardsService";

    private Context context;

    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        this.context = this;
    }

    @Override
    public void onDestroy() {
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (!LibraryLoader.getInstance().isInitialized()) {
            return START_STICKY;
        }
        try {
            mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
            if (mBraveRewardsNativeWorker != null) {
                mBraveRewardsNativeWorker.AddObserver(this);
                mBraveRewardsNativeWorker.CreateWallet();
            }
        } catch (UnsatisfiedLinkError error) {
            Log.e(TAG, error.getMessage());
            assert false;
        }

        return START_STICKY;
    }

    // interface BraveRewardsObserver
    @Override
    public void OnWalletInitialized(int error_code) {
        if (BraveRewardsNativeWorker.WALLET_CREATED == error_code
                && OnboardingPrefManager.getInstance().isAdsAvailable()) {
            // Enable ads
            BraveAdsNativeHelper.nativeSetAdsEnabled(Profile.getLastUsedRegularProfile());
        } else {
            // TODO: handle wallet creation problem
        }
        stopSelf();
    };

    @Override
    public void OnPublisherInfo(int tabId){};

    @Override
    public void OnGetCurrentBalanceReport(double[] report){};

    @Override
    public void OnNotificationAdded(String id, int type, long timestamp, String[] args){};

    @Override
    public void OnNotificationsCount(int count){};

    @Override
    public void OnGetLatestNotification(String id, int type, long timestamp, String[] args){};

    @Override
    public void OnNotificationDeleted(String id){};

    @Override
    public void OnIsWalletCreated(boolean created){};

    @Override
    public void OnGetPendingContributionsTotal(double amount){};

    @Override
    public void OnGetRewardsMainEnabled(boolean enabled){};

    @Override
    public void OnGetAutoContributeProperties(){};

    @Override
    public void OnGetReconcileStamp(long timestamp){};

    @Override
    public void OnRecurringDonationUpdated(){};

    @Override
    public void OnResetTheWholeState(boolean success){};

    @Override
    public void OnRewardsMainEnabled(boolean enabled){};

    @Override
    public void OnFetchPromotions() {}
}
