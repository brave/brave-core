/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

public interface BraveRewardsObserver {
  public void OnWalletInitialized(int error_code);
  public void OnWalletProperties(int error_code);
  public void OnPublisherInfo(int tabId);
  public void OnGetCurrentBalanceReport(String[] report);
  public void OnNotificationAdded(String id, int type, long timestamp,
        String[] args);
  public void OnNotificationsCount(int count);
  public void OnGetLatestNotification(String id, int type, long timestamp,
            String[] args);
  public void OnNotificationDeleted(String id);
  public void OnIsWalletCreated(boolean created);
  public void OnGetPendingContributionsTotal(double amount);
  public void OnGetRewardsMainEnabled(boolean enabled);
  public void OnGetAutoContributeProps();
  public void OnGetReconcileStamp(long timestamp);
  public void OnRecurringDonationUpdated();
  public void OnResetTheWholeState(boolean success);
  public void OnRewardsMainEnabled(boolean enabled);
}
