// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as preferencesAPI from './preferences'
import * as statsAPI from './stats'
import * as privateTabDataAPI from './privateTabData'
import * as torTabDataAPI from './torTabData'
import * as wallpaper from './wallpaper'
import * as storage from '../storage/new_tab_storage'
import { saveShowBinance, saveShowCryptoDotCom, saveShowFTX, saveShowGemini, saveWidgetVisibilityMigrated } from '../api/preferences'

export type InitialData = {
  preferences: NewTab.Preferences
  stats: statsAPI.Stats
  privateTabData: privateTabDataAPI.PrivateTabData
  torTabData: torTabDataAPI.TorTabData
  wallpaperData?: NewTab.Wallpaper
  braveTalkSupported: boolean
  geminiSupported: boolean
  binanceSupported: boolean
  cryptoDotComSupported: boolean
  ftxSupported: boolean
}

export type PreInitialRewardsData = {
  rewardsEnabled: boolean
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

async function migrateInitialDataForWidgetVisibility (initialData: InitialData) {
  // Other widget's auth state can be fetched from local storage.
  // But, need to fetch ftx auth state to migrate.
  // Start migration when ftx auth state is ready.
  const ftxUserAuthed = await new Promise(resolve => chrome.ftx.getAccountBalances((balances, authInvalid) => {
    resolve(!authInvalid)
  }))
  const peristentState: NewTab.PersistentState = storage.load()
  const widgetLookupTable = {
    'braveTalk': {
      display: initialData.braveTalkSupported && initialData.preferences.showBraveTalk,
      isCrypto: false
    },
    'rewards': {
      display: initialData.preferences.showRewards,
      isCrypto: false
    },
    'binance': {
      display: initialData.binanceSupported && initialData.preferences.showBinance,
      isCrypto: true,
      userAuthed: peristentState.binanceState.userAuthed
    },
    'cryptoDotCom': {
      display: initialData.cryptoDotComSupported && initialData.preferences.showCryptoDotCom,
      isCrypto: true,
      userAuthed: false
    },
    'ftx': {
      display: initialData.ftxSupported && initialData.preferences.showFTX,
      isCrypto: true,
      userAuthed: ftxUserAuthed
    },
    'gemini': {
      display: initialData.geminiSupported && initialData.preferences.showGemini,
      isCrypto: true,
      userAuthed: peristentState.geminiState.userAuthed
    }
  }

  // Find crypto widget that is foremost and visible.
  let foremostVisibleCryptoWidget = ''
  const lastIndex = peristentState.widgetStackOrder.length - 1
  for (let i = lastIndex; i >= 0; --i) {
    const widget = widgetLookupTable[peristentState.widgetStackOrder[i]]
    if (!widget) {
      console.error('Update above lookup table')
      continue
    }

    if (!widget.display) {
      continue
    }

    if (widget.isCrypto) {
      foremostVisibleCryptoWidget = peristentState.widgetStackOrder[i]
    }
    // Found visible foremost widget in the widget stack. Go out.
    break
  }

  const widgetsShowState = {
    'binance': false,
    'cryptoDotCom': false,
    'ftx': false,
    'gemini': false
  }

  for (const key in widgetsShowState) {
    // Show foremost visible crypto widget regardless of auth state
    // and show user authed crypto widget.
    if (key === foremostVisibleCryptoWidget ||
        widgetLookupTable[key].userAuthed) {
      widgetsShowState[key] = true
    }
  }

  // These don't return promise so we can't await and then fetch new preferences,
  // instead make manual changes to initialData.preferences after.
  saveShowBinance(widgetsShowState.binance)
  saveShowCryptoDotCom(widgetsShowState.cryptoDotCom)
  saveShowFTX(widgetsShowState.ftx)
  saveShowGemini(widgetsShowState.gemini)
  saveWidgetVisibilityMigrated()

  initialData.preferences = {
    ...initialData.preferences,
    showBinance: widgetsShowState.binance,
    showCryptoDotCom: widgetsShowState.cryptoDotCom,
    showFTX: widgetsShowState.ftx,
    showGemini: widgetsShowState.gemini
  }
}

// Gets all data required for the first render of the page
export async function getInitialData (): Promise<InitialData> {
  try {
    console.timeStamp('Getting initial data...')
    const [
      preferences,
      stats,
      privateTabData,
      torTabData,
      wallpaperData,
      braveTalkSupported,
      geminiSupported,
      cryptoDotComSupported,
      ftxSupported,
      binanceSupported
    ] = await Promise.all([
      preferencesAPI.getPreferences(),
      statsAPI.getStats(),
      privateTabDataAPI.getPrivateTabData(),
      torTabDataAPI.getTorTabData(),
      !isIncognito ? wallpaper.getWallpaper() : Promise.resolve(undefined),
      new Promise((resolve) => {
        if (!('braveTalk' in chrome)) {
          resolve(false)
          return
        }

        chrome.braveTalk.isSupported((supported: boolean) => {
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
    const initialData = {
      preferences,
      stats,
      privateTabData,
      torTabData,
      wallpaperData,
      braveTalkSupported,
      geminiSupported,
      cryptoDotComSupported,
      ftxSupported,
      binanceSupported
    } as InitialData

    if (!initialData.preferences.widgetVisibilityMigrated) {
      await migrateInitialDataForWidgetVisibility(initialData)
    }

    return initialData
  } catch (e) {
    console.error(e)
    throw Error('Error getting initial data')
  }
}

export async function getRewardsPreInitialData (): Promise<PreInitialRewardsData> {
  const [rewardsEnabled, enabledAds, adsSupported] = await Promise.all([
    new Promise<boolean>(
      (resolve) => chrome.braveRewards.getRewardsEnabled(resolve)),
    new Promise<boolean>(
      (resolve) => chrome.braveRewards.getAdsEnabled(resolve)),
    new Promise<boolean>(
      (resolve) => chrome.braveRewards.getAdsSupported(resolve))
  ])

  return {
    rewardsEnabled,
    enabledAds,
    adsSupported
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
      new Promise(resolve => chrome.braveRewards.getBalanceReport(new Date().getMonth() + 1, new Date().getFullYear(), (report: NewTab.RewardsBalanceReport) => {
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
