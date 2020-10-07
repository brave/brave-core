// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import getActions from './api/getActions'
import * as preferencesAPI from './api/preferences'
import * as statsAPI from './api/stats'
import * as privateTabDataAPI from './api/privateTabData'
import * as torTabDataAPI from './api/torTabData'
import { getInitialData, getRewardsInitialData, getRewardsPreInitialData, getBinanceBlackList } from './api/initialData'

async function updatePreferences (prefData: preferencesAPI.Preferences) {
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

function onRewardsToggled (prefData: preferencesAPI.Preferences): void {
  if (prefData.showRewards) {
    rewardsInitData()
  }
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
    binanceInitData()
    getActions().setInitialData(initialData)
    getActions().setFirstRenderGridSitesData(initialData)
    // Listen for API changes and dispatch to store
    statsAPI.addChangeListener(updateStats)
    preferencesAPI.addChangeListener(updatePreferences)
    preferencesAPI.addChangeListener(onRewardsToggled)
    privateTabDataAPI.addChangeListener(updatePrivateTabData)
    torTabDataAPI.addChangeListener(updateTorTabData)
  })
  .catch(e => {
    console.error('New Tab Page fatal error:', e)
  })
}

export function rewardsInitData () {
  getRewardsPreInitialData()
  .then((preInitialRewardsData) => {
    getActions().setPreInitialRewardsData(preInitialRewardsData)

    chrome.braveRewards.getWalletExists((exists: boolean) => {
      getActions().onWalletExists(exists)
      if (exists) {
        if (!preInitialRewardsData.enabledMain) {
          return
        }

        fetchCreatedWalletData()
        setRewardsFetchInterval()
      }
    })
  })
  .catch(e => {
    console.error('Error fetching pre-initial rewards data: ', e)
  })
}

function binanceInitData () {
  getBinanceBlackList()
  .then(({ isSupportedRegion, onlyAnonWallet }) => {
    if (onlyAnonWallet || !isSupportedRegion) {
      getActions().removeStackWidget('binance')
    }
    getActions().setOnlyAnonWallet(onlyAnonWallet)
    getActions().setBinanceSupported(isSupportedRegion && !onlyAnonWallet)
  })
  .catch(e => {
    console.error('Error fetching binance init data')
  })
}

function setRewardsFetchInterval () {
  window.setInterval(() => {
    chrome.braveRewards.getRewardsMainEnabled((enabledMain: boolean) => {
      if (!enabledMain) {
        return
      }
      chrome.braveRewards.getWalletExists((exists: boolean) => {
        if (exists) {
          fetchCreatedWalletData()
        }
      })
    })
  }, 30000)
}

function fetchCreatedWalletData () {
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

chrome.braveRewards.walletCreated.addListener(() => {
  getActions().onWalletInitialized(12)
})

chrome.braveRewards.walletCreationFailed.addListener((result: any | NewTab.RewardsResult) => {
  getActions().onWalletInitialized(result)
})

chrome.braveRewards.initialized.addListener((result: any | NewTab.RewardsResult) => {
  rewardsInitData()
})

chrome.braveRewards.onEnabledMain.addListener((enabledMain: boolean) => {
  if (enabledMain) {
    chrome.braveRewards.getAdsEnabled((enabledAds: boolean) => {
      getActions().onEnabledMain(enabledMain, enabledAds)
    })
  } else {
    getActions().onEnabledMain(false, false)
  }
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
