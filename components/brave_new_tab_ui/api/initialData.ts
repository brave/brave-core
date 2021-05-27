// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as preferencesAPI from './preferences'
import * as statsAPI from './stats'
import * as privateTabDataAPI from './privateTabData'
import * as torTabDataAPI from './torTabData'
import * as brandedWallpaper from './brandedWallpaper'

export type InitialData = {
  preferences: NewTab.Preferences
  stats: statsAPI.Stats
  privateTabData: privateTabDataAPI.PrivateTabData
  torTabData: torTabDataAPI.TorTabData
  brandedWallpaperData: undefined | NewTab.BrandedWallpaper
  togetherSupported: boolean
  geminiSupported: boolean
  binanceSupported: boolean
  cryptoDotComSupported: boolean
  ftxSupported: boolean
}

export type PreInitialRewardsData = {
  enabledAds: boolean
  adsSupported: boolean
}

export type InitialRewardsData = {
  report: NewTab.RewardsBalanceReport
  balance: NewTab.RewardsBalance
  adsAccountStatement: NewTab.AdsAccountStatement
  parameters: NewTab.RewardsParameters
}

const isIncognito: boolean = chrome.extension.inIncognitoContext

// Gets all data required for the first render of the page
export async function getInitialData (): Promise<InitialData> {
  try {
    console.timeStamp('Getting initial data...')
    const [
      preferences,
      stats,
      privateTabData,
      torTabData,
      brandedWallpaperData,
      togetherSupported,
      geminiSupported,
      cryptoDotComSupported,
      ftxSupported,
      binanceSupported
    ] = await Promise.all([
      preferencesAPI.getPreferences(),
      statsAPI.getStats(),
      privateTabDataAPI.getPrivateTabData(),
      torTabDataAPI.getTorTabData(),
      !isIncognito ? brandedWallpaper.getBrandedWallpaper() : Promise.resolve(undefined),
      new Promise((resolve) => {
        if (!('braveTogether' in chrome)) {
          resolve(false)
          return
        }

        chrome.braveTogether.isSupported((supported: boolean) => {
          resolve(supported)
        })
      }),
      new Promise((resolve) => {
        chrome.gemini.isSupported((supported: boolean) => {
          resolve(supported)
        })
      }),
      new Promise((resolve) => {
        chrome.cryptoDotCom.isSupported((supported: boolean) => {
          resolve(supported)
        })
      }),
      new Promise((resolve) => {
        chrome.ftx.isSupported((supported: boolean) => {
          resolve(supported)
        })
      }),
      new Promise((resolve) => {
        chrome.binance.isSupportedRegion((supported: boolean) => {
          resolve(supported)
        })
      })
    ])
    console.timeStamp('Got all initial data.')
    return {
      preferences,
      stats,
      privateTabData,
      torTabData,
      brandedWallpaperData,
      togetherSupported,
      geminiSupported,
      cryptoDotComSupported,
      ftxSupported,
      binanceSupported
    } as InitialData
  } catch (e) {
    console.error(e)
    throw Error('Error getting initial data')
  }
}

export async function getRewardsPreInitialData (): Promise<PreInitialRewardsData> {
  try {
    const [
      enabledAds,
      adsSupported
    ] = await Promise.all([
      new Promise(resolve => chrome.braveRewards.getAdsEnabled((enabledAds: boolean) => {
        resolve(enabledAds)
      })),
      new Promise(resolve => chrome.braveRewards.getAdsSupported((adsSupported: boolean) => {
        resolve(adsSupported)
      }))
    ])
    return {
      enabledAds,
      adsSupported
    } as PreInitialRewardsData
  } catch (err) {
    throw Error(err)
  }
}

export async function getRewardsInitialData (): Promise<InitialRewardsData> {
  try {
    const [
      adsAccountStatement,
      report,
      balance,
      parameters
    ] = await Promise.all([
      new Promise(resolve => chrome.braveRewards.getAdsAccountStatement((success: boolean, adsAccountStatement: NewTab.AdsAccountStatement) => {
        resolve(success ? adsAccountStatement : undefined)
      })),
      new Promise(resolve => chrome.braveRewards.getBalanceReport(new Date().getMonth() + 1, new Date().getFullYear(),(report: NewTab.RewardsBalanceReport) => {
        resolve(report)
      })),
      new Promise(resolve => chrome.braveRewards.fetchBalance((balance: NewTab.RewardsBalance) => {
        resolve(balance)
      })),
      new Promise(resolve => chrome.braveRewards.getRewardsParameters((parameters: NewTab.RewardsParameters) => {
        resolve(parameters)
      })),
      new Promise(resolve => {
        chrome.braveRewards.fetchPromotions()
        resolve(true)
      })
    ])
    return {
      adsAccountStatement,
      report,
      balance,
      parameters
    } as InitialRewardsData
  } catch (err) {
    throw Error(err)
  }
}
