// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import getActions from './api/getActions'
import * as preferencesAPI from './api/preferences'
import * as statsAPI from './api/stats'
import * as topSitesAPI from './api/topSites'
import * as privateTabDataAPI from './api/privateTabData'
import * as torTabDataAPI from './api/torTabData'
import getNTPBrowserAPI, { CustomBackground } from './api/background'
import { getInitialData, getRewardsInitialData, getRewardsPreInitialData } from './api/initialData'

async function updatePreferences (prefData: NewTab.Preferences) {
  getActions().preferencesUpdated(prefData)
}

async function updateStats (statsData: statsAPI.Stats) {
  getActions().statsUpdated(statsData)
}

async function updatePrivateTabData (data: privateTabDataAPI.PrivateTabData) {
  getActions().privateTabDataUpdated(data)
}

async function updateTorTabData (data: torTabDataAPI.TorTabData) {
  getActions().torTabDataUpdated(data)
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

async function onCustomBackgroundUpdated (customBackground: CustomBackground) {
  getActions().customBackgroundUpdated(customBackground)
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
    torTabDataAPI.addChangeListener(updateTorTabData)
    getNTPBrowserAPI().addCustomBackgroundUpdatedListener(onCustomBackgroundUpdated)
  })
  .catch(e => {
    console.error('New Tab Page fatal error:', e)
  })
}

export function rewardsInitData () {
  getRewardsPreInitialData()
  .then((preInitialRewardsData) => {
    getActions().setPreInitialRewardsData(preInitialRewardsData)
    fetchRewardsData()
    setRewardsFetchInterval()
  })
  .catch(e => {
    console.error('Error fetching pre-initial rewards data: ', e)
  })
}

function setRewardsFetchInterval () {
  window.setInterval(() => {
    fetchRewardsData()
  }, 30000)
}

function fetchRewardsData () {
  chrome.braveRewards.isInitialized((initialized: boolean) => {
    if (!initialized) {
      return
    }

    getRewardsInitialData()
    .then((initialRewardsData) => {
      getActions().setInitialRewardsData(initialRewardsData)
    })
    .catch(e => {
      console.error('Error fetching initial rewards data: ', e)
    })
  })
}

chrome.braveRewards.initialized.addListener((result: any | NewTab.RewardsResult) => {
  fetchRewardsData()
})

chrome.braveRewards.onAdsEnabled.addListener((enabled: boolean) => {
  getActions().onAdsEnabled(enabled)
})

chrome.braveRewards.onPromotions.addListener((result: number, promotions: NewTab.Promotion[]) => {
  getActions().onPromotions(result, promotions)
})

chrome.braveRewards.onPromotionFinish.addListener((result: number, promotion: NewTab.Promotion) => {
  getActions().onPromotionFinish(result, promotion)
})

chrome.braveRewards.onCompleteReset.addListener((properties: { success: boolean }) => {
  getActions().onCompleteReset(properties.success)
})
