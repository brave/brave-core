/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

public interface BraveRewardsObserver {
    public default void onCreateRewardsWallet(String result) {}

    public default void onRewardsParameters() {}

    public default void onBalance(boolean success) {}

    public default void onPublisherInfo(int tabId) {}

    public default void onGetCurrentBalanceReport(double[] report) {}

    public default void onNotificationAdded(String id, int type, long timestamp, String[] args) {}

    public default void onNotificationsCount(int count) {}

    public default void onGetLatestNotification(
            String id, int type, long timestamp, String[] args) {}

    public default void onNotificationDeleted(String id) {}

    public default void onGetAutoContributeProperties() {}

    public default void onGetAutoContributionAmount(double amount) {}

    public default void onGetReconcileStamp(long timestamp) {}

    public default void onRecurringDonationUpdated() {}

    public default void onResetTheWholeState(boolean success) {}

    public default void onGrantFinish(int result) {}

    public default void onGetExternalWallet(String externalWallet) {}

    public default void onGetAvailableCountries(String[] countries) {}

    public default void onExternalWalletConnected() {}

    public default void onExternalWalletLoggedOut() {}

    public default void onExternalWalletReconnected() {}

    public default void onClaimPromotion(int errorCode) {}

    public default void onUnblindedTokensReady() {}

    public default void onReconcileComplete(int resultCode, int rewardsType, double amount) {}

    public default void onRefreshPublisher(int status, String publisherKey) {}

    public default void onSendContribution(boolean result) {}

    public default void onGetAdsAccountStatement(
            boolean success,
            double nextPaymentDate,
            int adsReceivedThisMonth,
            double minEarningsThisMonth,
            double maxEarningsThisMonth,
            double minEarningsLastMonth,
            double maxEarningsLastMonth) {}

    public default void onPublisherBanner(String jsonBannerInfo) {}

    public default void onGetPublishersVisitedCount(int count) {}

    public default void onGetUserType(int userType) {}

    public default void onCompleteReset(boolean success) {}

    public default void onTermsOfServiceUpdateAccepted() {}
}
