// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Utils
import { debounce } from '../../common/debounce'

export const keyName = 'new-tab-data'

export const defaultState: NewTab.State = {
  initialDataLoaded: false,
  textDirection: window.loadTimeData.getString('textdirection'),
  featureFlagBraveNTPSponsoredImagesWallpaper: window.loadTimeData.getBoolean('featureFlagBraveNTPSponsoredImagesWallpaper'),
  showBackgroundImage: false,
  showStats: false,
  showToday: false,
  showClock: false,
  clockFormat: '',
  showTopSites: false,
  customLinksEnabled: false,
  customLinksNum: 0,
  showRewards: false,
  showTogether: false,
  showBinance: false,
  showGemini: false,
  showBitcoinDotCom: false,
  showCryptoDotCom: false,
  showFTX: false,
  hideAllWidgets: false,
  brandedWallpaperOptIn: false,
  isBrandedWallpaperNotificationDismissed: true,
  isBraveTodayOptedIn: false,
  showEmptyPage: false,
  togetherSupported: false,
  geminiSupported: false,
  binanceSupported: false,
  bitcoinDotComSupported: false,
  cryptoDotComSupported: false,
  ftxSupported: false,
  isIncognito: chrome.extension.inIncognitoContext,
  useAlternativePrivateSearchEngine: false,
  torCircuitEstablished: false,
  torInitProgress: '',
  isTor: false,
  isQwant: false,
  stats: {
    adsBlockedStat: 0,
    javascriptBlockedStat: 0,
    bandwidthSavedStat: 0,
    httpsUpgradesStat: 0,
    fingerprintingBlockedStat: 0
  },
  togetherPromptDismissed: false,
  rewardsState: {
    adsAccountStatement: {
      nextPaymentDate: 0,
      adsReceivedThisMonth: 0,
      earningsThisMonth: 0,
      earningsLastMonth: 0
    },
    balance: {
      total: 0,
      wallets: {}
    },
    dismissedNotifications: [],
    enabledAds: false,
    adsSupported: false,
    promotions: [],
    totalContribution: 0.0,
    parameters: {
      rate: 0,
      monthlyTipChoices: []
    }
  },
  currentStackWidget: '',
  removedStackWidgets: [],
  // Order is ascending, with last entry being in the foreground
  widgetStackOrder: ['ftx', 'cryptoDotCom', 'binance', 'gemini', 'rewards'],
  binanceState: {
    userTLD: 'com',
    userLocale: 'en',
    initialFiat: 'USD',
    initialAmount: '',
    initialAsset: 'BTC',
    userTLDAutoSet: false,
    hideBalance: true,
    binanceClientUrl: '',
    userAuthed: false,
    authInProgress: false,
    btcBalanceValue: '0.00',
    accountBalances: {},
    assetBTCValues: {},
    assetBTCVolumes: {},
    assetUSDValues: {},
    btcPrice: '0.00',
    btcVolume: '0',
    assetDepositInfo: {},
    assetDepoitQRCodeSrcs: {},
    convertAssets: {},
    accountBTCValue: '0.00',
    accountBTCUSDValue: '0.00',
    disconnectInProgress: false,
    authInvalid: false,
    selectedView: 'summary',
    depositInfoSaved: false
  },
  geminiState: {
    geminiClientUrl: '',
    userAuthed: false,
    authInProgress: false,
    tickerPrices: {},
    selectedView: 'balance',
    assetAddresses: {},
    assetAddressQRCodes: {},
    hideBalance: true,
    accountBalances: {},
    disconnectInProgress: false,
    authInvalid: false
  },
  cryptoDotComState: {
    optInTotal: false,
    optInBTCPrice: false,
    optInMarkets: false,
    tickerPrices: {},
    losersGainers: {},
    supportedPairs: {},
    charts: []
  },
  ftxState: {
    optedIntoMarkets: false
  }
}

if (chrome.extension.inIncognitoContext) {
  defaultState.isTor = window.loadTimeData.getBoolean('isTor')
  defaultState.isQwant = window.loadTimeData.getBoolean('isQwant')
}

// For users upgrading to the new list based widget stack state,
// a list in the current format will need to be generated based on their
// previous configuration.
const getMigratedWidgetOrder = (state: NewTab.State) => {
  const {
    showRewards,
    showBinance,
    currentStackWidget
  } = state

  if (!showRewards && !showBinance) {
    return {
      widgetStackOrder: [],
      removedStackWidgets: ['rewards', 'binance']
    }
  }

  if (showRewards && !showBinance) {
    return {
      widgetStackOrder: ['rewards'],
      removedStackWidgets: ['binance']
    }
  }

  if (!showRewards && showBinance) {
    return {
      widgetStackOrder: ['binance'],
      removedStackWidgets: ['rewards']
    }
  }

  const widgetStackOrder = []
  const nonCurrentWidget = currentStackWidget === 'rewards'
    ? 'binance'
    : 'rewards'

  widgetStackOrder.push(currentStackWidget)
  widgetStackOrder.unshift(nonCurrentWidget)

  return {
    widgetStackOrder,
    removedStackWidgets: []
  }
}

export const migrateStackWidgetSettings = (state: NewTab.State) => {
  // Migrating to the new stack widget data format
  const { widgetStackOrder, removedStackWidgets } = getMigratedWidgetOrder(state)
  state.widgetStackOrder = widgetStackOrder as NewTab.StackWidget[]
  state.removedStackWidgets = removedStackWidgets as NewTab.StackWidget[]
  state.currentStackWidget = ''
  return state
}

// Ensure any new stack widgets introduced are put behind
// the others, and not re-added unecessarily if removed
// at one point.
export const addNewStackWidget = (state: NewTab.State) => {
  defaultState.widgetStackOrder.map((widget: NewTab.StackWidget) => {
    if (!state.widgetStackOrder.includes(widget) &&
        !state.removedStackWidgets.includes(widget)) {
      state.widgetStackOrder.unshift(widget)
    }
  })
  return state
}

// Replaces any stack widgets that were improperly removed
// as a result of https://github.com/brave/brave-browser/issues/10067
export const replaceStackWidgets = (state: NewTab.State) => {
  const {
    showBinance,
    showRewards,
    showTogether,
    togetherSupported,
    binanceSupported
  } = state
  const displayLookup = {
    'rewards': {
      display: showRewards
    },
    'binance': {
      display: binanceSupported && showBinance
    },
    'together': {
      display: togetherSupported && showTogether
    }
  }
  for (const key in displayLookup) {
    const widget = key as NewTab.StackWidget
    if (!state.widgetStackOrder.includes(widget) &&
        displayLookup[widget].display) {
      state.widgetStackOrder.unshift(widget)
    }
  }
  return state
}

const cleanData = (state: NewTab.State) => {
  const { rewardsState } = state

  if (typeof (rewardsState.totalContribution as any) === 'string') {
    rewardsState.totalContribution = 0
  }

  // nextPaymentDate updated from seconds-since-epoch-string to ms-since-epoch
  const { adsAccountStatement } = rewardsState
  if (adsAccountStatement &&
      typeof (adsAccountStatement.nextPaymentDate as any) === 'string') {
    adsAccountStatement.nextPaymentDate =
      Number(adsAccountStatement.nextPaymentDate) * 1000 || 0
  }

  if (!rewardsState.parameters) {
    rewardsState.parameters = defaultState.rewardsState.parameters
  }

  return state
}

export const load = (): NewTab.State => {
  const data: string | null = window.localStorage.getItem(keyName)
  let state = defaultState
  let storedState

  if (data) {
    try {
      storedState = JSON.parse(data)
      // add defaults for non-peristant data
      state = {
        ...state,
        ...storedState
      }
    } catch (e) {
      console.error('[NewTabData] Could not parse local storage data: ', e)
    }
  }
  return cleanData(state)
}

export const debouncedSave = debounce<NewTab.State>((data: NewTab.State) => {
  if (data) {
    const dataToSave = {
      togetherSupported: data.togetherSupported,
      togetherPromptDismissed: data.togetherPromptDismissed,
      binanceSupported: data.binanceSupported,
      geminiSupported: data.geminiSupported,
      bitcoinDotComSupported: data.bitcoinDotComSupported,
      rewardsState: data.rewardsState,
      binanceState: data.binanceState,
      geminiState: data.geminiState,
      cryptoDotComState: data.cryptoDotComState,
      ftxState: data.ftxState,
      removedStackWidgets: data.removedStackWidgets,
      widgetStackOrder: data.widgetStackOrder
    }
    window.localStorage.setItem(keyName, JSON.stringify(dataToSave))
  }
}, 50)
