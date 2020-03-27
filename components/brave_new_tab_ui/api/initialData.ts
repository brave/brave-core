// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as preferencesAPI from './preferences'
import * as statsAPI from './stats'
import * as privateTabDataAPI from './privateTabData'
import * as topSitesAPI from './topSites'
import * as brandedWallpaper from './brandedWallpaper'

export type InitialData = {
  preferences: preferencesAPI.Preferences
  stats: statsAPI.Stats
  privateTabData: privateTabDataAPI.PrivateTabData
  topSites: chrome.topSites.MostVisitedURL[]
  defaultTopSites: undefined | NewTab.DefaultTopSite[]
  brandedWallpaperData: undefined | NewTab.BrandedWallpaper
}

export type PreInitialRewardsData = {
  enabledAds: boolean
  adsSupported: boolean
  enabledMain: boolean
}

export type InitialRewardsData = {
  adsEstimatedEarnings: number
  report: NewTab.RewardsBalanceReport
  balance: NewTab.RewardsBalance
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
      topSites,
      defaultTopSites,
      brandedWallpaperData
    ] = await Promise.all([
      preferencesAPI.getPreferences(),
      statsAPI.getStats(),
      privateTabDataAPI.getPrivateTabData(),
      topSitesAPI.getTopSites(),
      !isIncognito ? brandedWallpaper.getDefaultTopSites() : Promise.resolve(undefined),
      !isIncognito ? brandedWallpaper.getBrandedWallpaper() : Promise.resolve(undefined)
    ])
    console.timeStamp('Got all initial data.')
    return {
      preferences,
      stats,
      privateTabData,
      topSites,
      defaultTopSites,
      brandedWallpaperData
    }
  } catch (e) {
    console.error(e)
    throw Error('Error getting initial data')
  }
}

export async function getBinanceBlackList (): Promise<any> {
  try {
    const [
      onlyAnonWallet,
      isSupportedRegion
    ] = await Promise.all([
      new Promise(resolve => chrome.braveRewards.onlyAnonWallet((onlyAnonWallet: boolean) => {
        resolve(!!onlyAnonWallet)
      })),
      new Promise(resolve => chrome.binance.isSupportedRegion((supported: boolean) => {
        resolve(!!supported)
      }))
    ])
    return {
      isSupportedRegion,
      onlyAnonWallet
    }
  } catch (err) {
    throw Error(err)
  }
}

export async function getRewardsPreInitialData (): Promise<PreInitialRewardsData> {
  try {
    const [
      enabledAds,
      adsSupported,
      enabledMain
    ] = await Promise.all([
      new Promise(resolve => chrome.braveRewards.getAdsEnabled((enabledAds: boolean) => {
        resolve(enabledAds)
      })),
      new Promise(resolve => chrome.braveRewards.getAdsSupported((adsSupported: boolean) => {
        resolve(adsSupported)
      })),
      new Promise(resolve => chrome.braveRewards.getRewardsMainEnabled((enabledMain: boolean) => {
        resolve(enabledMain)
      }))
    ])
    return {
      enabledAds,
      adsSupported,
      enabledMain
    } as PreInitialRewardsData
  } catch (err) {
    throw Error(err)
  }
}

export async function getRewardsInitialData (): Promise<InitialRewardsData> {
  try {
    const [
      adsEstimatedEarnings,
      report,
      balance
    ] = await Promise.all([
      new Promise(resolve => chrome.braveRewards.getAdsEstimatedEarnings((adsEstimatedEarnings: number) => {
        resolve(adsEstimatedEarnings)
      })),
      new Promise(resolve => chrome.braveRewards.getBalanceReport(new Date().getMonth() + 1, new Date().getFullYear(),(report: NewTab.RewardsBalanceReport) => {
        resolve(report)
      })),
      new Promise(resolve => chrome.braveRewards.fetchBalance((balance: NewTab.RewardsBalance) => {
        resolve(balance)
      })),
      new Promise(resolve => {
        chrome.braveRewards.fetchPromotions()
        resolve(true)
      })
    ])
    return {
      adsEstimatedEarnings,
      report,
      balance
    } as InitialRewardsData
  } catch (err) {
    throw Error(err)
  }
}
