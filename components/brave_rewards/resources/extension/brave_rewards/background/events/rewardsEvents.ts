/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import rewardsPanelActions from '../actions/rewardsPanelActions'

chrome.braveRewards.onWalletCreated.addListener(() => {
  rewardsPanelActions.onWalletCreated()
})

chrome.braveRewards.onPublisherData.addListener((windowId: number, publisher: RewardsExtension.Publisher) => {
  rewardsPanelActions.onPublisherData(windowId, publisher)

  // Get publisher amounts
  if (publisher && publisher.publisher_key) {
    chrome.braveRewards.getPublisherBanner(publisher.publisher_key, ((banner: RewardsExtension.PublisherBanner) => {
      rewardsPanelActions.onPublisherBanner(banner)
    }))
  }
})

chrome.braveRewards.onWalletProperties.addListener((properties: RewardsExtension.WalletProperties) => {
  rewardsPanelActions.onWalletProperties(properties)
})

chrome.braveRewards.onCurrentReport.addListener((properties: RewardsExtension.Report) => {
  rewardsPanelActions.onCurrentReport(properties)
})

chrome.braveRewards.onGrant.addListener((properties: RewardsExtension.GrantResponse) => {
  rewardsPanelActions.onGrant(properties)
})

chrome.braveRewards.onGrantCaptcha.addListener((captcha: RewardsExtension.Captcha) => {
  rewardsPanelActions.onGrantCaptcha(captcha)
})

chrome.braveRewards.onGrantFinish.addListener((properties: RewardsExtension.GrantFinish) => {
  rewardsPanelActions.onGrantFinish(properties)
})

chrome.rewardsNotifications.onNotificationAdded.addListener((id: string, type: number, timestamp: number, args: string[]) => {
  rewardsPanelActions.onNotificationAdded(id, type, timestamp, args)
})

chrome.rewardsNotifications.onNotificationDeleted.addListener((id: string, type: number, timestamp: number) => {
  rewardsPanelActions.onNotificationDeleted(id, type, timestamp)
})

chrome.braveRewards.onEnabledMain.addListener((enabledMain: boolean) => {
  rewardsPanelActions.onEnabledMain(enabledMain)
})

chrome.braveRewards.onPendingContributionSaved.addListener((result: number) => {
  if (result === 0) {
    chrome.braveRewards.getPendingContributionsTotal(((amount: number) => {
      rewardsPanelActions.OnPendingContributionsTotal(amount)
    }))
  }
})

chrome.braveRewards.onWalletFailed.addListener(() => {
  rewardsPanelActions.onWalletCreateFailed()
})

chrome.braveRewards.onPublisherListNormalized.addListener((properties: RewardsExtension.PublisherNormalized[]) => {
  rewardsPanelActions.onPublisherListNormalized(properties)
})

chrome.braveRewards.onExcludedSitesChanged.addListener((properties: RewardsExtension.ExcludedSitesChanged) => {
  rewardsPanelActions.onExcludedSitesChanged(properties)
})

chrome.braveRewards.onRecurringDonations.addListener((donations: Record<string, number>[]) => {
  rewardsPanelActions.onRecurringDonations({
    recurringDonations: donations
  })
})
