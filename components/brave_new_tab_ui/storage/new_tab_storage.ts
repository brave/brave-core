// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Utils
import { debounce } from '../../common/debounce'
import { loadTimeData } from '../../common/loadTimeData'
import { braveSearchHost } from '../components/search/config'

export const keyName = 'new-tab-data'

export const defaultState: NewTab.State = {
  initialDataLoaded: false,
  textDirection: loadTimeData.getString('textdirection'),
  featureFlagBraveNTPSponsoredImagesWallpaper: loadTimeData.getBoolean('featureFlagBraveNTPSponsoredImagesWallpaper'),
  featureFlagBraveNewsPromptEnabled: loadTimeData.getBoolean('featureFlagBraveNewsPromptEnabled'),
  featureFlagBraveNewsFeedV2Enabled: loadTimeData.getBoolean('featureFlagBraveNewsFeedV2Enabled'),
  featureCustomBackgroundEnabled: loadTimeData.getBoolean('featureCustomBackgroundEnabled'),
  searchPromotionEnabled: false,
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
  showBraveVPN: false,
  showSearchBox: true,
  lastUsedNtpSearchEngine: braveSearchHost,
  promptEnableSearchSuggestions: true,
  searchSuggestionsEnabled: false,
  showBitcoinDotCom: false,
  hideAllWidgets: false,
  brandedWallpaperOptIn: false,
  isBrandedWallpaperNotificationDismissed: true,
  isBraveNewsOptedIn: false,
  showEmptyPage: false,
  braveRewardsSupported: false,
  braveTalkSupported: false,
  braveVPNSupported: false,
  bitcoinDotComSupported: false,
  isIncognito: chrome.extension.inIncognitoContext,
  useAlternativePrivateSearchEngine: false,
  showAlternativePrivateSearchEngineToggle: false,
  torCircuitEstablished: false,
  torInitProgress: '',
  isTor: false,
  stats: {
    adsBlockedStat: 0,
    javascriptBlockedStat: 0,
    bandwidthSavedStat: 0,
    httpsUpgradesStat: 0,
    fingerprintingBlockedStat: 0
  },
  rewardsState: {
    adsAccountStatement: {
      nextPaymentDate: 0,
      adsReceivedThisMonth: 0,
      minEarningsThisMonth: 0,
      maxEarningsThisMonth: 0,
      minEarningsLastMonth: 0,
      maxEarningsLastMonth: 0
    },
    balance: undefined,
    externalWallet: undefined,
    externalWalletProviders: [],
    dismissedNotifications: [],
    rewardsEnabled: false,
    userType: '',
    declaredCountry: '',
    needsBrowserUpgradeToServeAds: false,
    totalContribution: 0.0,
    publishersVisitedCount: 0,
    selfCustodyInviteDismissed: false,
    isTermsOfServiceUpdateRequired: false,
    parameters: {
      rate: 0,
      monthlyTipChoices: [],
      payoutStatus: {},
      walletProviderRegions: {},
      vbatDeadline: undefined,
      vbatExpired: false
    }
  },
  currentStackWidget: '',
  removedStackWidgets: [],
  // Order is ascending, with last entry being in the foreground
  widgetStackOrder: ['rewards', 'braveVPN'],
  customImageBackgrounds: []
}

if (chrome.extension.inIncognitoContext) {
  defaultState.isTor = loadTimeData.getBoolean('isTor')
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
    showRewards,
    showBraveTalk,
    braveRewardsSupported,
    braveTalkSupported
  } = state
  const displayLookup: { [p: string]: { display: boolean } } = {
    'rewards': {
      display: braveRewardsSupported && showRewards
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

const cleanData = (state: NewTab.State) => {
  const { rewardsState } = state

  if (typeof (rewardsState.totalContribution as any) === 'string') {
    rewardsState.totalContribution = 0
  }

  const { adsAccountStatement } = rewardsState
  if (adsAccountStatement) {
    // nextPaymentDate updated from seconds-since-epoch-string to ms-since-epoch
    if (typeof (adsAccountStatement.nextPaymentDate as any) === 'string') {
      adsAccountStatement.nextPaymentDate =
        Number(adsAccountStatement.nextPaymentDate) * 1000 || 0
    }
    // earningsThisMonth replaced with range
    if (typeof (adsAccountStatement.minEarningsThisMonth as any) !== 'number') {
      adsAccountStatement.minEarningsThisMonth = 0
      adsAccountStatement.maxEarningsThisMonth = 0
    }
    // earningsLastMonth replaced with range
    if (typeof (adsAccountStatement.minEarningsLastMonth as any) !== 'number') {
      adsAccountStatement.minEarningsLastMonth = 0
      adsAccountStatement.maxEarningsLastMonth = 0
    }
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
      braveRewardsSupported: data.braveRewardsSupported,
      braveTalkSupported: data.braveTalkSupported,
      bitcoinDotComSupported: data.bitcoinDotComSupported,
      rewardsState: data.rewardsState,
      removedStackWidgets: data.removedStackWidgets,
      widgetStackOrder: data.widgetStackOrder
    }
    window.localStorage.setItem(keyName, JSON.stringify(dataToSave))
  }
}, 50)
