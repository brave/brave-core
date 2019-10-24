/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import rewardsPanelActions from '../actions/rewardsPanelActions'

// Handle all rewards events and pass to actions
chrome.braveRewards.onWalletInitialized.addListener((result: RewardsExtension.Result) => {
  rewardsPanelActions.onWalletInitialized(result)
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

chrome.braveRewards.onCurrentReport.addListener((properties: RewardsExtension.Report) => {
  rewardsPanelActions.onCurrentReport(properties)
})

chrome.braveRewards.onPromotion.addListener((properties: RewardsExtension.PromotionResponse) => {
  rewardsPanelActions.onPromotion(properties)
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

chrome.braveRewards.onPublisherListNormalized.addListener((properties: RewardsExtension.PublisherNormalized[]) => {
  rewardsPanelActions.onPublisherListNormalized(properties)
})

chrome.braveRewards.onExcludedSitesChanged.addListener((properties: RewardsExtension.ExcludedSitesChanged) => {
  rewardsPanelActions.onExcludedSitesChanged(properties)
})

chrome.braveRewards.onRecurringTipSaved.addListener((success: boolean) => {
  if (success) {
    chrome.braveRewards.getRecurringTips((tips: RewardsExtension.RecurringTips) => {
      rewardsPanelActions.onRecurringTips(tips)
    })
  }
})

chrome.braveRewards.onRecurringTipRemoved.addListener((success: boolean) => {
  if (success) {
    chrome.braveRewards.getRecurringTips((tips: RewardsExtension.RecurringTips) => {
      rewardsPanelActions.onRecurringTips(tips)
    })
  }
})

chrome.braveRewards.onPendingContributionSaved.addListener((result: number) => {
  if (result === 0) {
    chrome.braveRewards.getPendingContributionsTotal(((amount: number) => {
      rewardsPanelActions.OnPendingContributionsTotal(amount)
    }))
  }
})

chrome.braveRewards.onReconcileComplete.addListener((result: number, type: number) => {
  if (result === 0) {
    chrome.braveRewards.fetchBalance((balance: RewardsExtension.Balance) => {
      rewardsPanelActions.onBalance(balance)
    })
  }
})

chrome.braveRewards.onDisconnectWallet.addListener((properties: {result: number, walletType: string}) => {
  if (properties.result === 0) {
    chrome.braveRewards.getExternalWallet(properties.walletType, (result: number, wallet: RewardsExtension.ExternalWallet) => {
      rewardsPanelActions.onExternalWallet(wallet)
    })

    chrome.braveRewards.fetchBalance((balance: RewardsExtension.Balance) => {
      rewardsPanelActions.onBalance(balance)
    })
  }
})

// Fetch initial data required to refresh state, keeping in mind
// that the extension process be restarted at any time.
// TODO(petemill): Move to initializer function or single 'init' action.
chrome.braveRewards.getRewardsMainEnabled((enabledMain: boolean) => {
  rewardsPanelActions.onEnabledMain(enabledMain)
  if (enabledMain) {
    chrome.braveRewards.fetchPromotions()
    chrome.braveRewards.fetchBalance((balance: RewardsExtension.Balance) => {
      rewardsPanelActions.onBalance(balance)
    })
    chrome.braveRewards.getAllNotifications((list: RewardsExtension.Notification[]) => {
      rewardsPanelActions.onAllNotifications(list)
    })
  }
})
