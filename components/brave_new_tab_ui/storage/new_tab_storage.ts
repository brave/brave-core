// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Utils
import { saveShowBinance, saveShowCryptoDotCom, saveShowFTX, saveShowGemini, saveWidgetVisibilityMigrated } from '../api/preferences'
import { debounce } from '../../common/debounce'
import { loadTimeData } from '../../common/loadTimeData'

export const keyName = 'new-tab-data'

export const defaultState: NewTab.State = {
  initialDataLoaded: false,
  textDirection: loadTimeData.getString('textdirection'),
  featureFlagBraveNTPSponsoredImagesWallpaper: loadTimeData.getBoolean('featureFlagBraveNTPSponsoredImagesWallpaper'),
  showBackgroundImage: false,
  showStats: false,
  showToday: false,
  showClock: false,
  clockFormat: '',
  showTopSites: false,
  customLinksEnabled: false,
  customLinksNum: 0,
  showRewards: false,
  showBraveTalk: false,
  showBinance: false,
  showGemini: false,
  showBitcoinDotCom: false,
  showCryptoDotCom: false,
  showFTX: false,
  widgetVisibilityMigrated: false,
  hideAllWidgets: false,
  brandedWallpaperOptIn: false,
  isBrandedWallpaperNotificationDismissed: true,
  isBraveTodayOptedIn: false,
  showEmptyPage: false,
  braveTalkSupported: false,
  geminiSupported: false,
  binanceSupported: false,
  bitcoinDotComSupported: false,
  cryptoDotComSupported: false,
  ftxSupported: false,
  isIncognito: chrome.extension.inIncognitoContext,
  useAlternativePrivateSearchEngine: false,
  showAlternativePrivateSearchEngineToggle: false,
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
  braveTalkPromptDismissed: false,
  braveTalkPromptAutoDismissed: false,
  braveTalkPromptAllowed: loadTimeData.getBoolean('braveTalkPromptAllowed'),
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
    rewardsEnabled: false,
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
  defaultState.isTor = loadTimeData.getBoolean('isTor')
  defaultState.isQwant = loadTimeData.getBoolean('isQwant')
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
    showBraveTalk,
    braveTalkSupported,
    binanceSupported
  } = state
  const displayLookup = {
    'rewards': {
      display: showRewards
    },
    'binance': {
      display: binanceSupported && showBinance
    },
    'braveTalk': {
      display: braveTalkSupported && showBraveTalk
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

export const updateWidgetVisibility = (state: NewTab.State, ftxUserAuthed: boolean) => {
  // Do visibility migration only once.
  if (state.widgetVisibilityMigrated) {
    return state
  }

  const {
    showRewards,
    showBraveTalk,
    braveTalkSupported,
    showBinance,
    binanceSupported,
    showCryptoDotCom,
    cryptoDotComSupported,
    showFTX,
    ftxSupported,
    showGemini,
    geminiSupported,
    binanceState,
    geminiState
  } = state

  const widgetLookupTable = {
    'braveTalk': {
      display: braveTalkSupported && showBraveTalk,
      isCrypto: false
    },
    'rewards': {
      display: showRewards,
      isCrypto: false
    },
    'binance': {
      display: binanceSupported && showBinance,
      isCrypto: true,
      userAuthed: binanceState.userAuthed
    },
    'cryptoDotCom': {
      display: cryptoDotComSupported && showCryptoDotCom,
      isCrypto: true,
      userAuthed: false
    },
    'ftx': {
      display: ftxSupported && showFTX,
      isCrypto: true,
      userAuthed: ftxUserAuthed
    },
    'gemini': {
      display: geminiSupported && showGemini,
      isCrypto: true,
      userAuthed: geminiState.userAuthed
    }
  }

  // Find crypto widget that is foremost and visible.
  let foremostVisibleCryptoWidget = ''
  const lastIndex = state.widgetStackOrder.length - 1
  for (let i = lastIndex; i >= 0; --i) {
    const widget = widgetLookupTable[state.widgetStackOrder[i]]
    if (!widget) {
      console.error('Update above lookup table')
      continue
    }

    if (!widget.display) {
      continue
    }

    if (widget.isCrypto) {
      foremostVisibleCryptoWidget = state.widgetStackOrder[i]
    }
    // Found visible foremost widget in the widget stack. Go out.
    break
  }

  const widgetsShowKey = {
    'binance': 'showBinance',
    'cryptoDotCom': 'showCryptoDotCom',
    'ftx': 'showFTX',
    'gemini': 'showGemini'
  }

  for (let key in widgetsShowKey) {
    // Show foremost visible crypto widget regardless of auth state
    // and show user authed crypto widget.
    if (key === foremostVisibleCryptoWidget ||
        widgetLookupTable[key].userAuthed) {
      state[widgetsShowKey[key]] = true
      continue
    }

    state[widgetsShowKey[key]] = false
  }

  saveShowBinance(state.showBinance)
  saveShowCryptoDotCom(state.showCryptoDotCom)
  saveShowFTX(state.showFTX)
  saveShowGemini(state.showGemini)
  saveWidgetVisibilityMigrated()

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
    // TODO(petemill): This should be of type NewTab.PersistantState, and first
    // fix errors related to properties which shouldn't be defined as persistant
    // (or are obsolete).
    const dataToSave = {
      braveTalkSupported: data.braveTalkSupported,
      braveTalkPromptDismissed: data.braveTalkPromptDismissed,
      braveTalkPromptAutoDismissed: data.braveTalkPromptAutoDismissed,
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
