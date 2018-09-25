/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import rewardsPanelActions from '../actions/rewardsPanelActions'

chrome.braveRewards.onWalletCreated.addListener(() => {
  rewardsPanelActions.onWalletCreated()
})

chrome.braveRewards.onPublisherData.addListener((windowId: number, publisher: RewardsExtension.Publisher) => {
  rewardsPanelActions.onPublisherData(windowId, publisher)
})

chrome.braveRewards.onWalletProperties.addListener((properties: RewardsExtension.WalletProperties) => {
  rewardsPanelActions.onWalletProperties(properties)
})

chrome.braveRewards.onCurrentReport.addListener((properties: RewardsExtension.Report) => {
  rewardsPanelActions.onCurrentReport(properties)
})
