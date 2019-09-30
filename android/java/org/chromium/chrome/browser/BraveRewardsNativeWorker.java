/** Copyright (c) 2019 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */

package org.chromium.chrome.browser;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.BraveRewardsObserver;

import java.util.ArrayList;
import java.util.List;

@JNINamespace("chrome::android")
public class BraveRewardsNativeWorker {
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

    public static final int LEDGER_OK = 0;
    public static final int LEDGER_ERROR = 1;
    public static final int WALLET_CREATED = 12;
    public static final int SAFETYNET_ATTESTATION_FAILED = 27;
    
    private List<BraveRewardsObserver> observers_;
    private long mNativeBraveRewardsNativeWorker;

    private static BraveRewardsNativeWorker instance;
    private static final Object lock = new Object();
    private boolean createWalletInProcess;  // flag: wallet is being created
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
        observers_ = new ArrayList<BraveRewardsObserver>();
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
            observers_.add(observer);
        }
    }

    public void RemoveObserver(BraveRewardsObserver observer) {
        synchronized(lock) {
            observers_.remove(observer);
        }
    }

    public void CreateWallet() {
        synchronized(lock) {
            if (createWalletInProcess) {
                return;
            }
            createWalletInProcess = true;
            nativeCreateWallet(mNativeBraveRewardsNativeWorker);
        }
    }

    public boolean IsCreateWalletInProcess() {
        synchronized(lock) {
          return createWalletInProcess;
        }
    }

    public boolean IsGrantClaimInProcess() {
        synchronized(lock) {
          return grantClaimInProcess;
        }
    }

    public void WalletExist() {
        synchronized(lock) {
            nativeWalletExist(mNativeBraveRewardsNativeWorker);
        }
    }

    public void GetWalletProperties() {
        synchronized(lock) {
            nativeGetWalletProperties(mNativeBraveRewardsNativeWorker);
        }
    }

    public double GetWalletBalance() {
        synchronized(lock) {
            return nativeGetWalletBalance(mNativeBraveRewardsNativeWorker);
        }
    }

    public double GetWalletRate(String rate) {
        synchronized(lock) {
            return nativeGetWalletRate(mNativeBraveRewardsNativeWorker, rate);
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

    public boolean GetPublisherVerified(int tabId) {
        synchronized(lock) {
            return nativeGetPublisherVerified(mNativeBraveRewardsNativeWorker, tabId);
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

    public int GetCurrentGrantsCount() {
        synchronized(lock) {
            return nativeGetCurrentGrantsCount(mNativeBraveRewardsNativeWorker);
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

    public void SetRewardsMainEnabled(boolean enabled) {
        synchronized(lock) {
            nativeSetRewardsMainEnabled(mNativeBraveRewardsNativeWorker, enabled);
        }
    }

    public void GetRewardsMainEnabled() {
        synchronized(lock) {
            nativeGetRewardsMainEnabled(mNativeBraveRewardsNativeWorker);
        }
    }

    public void GetAutoContributeProps() {
        synchronized(lock) {
            nativeGetAutoContributeProps(mNativeBraveRewardsNativeWorker);
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

    public void GetAddresses() {
        synchronized(lock) {
            nativeGetAddresses(mNativeBraveRewardsNativeWorker);
        }
    }

    public String GetAddress(String addressName) {
        synchronized(lock) {
            return nativeGetAddress(mNativeBraveRewardsNativeWorker, addressName);
        }
    }

    @CalledByNative
    public void OnGetRewardsMainEnabled(boolean enabled) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnGetRewardsMainEnabled(enabled);
        }
    }

    @CalledByNative
    public void OnIsWalletCreated(boolean created) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnIsWalletCreated(created);
        }
    }

    @CalledByNative
    public void OnGetCurrentBalanceReport(String[] report) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnGetCurrentBalanceReport(report);
        }
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveRewardsNativeWorker == 0;
        mNativeBraveRewardsNativeWorker = nativePtr;
    }

    @CalledByNative
    public void OnWalletInitialized(int error_code) {
        createWalletInProcess = false;
        for(BraveRewardsObserver observer : observers_) {
            observer.OnWalletInitialized(error_code);
        }
    }

    @CalledByNative
    public void OnPublisherInfo(int tabId) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnPublisherInfo(tabId);
        }
    }

    @CalledByNative
    public void OnWalletProperties(int error_code) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnWalletProperties(error_code);
        }
    }

    @CalledByNative
    public void OnNotificationAdded(String id, int type, long timestamp,
            String[] args) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnNotificationAdded(id, type, timestamp, args);
        }
    }

    @CalledByNative
    public void OnNotificationsCount(int count) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnNotificationsCount(count);
        }
    }

    @CalledByNative
    public void OnGetLatestNotification(String id, int type, long timestamp,
            String[] args) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnGetLatestNotification(id, type, timestamp, args);
        }
    }

    @CalledByNative
    public void OnNotificationDeleted(String id) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnNotificationDeleted(id);
        }
    }

    @CalledByNative
    public void OnGetPendingContributionsTotal(double amount) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnGetPendingContributionsTotal(amount);
        }
    }

    @CalledByNative
    public void OnGetAutoContributeProps() {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnGetAutoContributeProps();
        }
    }

    @CalledByNative
    public void OnGetReconcileStamp(long timestamp) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnGetReconcileStamp(timestamp);
        }
    }

    @CalledByNative
    public void OnRecurringDonationUpdated() {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnRecurringDonationUpdated();
        }
    }

    @CalledByNative
    public void OnGrantFinish(int result) {
        grantClaimInProcess = false;
    }

    @CalledByNative
    public void OnResetTheWholeState(boolean success) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnResetTheWholeState(success);
        }
    }

    @CalledByNative
    public void OnRewardsMainEnabled(boolean enabled) {
        for(BraveRewardsObserver observer : observers_) {
            observer.OnRewardsMainEnabled(enabled);
        }
    }

    private native void nativeInit();
    private native void nativeDestroy(long nativeBraveRewardsNativeWorker);
    private native void nativeCreateWallet(long nativeBraveRewardsNativeWorker);
    private native void nativeWalletExist(long nativeBraveRewardsNativeWorker);
    private native void nativeGetWalletProperties(long nativeBraveRewardsNativeWorker);
    private native double nativeGetWalletBalance(long nativeBraveRewardsNativeWorker);
    private native double nativeGetWalletRate(long nativeBraveRewardsNativeWorker, String rate);
    private native void nativeGetPublisherInfo(long nativeBraveRewardsNativeWorker, int tabId, String host);
    private native String nativeGetPublisherURL(long nativeBraveRewardsNativeWorker, int tabId);
    private native String nativeGetPublisherFavIconURL(long nativeBraveRewardsNativeWorker, int tabId);
    private native String nativeGetPublisherName(long nativeBraveRewardsNativeWorker, int tabId);
    private native String nativeGetPublisherId(long nativeBraveRewardsNativeWorker, int tabId);
    private native int nativeGetPublisherPercent(long nativeBraveRewardsNativeWorker, int tabId);
    private native boolean nativeGetPublisherExcluded(long nativeBraveRewardsNativeWorker, int tabId);
    private native boolean nativeGetPublisherVerified(long nativeBraveRewardsNativeWorker, int tabId);
    private native void nativeIncludeInAutoContribution(long nativeBraveRewardsNativeWorker, int tabId,
      boolean exclude);
    private native void nativeRemovePublisherFromMap(long nativeBraveRewardsNativeWorker, int tabId);
    private native void nativeGetCurrentBalanceReport(long nativeBraveRewardsNativeWorker);
    private native void nativeDonate(long nativeBraveRewardsNativeWorker, String publisher_key, 
        int amount, boolean recurring);
    private native void nativeGetAllNotifications(long nativeBraveRewardsNativeWorker);
    private native void nativeDeleteNotification(long nativeBraveRewardsNativeWorker, 
        String notification_id);
    private native void nativeGetGrant(long nativeBraveRewardsNativeWorker, String promotionId);
    private native int nativeGetCurrentGrantsCount(long nativeBraveRewardsNativeWorker);
    private native String[] nativeGetCurrentGrant(long nativeBraveRewardsNativeWorker, int position);
    private native void nativeGetPendingContributionsTotal(long nativeBraveRewardsNativeWorker);
    private native void nativeGetRecurringDonations(long nativeBraveRewardsNativeWorker);
    private native boolean nativeIsCurrentPublisherInRecurrentDonations(long nativeBraveRewardsNativeWorker,
        String publisher);
    private native void nativeGetRewardsMainEnabled(long nativeBraveRewardsNativeWorker);
    private native void nativeSetRewardsMainEnabled(long nativeBraveRewardsNativeWorker, boolean enabled);
    private native void nativeGetAutoContributeProps(long nativeBraveRewardsNativeWorker);
    private native boolean nativeIsAutoContributeEnabled(long nativeBraveRewardsNativeWorker);
    private native void nativeGetReconcileStamp(long nativeBraveRewardsNativeWorker);
    private native double nativeGetPublisherRecurrentDonationAmount(long nativeBraveRewardsNativeWorker, String publisher);
    private native void nativeRemoveRecurring(long nativeBraveRewardsNativeWorker, String publisher);
    private native void nativeResetTheWholeState(long nativeBraveRewardsNativeWorker);
    private native void nativeFetchGrants(long nativeBraveRewardsNativeWorker);
    private native void nativeGetAddresses(long nativeBraveRewardsNativeWorker);
    private native String nativeGetAddress(long nativeBraveRewardsNativeWorker, String addressName);
}
