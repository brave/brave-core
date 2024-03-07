/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

public interface BraveRewardsObserver {
    default public void onCreateRewardsWallet(String result){};
    default public void OnRewardsParameters(){};
    default public void onBalance(boolean success){};
    default public void OnPublisherInfo(int tabId){};
    default public void OnGetCurrentBalanceReport(double[] report){};
    default public void OnNotificationAdded(String id, int type, long timestamp, String[] args){};
    default public void OnNotificationsCount(int count){};
    default public void OnGetLatestNotification(
            String id, int type, long timestamp, String[] args){};
    default public void OnNotificationDeleted(String id){};
    default public void OnGetAutoContributeProperties(){};
    default public void onGetAutoContributionAmount(double amount){};
    default public void OnGetReconcileStamp(long timestamp){};
    default public void OnRecurringDonationUpdated(){};
    default public void OnResetTheWholeState(boolean success){};
    default public void OnGrantFinish(int result){};
    default public void OnGetExternalWallet(String external_wallet){};
    default public void onGetAvailableCountries(String[] countries){};
    default public void OnExternalWalletConnected(){};
    default public void OnExternalWalletLoggedOut(){};
    default public void OnExternalWalletReconnected(){};
    default public void OnClaimPromotion(int error_code){};
    default public void onUnblindedTokensReady() {}
    default public void onReconcileComplete(int resultCode, int rewardsType, double amount) {}
    default public void OnRefreshPublisher(int status, String publisherKey){};
    default public void onSendContribution(boolean result){};
    default public void OnGetAdsAccountStatement(boolean success, double nextPaymentDate,
            int adsReceivedThisMonth, double minEarningsThisMonth, double maxEarningsThisMonth,
            double minEarningsLastMonth, double maxEarningsLastMonth){};
    default public void onPublisherBanner(String jsonBannerInfo){};
    default public void onGetPublishersVisitedCount(int count){};
    default public void onGetUserType(int userType){};
    default public void onCompleteReset(boolean success) {}

    public default void onTermsOfServiceUpdateAccepted() {}
}
