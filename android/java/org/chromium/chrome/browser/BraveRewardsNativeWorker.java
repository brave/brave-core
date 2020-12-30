/** Copyright (c) 2019 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */

package org.chromium.chrome.browser;

import android.os.Handler;
import androidx.annotation.Nullable;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsPublisher.PublisherStatus;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.json.JSONException;

import java.util.ArrayList;
import java.util.List;

@JNINamespace("chrome::android")
public class BraveRewardsNativeWorker {
    /**
     * Allows to monitor a front tab publisher changes.
     */
    public interface PublisherObserver { void onFrontTabPublisherChanged(boolean verified); }

    // Rewards notifications
    // Taken from components/brave_rewards/browser/rewards_notification_service.h
    public static final int REWARDS_NOTIFICATION_INVALID = 0;
    public static final int REWARDS_NOTIFICATION_AUTO_CONTRIBUTE = 1;
    public static final int REWARDS_NOTIFICATION_GRANT = 2;
    public static final int REWARDS_NOTIFICATION_GRANT_ADS = 3;
    public static final int REWARDS_NOTIFICATION_FAILED_CONTRIBUTION = 4;
    public static final int REWARDS_NOTIFICATION_IMPENDING_CONTRIBUTION = 5;
    public static final int REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS = 6;
    public static final int REWARDS_NOTIFICATION_BACKUP_WALLET = 7;
    public static final int REWARDS_NOTIFICATION_TIPS_PROCESSED = 8;
    public static final int REWARDS_NOTIFICATION_ADS_ONBOARDING = 9;
    public static final int REWARDS_NOTIFICATION_VERIFIED_PUBLISHER = 10;

    public static final int LEDGER_OK = 0;
    public static final int LEDGER_ERROR = 1;
    public static final int WALLET_CREATED = 12;
    public static final int BAT_NOT_ALLOWED = 25;
    public static final int SAFETYNET_ATTESTATION_FAILED = 27;

    private static final int REWARDS_UNKNOWN = 0;
    private static final int REWARDS_DISABLED = 1;
    private static final int REWARDS_ENABLED = 2;
    private static int rewardsStatus = REWARDS_UNKNOWN;
    private String frontTabUrl;
    private static final Handler mHandler = new Handler();

    private List<BraveRewardsObserver> mObservers;
    private List<PublisherObserver> mFrontTabPublisherObservers;
    private long mNativeBraveRewardsNativeWorker;

    private static BraveRewardsNativeWorker instance;
    private static final Object lock = new Object();
    private boolean grantClaimInProcess;  // flag: wallet is being created

    public static  BraveRewardsNativeWorker getInstance() {
        synchronized(lock) {
          if(instance == null) {
              instance = new BraveRewardsNativeWorker();
              instance.Init();
          }
        }
        return instance;
    }

    private BraveRewardsNativeWorker() {
        mObservers = new ArrayList<BraveRewardsObserver>();
        mFrontTabPublisherObservers = new ArrayList<PublisherObserver>();
    }

    private void Init() {
      if (mNativeBraveRewardsNativeWorker == 0) {
          nativeInit();
      }
    }

    @Override
    protected void finalize() {
        Destroy();
    }

    private void Destroy() {
        if (mNativeBraveRewardsNativeWorker != 0) {
            nativeDestroy(mNativeBraveRewardsNativeWorker);
            mNativeBraveRewardsNativeWorker = 0;
        }
    }

    public void AddObserver(BraveRewardsObserver observer) {
        synchronized(lock) {
            mObservers.add(observer);
        }
    }

    public void RemoveObserver(BraveRewardsObserver observer) {
        synchronized(lock) {
            mObservers.remove(observer);
        }
    }

    public void AddPublisherObserver(PublisherObserver observer) {
        synchronized (lock) {
            mFrontTabPublisherObservers.add(observer);
        }
    }

    public void RemovePublisherObserver(PublisherObserver observer) {
        synchronized (lock) {
            mFrontTabPublisherObservers.remove(observer);
        }
    }

    public void OnNotifyFrontTabUrlChanged(int tabId, String url) {
        boolean chromeUrl = url.startsWith(UrlConstants.CHROME_SCHEME);
        boolean newUrl = (frontTabUrl == null || !frontTabUrl.equals(url));
        if (chromeUrl) {
            // Don't query 'GetPublisherInfo' and post response now.
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    NotifyPublisherObservers(false);
                }
            });
        } else if (newUrl) {
            GetPublisherInfo(tabId, url);
        }

        frontTabUrl = url;
    }

    private void NotifyPublisherObservers(boolean verified) {
        for (PublisherObserver observer : mFrontTabPublisherObservers) {
            observer.onFrontTabPublisherChanged(verified);
        }
    }

    public void TriggerOnNotifyFrontTabUrlChanged() {
        // Clear frontTabUrl so that all observers are updated.
        frontTabUrl = "";
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                Tab tab = BraveRewardsHelper.currentActiveChromeTabbedActivityTab();
                if (tab != null && !tab.isIncognito()) {
                    OnNotifyFrontTabUrlChanged(tab.getId(), tab.getUrlString());
                }
            }
        });
    }

    public boolean IsGrantClaimInProcess() {
        synchronized(lock) {
          return grantClaimInProcess;
        }
    }

    public void GetRewardsParameters() {
        synchronized(lock) {
            nativeGetRewardsParameters(mNativeBraveRewardsNativeWorker);
        }
    }

    @Nullable
    public BraveRewardsBalance GetWalletBalance() {
        synchronized(lock) {
            String  json = nativeGetWalletBalance(mNativeBraveRewardsNativeWorker);
            BraveRewardsBalance balance = null;
            try{
                balance = new BraveRewardsBalance (json);
            }
            catch (JSONException e) {
                balance = null;
            }
            return balance;
        }
    }

    public double GetWalletRate() {
        synchronized(lock) {
            return nativeGetWalletRate(mNativeBraveRewardsNativeWorker);
        }
    }

    public void GetPublisherInfo(int tabId, String host) {
        synchronized(lock) {
            nativeGetPublisherInfo(mNativeBraveRewardsNativeWorker, tabId, host);
        }
    }

    public String GetPublisherURL(int tabId) {
        synchronized(lock) {
            return nativeGetPublisherURL(mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public String GetPublisherFavIconURL(int tabId) {
        synchronized(lock) {
            return nativeGetPublisherFavIconURL(mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public String GetPublisherName(int tabId) {
        synchronized(lock) {
            return nativeGetPublisherName(mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public String GetPublisherId(int tabId) {
        synchronized(lock) {
            return nativeGetPublisherId(mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public int GetPublisherPercent(int tabId) {
        synchronized(lock) {
            return nativeGetPublisherPercent(mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public boolean GetPublisherExcluded(int tabId) {
        synchronized(lock) {
            return nativeGetPublisherExcluded(mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public @PublisherStatus int GetPublisherStatus(int tabId) {
        synchronized(lock) {
            return nativeGetPublisherStatus(mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public void IncludeInAutoContribution(int tabId, boolean exclude) {
        synchronized(lock) {
            nativeIncludeInAutoContribution(mNativeBraveRewardsNativeWorker, tabId, exclude);
        }
    }

    public void RemovePublisherFromMap(int tabId) {
        synchronized(lock) {
            nativeRemovePublisherFromMap(mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public void GetCurrentBalanceReport() {
        synchronized(lock) {
            nativeGetCurrentBalanceReport(mNativeBraveRewardsNativeWorker);
        }
    }

    public void Donate(String publisher_key, int amount, boolean recurring) {
        synchronized(lock) {
            nativeDonate(mNativeBraveRewardsNativeWorker, publisher_key, amount, recurring);
        }
    }

    public void GetAllNotifications() {
        synchronized(lock) {
            nativeGetAllNotifications(mNativeBraveRewardsNativeWorker);
        }
    }

    public void DeleteNotification(String notification_id) {
        synchronized(lock) {
            nativeDeleteNotification(mNativeBraveRewardsNativeWorker, notification_id);
        }
    }

    public void GetGrant(String promotionId) {
        synchronized(lock) {
            if (grantClaimInProcess) {
                return;
            }
            grantClaimInProcess = true;
            nativeGetGrant(mNativeBraveRewardsNativeWorker, promotionId);
        }
    }

    public String[] GetCurrentGrant(int position) {
        synchronized(lock) {
            return nativeGetCurrentGrant(mNativeBraveRewardsNativeWorker, position);
        }
    }

    public void GetPendingContributionsTotal() {
        synchronized(lock) {
            nativeGetPendingContributionsTotal(mNativeBraveRewardsNativeWorker);
        }
    }

    public void GetRecurringDonations() {
        synchronized(lock) {
            nativeGetRecurringDonations(mNativeBraveRewardsNativeWorker);
        }
    }

    public boolean IsCurrentPublisherInRecurrentDonations(String publisher) {
        synchronized(lock) {
            return nativeIsCurrentPublisherInRecurrentDonations(mNativeBraveRewardsNativeWorker, publisher);
        }
    }

    public double GetPublisherRecurrentDonationAmount(String publisher) {
        synchronized(lock) {
            return nativeGetPublisherRecurrentDonationAmount(mNativeBraveRewardsNativeWorker, publisher);
        }
    }

    public void GetAutoContributeProperties() {
        synchronized(lock) {
            nativeGetAutoContributeProperties(mNativeBraveRewardsNativeWorker);
        }
    }

    public boolean IsAutoContributeEnabled() {
        synchronized(lock) {
            return nativeIsAutoContributeEnabled(mNativeBraveRewardsNativeWorker);
        }
    }

    public void GetReconcileStamp() {
        synchronized(lock) {
            nativeGetReconcileStamp(mNativeBraveRewardsNativeWorker);
        }
    }

    public void RemoveRecurring(String publisher) {
        synchronized(lock) {
            nativeRemoveRecurring(mNativeBraveRewardsNativeWorker,publisher);
        }
    }

    public void ResetTheWholeState() {
        synchronized(lock) {
            nativeResetTheWholeState(mNativeBraveRewardsNativeWorker);
        }
    }

    public void FetchGrants() {
        synchronized(lock) {
            nativeFetchGrants(mNativeBraveRewardsNativeWorker);
        }
    }

    public int GetAdsPerHour() {
        synchronized (lock) {
            return nativeGetAdsPerHour(mNativeBraveRewardsNativeWorker);
        }
    }

    public void SetAdsPerHour(int value) {
        synchronized (lock) {
            nativeSetAdsPerHour(mNativeBraveRewardsNativeWorker, value);
        }
    }

    public boolean IsAnonWallet() {
        synchronized(lock) {
            return nativeIsAnonWallet(mNativeBraveRewardsNativeWorker);
        }
    }

    public void GetExternalWallet() {
        synchronized (lock) {
            nativeGetExternalWallet(mNativeBraveRewardsNativeWorker);
        }
    }

    public void DisconnectWallet(String wallet_type) {
        synchronized (lock) {
            nativeDisconnectWallet(mNativeBraveRewardsNativeWorker, wallet_type);
        }
    }

    public void ProcessRewardsPageUrl(String path, String query) {
        synchronized (lock) {
            nativeProcessRewardsPageUrl(mNativeBraveRewardsNativeWorker,
                    path, query);
        }
    }

    public void RecoverWallet(String passPhrase) {
        synchronized (lock) {
            nativeRecoverWallet(mNativeBraveRewardsNativeWorker, passPhrase);
        }
    }

    @CalledByNative
    public void OnRecoverWallet(int errorCode) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnRecoverWallet(errorCode);
        }
    }

    public void RefreshPublisher(String publisherKey) {
        synchronized (lock) {
            nativeRefreshPublisher(mNativeBraveRewardsNativeWorker, publisherKey);
        }
    }

    public void SetAutoContributeEnabled(boolean isSetAutoContributeEnabled) {
        synchronized(lock) {
            nativeSetAutoContributeEnabled(mNativeBraveRewardsNativeWorker, isSetAutoContributeEnabled);
        }
    }

    public void SetAutoContributionAmount(double amount) {
        synchronized(lock) {
            nativeSetAutoContributionAmount(mNativeBraveRewardsNativeWorker, amount);
        }
    }

    public void StartProcess() {
        synchronized (lock) {
            nativeStartProcess(mNativeBraveRewardsNativeWorker);
        }
    }

    @CalledByNative
    public void OnStartProcess() {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnStartProcess();
        }
    }

    @CalledByNative
    public void OnRefreshPublisher(int status, String publisherKey) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnRefreshPublisher(status, publisherKey);
        }
    }

    @CalledByNative
    public void OnRewardsParameters(int errorCode) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnRewardsParameters(errorCode);
        }
    }

    @CalledByNative
    public void OnGetCurrentBalanceReport(double[] report) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnGetCurrentBalanceReport(report);
        }
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveRewardsNativeWorker == 0;
        mNativeBraveRewardsNativeWorker = nativePtr;
    }

    @CalledByNative
    public void OnPublisherInfo(int tabId) {
        @PublisherStatus int pubStatus = GetPublisherStatus(tabId);
        boolean verified = (pubStatus == BraveRewardsPublisher.CONNECTED ||
                pubStatus == BraveRewardsPublisher.VERIFIED) ? true : false;
        NotifyPublisherObservers(verified);

        // Notify BraveRewardsObserver (panel).
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnPublisherInfo(tabId);
        }
    }

    @CalledByNative
    public void OnNotificationAdded(String id, int type, long timestamp,
            String[] args) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnNotificationAdded(id, type, timestamp, args);
        }
    }

    @CalledByNative
    public void OnNotificationsCount(int count) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnNotificationsCount(count);
        }
    }

    @CalledByNative
    public void OnGetLatestNotification(String id, int type, long timestamp,
            String[] args) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnGetLatestNotification(id, type, timestamp, args);
        }
    }

    @CalledByNative
    public void OnNotificationDeleted(String id) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnNotificationDeleted(id);
        }
    }

    @CalledByNative
    public void OnGetPendingContributionsTotal(double amount) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnGetPendingContributionsTotal(amount);
        }
    }

    @CalledByNative
    public void OnGetAutoContributeProperties() {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnGetAutoContributeProperties();
        }
    }

    @CalledByNative
    public void OnGetReconcileStamp(long timestamp) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnGetReconcileStamp(timestamp);
        }
    }

    @CalledByNative
    public void OnRecurringDonationUpdated() {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnRecurringDonationUpdated();
        }
    }

    @CalledByNative
    public void OnGrantFinish(int result) {
        grantClaimInProcess = false;
        for(BraveRewardsObserver observer : mObservers) {
            observer.OnGrantFinish(result);
        }
    }

    @CalledByNative
    public void OnResetTheWholeState(boolean success) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnResetTheWholeState(success);
        }
    }

    @CalledByNative
    public void OnGetExternalWallet(int error_code, String external_wallet) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnGetExternalWallet(error_code, external_wallet);
        }
    }

    @CalledByNative
    public void OnDisconnectWallet(int error_code, String external_wallet) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnDisconnectWallet(error_code, external_wallet);
        }
    }

    @CalledByNative
    public void OnProcessRewardsPageUrl(int error_code, String wallet_type,
            String action, String json_args) {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnProcessRewardsPageUrl(error_code, wallet_type,
                    action, json_args);
        }
    }

    @CalledByNative
    public void OnClaimPromotion(int error_code) {
        grantClaimInProcess = false;
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnClaimPromotion(error_code);
        }
    }

    @CalledByNative
    public void OnOneTimeTip() {
        for (BraveRewardsObserver observer : mObservers) {
            observer.OnOneTimeTip();
        }
    }

    private native void nativeInit();
    private native void nativeDestroy(long nativeBraveRewardsNativeWorker);
    private native String nativeGetWalletBalance(long nativeBraveRewardsNativeWorker);
    private native double nativeGetWalletRate(long nativeBraveRewardsNativeWorker);
    private native void nativeGetPublisherInfo(long nativeBraveRewardsNativeWorker, int tabId, String host);
    private native String nativeGetPublisherURL(long nativeBraveRewardsNativeWorker, int tabId);
    private native String nativeGetPublisherFavIconURL(long nativeBraveRewardsNativeWorker, int tabId);
    private native String nativeGetPublisherName(long nativeBraveRewardsNativeWorker, int tabId);
    private native String nativeGetPublisherId(long nativeBraveRewardsNativeWorker, int tabId);
    private native int nativeGetPublisherPercent(long nativeBraveRewardsNativeWorker, int tabId);
    private native boolean nativeGetPublisherExcluded(long nativeBraveRewardsNativeWorker, int tabId);
    private native int nativeGetPublisherStatus(long nativeBraveRewardsNativeWorker, int tabId);
    private native void nativeIncludeInAutoContribution(long nativeBraveRewardsNativeWorker, int tabId,
      boolean exclude);
    private native void nativeRemovePublisherFromMap(long nativeBraveRewardsNativeWorker, int tabId);
    private native void nativeGetCurrentBalanceReport(long nativeBraveRewardsNativeWorker);
    private native void nativeDonate(long nativeBraveRewardsNativeWorker, String publisher_key,
            int amount, boolean recurring);
    private native void nativeGetAllNotifications(long nativeBraveRewardsNativeWorker);
    private native void nativeDeleteNotification(
            long nativeBraveRewardsNativeWorker, String notification_id);
    private native void nativeGetGrant(long nativeBraveRewardsNativeWorker, String promotionId);
    private native String[] nativeGetCurrentGrant(long nativeBraveRewardsNativeWorker, int position);
    private native void nativeGetPendingContributionsTotal(long nativeBraveRewardsNativeWorker);
    private native void nativeGetRecurringDonations(long nativeBraveRewardsNativeWorker);
    private native boolean nativeIsCurrentPublisherInRecurrentDonations(long nativeBraveRewardsNativeWorker,
        String publisher);
    private native void nativeGetAutoContributeProperties(long nativeBraveRewardsNativeWorker);
    private native boolean nativeIsAutoContributeEnabled(long nativeBraveRewardsNativeWorker);
    private native void nativeGetReconcileStamp(long nativeBraveRewardsNativeWorker);
    private native double nativeGetPublisherRecurrentDonationAmount(long nativeBraveRewardsNativeWorker, String publisher);
    private native void nativeRemoveRecurring(long nativeBraveRewardsNativeWorker, String publisher);
    private native void nativeResetTheWholeState(long nativeBraveRewardsNativeWorker);
    private native void nativeFetchGrants(long nativeBraveRewardsNativeWorker);
    private native int nativeGetAdsPerHour(long nativeBraveRewardsNativeWorker);
    private native void nativeSetAdsPerHour(long nativeBraveRewardsNativeWorker, int value);
    private native boolean nativeIsAnonWallet(long nativeBraveRewardsNativeWorker);
    private native void nativeGetExternalWallet(long nativeBraveRewardsNativeWorker);
    private native void nativeDisconnectWallet(long nativeBraveRewardsNativeWorker, String wallet_type);
    private native void nativeProcessRewardsPageUrl(long nativeBraveRewardsNativeWorker, String path, String query);
    private native void nativeRecoverWallet(long nativeBraveRewardsNativeWorker, String passPhrase);
    private native void nativeRefreshPublisher(long nativeBraveRewardsNativeWorker, String publisherKey);
    private native void nativeGetRewardsParameters(long nativeBraveRewardsNativeWorker);
    private native void nativeSetAutoContributeEnabled(long nativeBraveRewardsNativeWorker, boolean isSetAutoContributeEnabled);
    private native void nativeSetAutoContributionAmount(long nativeBraveRewardsNativeWorker, double amount);
    private native void nativeStartProcess(long nativeBraveRewardsNativeWorker);
}
