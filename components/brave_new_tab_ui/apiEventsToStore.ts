// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import getActions from './api/getActions'
import * as preferencesAPI from './api/preferences'
import * as statsAPI from './api/stats'
import * as topSitesAPI from './api/topSites'
import * as privateTabDataAPI from './api/privateTabData'
import * as newTabAdsDataAPI from './api/newTabAdsData'
import getNTPBrowserAPI, { Background, CustomBackground } from './api/background'
import { getInitialData, getRewardsInitialData, getRewardsPreInitialData } from './api/initialData'
import * as backgroundData from './data/backgrounds'
import { loadTimeData } from '$web-common/loadTimeData'

async function updatePreferences (prefData: NewTab.Preferences) {
  getActions().preferencesUpdated(prefData)
}

async function updateStats (statsData: statsAPI.Stats) {
  getActions().statsUpdated(statsData)
}

async function updatePrivateTabData (data: privateTabDataAPI.PrivateTabData) {
  getActions().privateTabDataUpdated(data)
}

async function updateNewTabAdsData (data: newTabAdsDataAPI.NewTabAdsData) {
  getActions().newTabAdsDataUpdated(data)
}

function onRewardsToggled (prefData: NewTab.Preferences): void {
  if (prefData.showRewards) {
    rewardsInitData()
  }
}

async function onMostVisitedInfoChanged (topSites: topSitesAPI.MostVisitedInfoChanged) {
  getActions().tilesUpdated(topSites.tiles)
  getActions().topSitesStateUpdated(topSites.visible, topSites.custom_links_enabled, topSites.custom_links_num)
}

async function onBackgroundUpdated (background: Background) {
  getActions().customBackgroundUpdated(background)
}

async function onCustomImageBackgroundsUpdated (backgrounds: CustomBackground[]) {
  getActions().customImageBackgroundsUpdated(backgrounds)
}

// Not marked as async so we don't return a promise
// and confuse callers
export function wireApiEventsToStore () {
  // Get initial data and dispatch to store
  getInitialData()
  .then((initialData) => {
    if (initialData.preferences.showRewards) {
      rewardsInitData()
    }
    getActions().setInitialData(initialData)
    // Listen for API changes and dispatch to store
    topSitesAPI.addMostVistedInfoChangedListener(onMostVisitedInfoChanged)
    topSitesAPI.updateMostVisitedInfo()
    statsAPI.addChangeListener(updateStats)
    preferencesAPI.addChangeListener(updatePreferences)
    preferencesAPI.addChangeListener(onRewardsToggled)
    privateTabDataAPI.addChangeListener(updatePrivateTabData)
    newTabAdsDataAPI.addChangeListener(updateNewTabAdsData)
    backgroundData.updateImages(initialData.braveBackgrounds)

    getNTPBrowserAPI().addBackgroundUpdatedListener(onBackgroundUpdated)
    getNTPBrowserAPI().addCustomImageBackgroundsUpdatedListener(onCustomImageBackgroundsUpdated)
    getNTPBrowserAPI().addSearchPromotionDisabledListener(() => getActions().searchPromotionDisabled())
    if (loadTimeData.getBoolean('vpnWidgetSupported')) {
      getActions().braveVPN.initialize(initialData.purchasedState)
    }
  })
  .catch(e => {
    console.error('New Tab Page fatal error:', e)
  })
}

export function rewardsInitData () {
  getRewardsPreInitialData().then((preInitialRewardsData) => {
    getActions().setPreInitialRewardsData(preInitialRewardsData)

    chrome.braveRewards.isInitialized((isInitialized) => {
      if (isInitialized) {
        getRewardsInitialData().then((data) => {
          getActions().setInitialRewardsData(data)
        })
      }
    })

    setRewardsFetchInterval()
  })
  .catch(e => {
    console.error('Error fetching pre-initial rewards data: ', e)
  })
}

let intervalId = 0
function setRewardsFetchInterval () {
  if (!intervalId) {
    intervalId = window.setInterval(() => { fetchRewardsData() }, 30000)
  }
}

function fetchRewardsData () {
  chrome.braveRewards.isInitialized((isInitialized) => {
    if (!isInitialized) {
      return
    }

    Promise.all([getRewardsPreInitialData(), getRewardsInitialData()]).then(
      ([preInitialData, initialData]) => {
        getActions().setPreInitialRewardsData(preInitialData)
        getActions().setInitialRewardsData(initialData)
      })
  })
}

chrome.braveRewards.initialized.addListener(fetchRewardsData)

chrome.braveRewards.onRewardsWalletCreated.addListener(fetchRewardsData)

chrome.braveRewards.onCompleteReset.addListener((properties: { success: boolean }) => {
  getActions().onCompleteReset(properties.success)
})

chrome.braveRewards.onSelfCustodyInviteDismissed.addListener(fetchRewardsData)

chrome.braveRewards.onTermsOfServiceUpdateAccepted.addListener(fetchRewardsData)

chrome.braveRewards.onExternalWalletLoggedOut.addListener(fetchRewardsData)

chrome.braveRewards.onExternalWalletDisconnected.addListener(fetchRewardsData)
