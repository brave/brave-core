/** Copyright (c) 2019 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */

package org.chromium.chrome.browser;

import android.os.Handler;

import androidx.annotation.Nullable;

import org.json.JSONException;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsPublisher.PublisherStatus;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.embedder_support.util.UrlConstants;

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
          BraveRewardsNativeWorkerJni.get().init(BraveRewardsNativeWorker.this);
      }
    }

    @Override
    protected void finalize() {
        Destroy();
    }

    private void Destroy() {
        if (mNativeBraveRewardsNativeWorker != 0) {
            BraveRewardsNativeWorkerJni.get().destroy(mNativeBraveRewardsNativeWorker);
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
                    OnNotifyFrontTabUrlChanged(tab.getId(), tab.getUrl().getSpec());
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
            BraveRewardsNativeWorkerJni.get().getRewardsParameters(mNativeBraveRewardsNativeWorker);
        }
    }

    @Nullable
    public BraveRewardsBalance GetWalletBalance() {
        synchronized(lock) {
            String json = BraveRewardsNativeWorkerJni.get().getWalletBalance(
                    mNativeBraveRewardsNativeWorker);
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
            return BraveRewardsNativeWorkerJni.get().getWalletRate(mNativeBraveRewardsNativeWorker);
        }
    }

    public void GetPublisherInfo(int tabId, String host) {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().getPublisherInfo(
                    mNativeBraveRewardsNativeWorker, tabId, host);
        }
    }

    public String GetPublisherURL(int tabId) {
        synchronized(lock) {
            return BraveRewardsNativeWorkerJni.get().getPublisherURL(
                    mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public String GetPublisherFavIconURL(int tabId) {
        synchronized(lock) {
            return BraveRewardsNativeWorkerJni.get().getPublisherFavIconURL(
                    mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public String GetPublisherName(int tabId) {
        synchronized(lock) {
            return BraveRewardsNativeWorkerJni.get().getPublisherName(
                    mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public String GetPublisherId(int tabId) {
        synchronized(lock) {
            return BraveRewardsNativeWorkerJni.get().getPublisherId(
                    mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public int GetPublisherPercent(int tabId) {
        synchronized(lock) {
            return BraveRewardsNativeWorkerJni.get().getPublisherPercent(
                    mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public boolean GetPublisherExcluded(int tabId) {
        synchronized(lock) {
            return BraveRewardsNativeWorkerJni.get().getPublisherExcluded(
                    mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public @PublisherStatus int GetPublisherStatus(int tabId) {
        synchronized(lock) {
            return BraveRewardsNativeWorkerJni.get().getPublisherStatus(
                    mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public void IncludeInAutoContribution(int tabId, boolean exclude) {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().includeInAutoContribution(
                    mNativeBraveRewardsNativeWorker, tabId, exclude);
        }
    }

    public void RemovePublisherFromMap(int tabId) {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().removePublisherFromMap(
                    mNativeBraveRewardsNativeWorker, tabId);
        }
    }

    public void GetCurrentBalanceReport() {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().getCurrentBalanceReport(
                    mNativeBraveRewardsNativeWorker);
        }
    }

    public void Donate(String publisher_key, int amount, boolean recurring) {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().donate(
                    mNativeBraveRewardsNativeWorker, publisher_key, amount, recurring);
        }
    }

    public void GetAllNotifications() {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().getAllNotifications(mNativeBraveRewardsNativeWorker);
        }
    }

    public void DeleteNotification(String notification_id) {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().deleteNotification(
                    mNativeBraveRewardsNativeWorker, notification_id);
        }
    }

    public void GetGrant(String promotionId) {
        synchronized(lock) {
            if (grantClaimInProcess) {
                return;
            }
            grantClaimInProcess = true;
            BraveRewardsNativeWorkerJni.get().getGrant(
                    mNativeBraveRewardsNativeWorker, promotionId);
        }
    }

    public String[] GetCurrentGrant(int position) {
        synchronized(lock) {
            return BraveRewardsNativeWorkerJni.get().getCurrentGrant(
                    mNativeBraveRewardsNativeWorker, position);
        }
    }

    public void GetPendingContributionsTotal() {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().getPendingContributionsTotal(
                    mNativeBraveRewardsNativeWorker);
        }
    }

    public void GetRecurringDonations() {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().getRecurringDonations(
                    mNativeBraveRewardsNativeWorker);
        }
    }

    public boolean IsCurrentPublisherInRecurrentDonations(String publisher) {
        synchronized(lock) {
            return BraveRewardsNativeWorkerJni.get().isCurrentPublisherInRecurrentDonations(
                    mNativeBraveRewardsNativeWorker, publisher);
        }
    }

    public double GetPublisherRecurrentDonationAmount(String publisher) {
        synchronized(lock) {
            return BraveRewardsNativeWorkerJni.get().getPublisherRecurrentDonationAmount(
                    mNativeBraveRewardsNativeWorker, publisher);
        }
    }

    public void GetAutoContributeProperties() {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().getAutoContributeProperties(
                    mNativeBraveRewardsNativeWorker);
        }
    }

    public boolean IsAutoContributeEnabled() {
        synchronized(lock) {
            return BraveRewardsNativeWorkerJni.get().isAutoContributeEnabled(
                    mNativeBraveRewardsNativeWorker);
        }
    }

    public void GetReconcileStamp() {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().getReconcileStamp(mNativeBraveRewardsNativeWorker);
        }
    }

    public void RemoveRecurring(String publisher) {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().removeRecurring(
                    mNativeBraveRewardsNativeWorker, publisher);
        }
    }

    public void ResetTheWholeState() {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().resetTheWholeState(mNativeBraveRewardsNativeWorker);
        }
    }

    public void FetchGrants() {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().fetchGrants(mNativeBraveRewardsNativeWorker);
        }
    }

    public int GetAdsPerHour() {
        synchronized (lock) {
            return BraveRewardsNativeWorkerJni.get().getAdsPerHour(mNativeBraveRewardsNativeWorker);
        }
    }

    public void SetAdsPerHour(int value) {
        synchronized (lock) {
            BraveRewardsNativeWorkerJni.get().setAdsPerHour(mNativeBraveRewardsNativeWorker, value);
        }
    }

    public boolean isRewardsEnabled() {
        synchronized (lock) {
            return BraveRewardsNativeWorkerJni.get().isRewardsEnabled(
                    mNativeBraveRewardsNativeWorker);
        }
    }

    public void GetExternalWallet() {
        synchronized (lock) {
            BraveRewardsNativeWorkerJni.get().getExternalWallet(mNativeBraveRewardsNativeWorker);
        }
    }

    public void DisconnectWallet() {
        synchronized (lock) {
            BraveRewardsNativeWorkerJni.get().disconnectWallet(mNativeBraveRewardsNativeWorker);
        }
    }

    public void ProcessRewardsPageUrl(String path, String query) {
        synchronized (lock) {
            BraveRewardsNativeWorkerJni.get().processRewardsPageUrl(
                    mNativeBraveRewardsNativeWorker, path, query);
        }
    }

    public void RecoverWallet(String passPhrase) {
        synchronized (lock) {
            BraveRewardsNativeWorkerJni.get().recoverWallet(
                    mNativeBraveRewardsNativeWorker, passPhrase);
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
            BraveRewardsNativeWorkerJni.get().refreshPublisher(
                    mNativeBraveRewardsNativeWorker, publisherKey);
        }
    }

    public void SetAutoContributeEnabled(boolean isSetAutoContributeEnabled) {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().setAutoContributeEnabled(
                    mNativeBraveRewardsNativeWorker, isSetAutoContributeEnabled);
        }
    }

    public void SetAutoContributionAmount(double amount) {
        synchronized(lock) {
            BraveRewardsNativeWorkerJni.get().setAutoContributionAmount(
                    mNativeBraveRewardsNativeWorker, amount);
        }
    }

    public void StartProcess() {
        synchronized (lock) {
            BraveRewardsNativeWorkerJni.get().startProcess(mNativeBraveRewardsNativeWorker);
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
        boolean verified = (pubStatus == BraveRewardsPublisher.CONNECTED
                                   || pubStatus == BraveRewardsPublisher.UPHOLD_VERIFIED)
                ? true
                : false;
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

    @NativeMethods
    interface Natives {
        void init(BraveRewardsNativeWorker caller);
        void destroy(long nativeBraveRewardsNativeWorker);
        String getWalletBalance(long nativeBraveRewardsNativeWorker);
        double getWalletRate(long nativeBraveRewardsNativeWorker);
        void getPublisherInfo(long nativeBraveRewardsNativeWorker, int tabId, String host);
        String getPublisherURL(long nativeBraveRewardsNativeWorker, int tabId);
        String getPublisherFavIconURL(long nativeBraveRewardsNativeWorker, int tabId);
        String getPublisherName(long nativeBraveRewardsNativeWorker, int tabId);
        String getPublisherId(long nativeBraveRewardsNativeWorker, int tabId);
        int getPublisherPercent(long nativeBraveRewardsNativeWorker, int tabId);
        boolean getPublisherExcluded(long nativeBraveRewardsNativeWorker, int tabId);
        int getPublisherStatus(long nativeBraveRewardsNativeWorker, int tabId);
        void includeInAutoContribution(
                long nativeBraveRewardsNativeWorker, int tabId, boolean exclude);
        void removePublisherFromMap(long nativeBraveRewardsNativeWorker, int tabId);
        void getCurrentBalanceReport(long nativeBraveRewardsNativeWorker);
        void donate(long nativeBraveRewardsNativeWorker, String publisher_key, int amount,
                boolean recurring);
        void getAllNotifications(long nativeBraveRewardsNativeWorker);
        void deleteNotification(long nativeBraveRewardsNativeWorker, String notification_id);
        void getGrant(long nativeBraveRewardsNativeWorker, String promotionId);
        String[] getCurrentGrant(long nativeBraveRewardsNativeWorker, int position);
        void getPendingContributionsTotal(long nativeBraveRewardsNativeWorker);
        void getRecurringDonations(long nativeBraveRewardsNativeWorker);
        boolean isCurrentPublisherInRecurrentDonations(
                long nativeBraveRewardsNativeWorker, String publisher);
        void getAutoContributeProperties(long nativeBraveRewardsNativeWorker);
        boolean isAutoContributeEnabled(long nativeBraveRewardsNativeWorker);
        void getReconcileStamp(long nativeBraveRewardsNativeWorker);
        double getPublisherRecurrentDonationAmount(
                long nativeBraveRewardsNativeWorker, String publisher);
        void removeRecurring(long nativeBraveRewardsNativeWorker, String publisher);
        void resetTheWholeState(long nativeBraveRewardsNativeWorker);
        void fetchGrants(long nativeBraveRewardsNativeWorker);
        int getAdsPerHour(long nativeBraveRewardsNativeWorker);
        void setAdsPerHour(long nativeBraveRewardsNativeWorker, int value);
        boolean isRewardsEnabled(long nativeBraveRewardsNativeWorker);
        void getExternalWallet(long nativeBraveRewardsNativeWorker);
        void disconnectWallet(long nativeBraveRewardsNativeWorker);
        void processRewardsPageUrl(long nativeBraveRewardsNativeWorker, String path, String query);
        void recoverWallet(long nativeBraveRewardsNativeWorker, String passPhrase);
        void refreshPublisher(long nativeBraveRewardsNativeWorker, String publisherKey);
        void getRewardsParameters(long nativeBraveRewardsNativeWorker);
        void setAutoContributeEnabled(
                long nativeBraveRewardsNativeWorker, boolean isSetAutoContributeEnabled);
        void setAutoContributionAmount(long nativeBraveRewardsNativeWorker, double amount);
        void startProcess(long nativeBraveRewardsNativeWorker);
    }
}
