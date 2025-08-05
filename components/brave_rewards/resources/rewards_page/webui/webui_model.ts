/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import { debounce } from '$web-common/debounce'

import {
  externalWalletFromExtensionData,
  isSelfCustodyProvider,
  ExternalWalletProvider,
  externalWalletProviderFromString
} from '../../shared/lib/external_wallet'

import { AppModel, defaultModel } from '../lib/app_model'
import { AppState, Notification, defaultState } from '../lib/app_state'
import { RewardsPageProxy } from './rewards_page_proxy'
import { createStateManager } from '../../shared/lib/state_manager'
import { createAdsHistoryAdapter } from './ads_history_adapter'
import { optional } from '../../shared/lib/optional'
import * as mojom from './mojom'

import {
  convertMojoTime,
  convertAdType,
  mapNotification,
  walletProvidersFromPublisherStatus
} from './mojom_helpers'

function normalizePlatform(name: string) {
  switch (name) {
    case 'android':
    case 'desktop':
      return name
  }
  console.error('Invalid platform name: ' + name)
  return 'desktop'
}

function parseCreatorPlatform(value: string) {
  switch (value) {
    case 'twitter':
    case 'youtube':
    case 'twitch':
    case 'reddit':
    case 'vimeo':
    case 'github':
      return value
  }
  return ''
}

function openTab(url: string) {
  window.open(url, '__blank', 'noopener noreferrer')
}

function createModelForUnsupportedRegion(): AppModel {
  const stateManager = createStateManager<AppState>(defaultState())
  stateManager.update({
    loading: false,
    isUnsupportedRegion: true
  })
  return {
    ...defaultModel(),
    getState: stateManager.getState,
    addListener: stateManager.addListener,
    openTab,
    getString(key) { return loadTimeData.getString(key) }
  }
}

export function createModel(): AppModel {
  if (loadTimeData.getBoolean('isUnsupportedRegion')) {
    return createModelForUnsupportedRegion()
  }

  const searchParams = new URLSearchParams(location.search)
  const browserProxy = RewardsPageProxy.getInstance()
  const pageHandler = browserProxy.handler
  const adsHistoryAdapter = createAdsHistoryAdapter()
  const stateManager = createStateManager<AppState>(defaultState())
  const platform = normalizePlatform(loadTimeData.getString('platform'))
  const creatorParam = searchParams.get('creator') ?? ''
  const isAutoResizeBubble = loadTimeData.getBoolean('isAutoResizeBubble')
  const isBubble = isAutoResizeBubble || searchParams.has('bubble')
  let lastPublisherRefresh = 0

  // Expose the state manager for devtools diagnostic purposes.
  Object.assign(self, {
    [Symbol.for('stateManager')]: stateManager
  })

  stateManager.update({
    embedder: {
      isBubble,
      isAutoResizeBubble,
      platform,
      animatedBackgroundEnabled:
        loadTimeData.getBoolean('animatedBackgroundEnabled')
    }
  })

  async function updatePaymentId() {
    const { paymentId } = await pageHandler.getRewardsPaymentId()
    stateManager.update({ paymentId })
  }

  async function updateCountryCode() {
    const { countryCode } = await pageHandler.getCountryCode()
    stateManager.update({ countryCode })
  }

  async function updateExternalWallet() {
    const { externalWallet } = await pageHandler.getExternalWallet()
    stateManager.update({
      externalWallet: externalWalletFromExtensionData(externalWallet)
    })
  }

  async function updateExternalWalletProviders() {
    const providers: ExternalWalletProvider[] = []
    const result = await pageHandler.getExternalWalletProviders()
    for (const key of result.providers) {
      const provider = externalWalletProviderFromString(key)
      if (provider) {
        providers.push(provider)
      }
    }
    stateManager.update({ externalWalletProviders: providers })
  }

  async function updateBalance() {
    const { balance } = await pageHandler.getAvailableBalance()
    stateManager.update({
      balance: typeof balance === 'number' ? optional(balance) : optional()
    })
  }

  async function updateTosUpdateRequired() {
    const { updateRequired } =
      await pageHandler.getTermsOfServiceUpdateRequired()
    stateManager.update({ tosUpdateRequired: updateRequired })
  }

  async function updateSelfCustodyProviderInvites() {
    const { providers } = await pageHandler.getSelfCustodyProviderInvites()

    const selfCustodyProviderInvites: ExternalWalletProvider[] = []
    for (const name of providers) {
      const provider = externalWalletProviderFromString(name)
      if (provider && isSelfCustodyProvider(provider)) {
        selfCustodyProviderInvites.push(provider)
      }
    }

    stateManager.update({ selfCustodyProviderInvites })
  }

  async function updateSelfCustodyInviteDismissed() {
    const { inviteDismissed } =
      await pageHandler.getSelfCustodyInviteDismissed()
    stateManager.update({ selfCustodyInviteDismissed: inviteDismissed })
  }

  // TODO(https://github.com/brave/brave-browser/issues/42702): Remove this
  // after a listener is added for Ads initialization, or after the Ads service
  // queues these calls during service restart.
  function scheduleAdsInfoUpdate() {
    setTimeout(() => { updateAdsInfo() }, 1000)
  }

  async function updateAdsInfo() {
    let [{ statement }, { settings }] = await Promise.all([
      await pageHandler.getAdsStatement(),
      await pageHandler.getAdsSettings()
    ])

    if (statement && settings) {
      const { adTypeSummaryThisMonth } = statement
      stateManager.update({
        adsInfo: {
          browserUpgradeRequired: settings.browserUpgradeRequired,
          isSupportedRegion: settings.isSupportedRegion,
          adsEnabled: {
            'new-tab-page': settings.newTabPageAdsEnabled,
            'notification': settings.notificationAdsEnabled,
            'search-result': settings.searchAdsEnabled
          },
          adTypesReceivedThisMonth: {
            'new-tab-page': adTypeSummaryThisMonth.newTabPageAds,
            'notification': adTypeSummaryThisMonth.notificationAds,
            'search-result': adTypeSummaryThisMonth.searchResultAds
          },
          minEarningsPreviousMonth: statement.minEarningsPreviousMonth,
          nextPaymentDate: convertMojoTime(statement.nextPaymentDate),
          notificationAdsPerHour: settings.notificationAdsPerHour,
          shouldAllowSubdivisionTargeting:
            settings.shouldAllowSubdivisionTargeting,
          currentSubdivision: settings.currentSubdivision,
          availableSubdivisions: settings.availableSubdivisions,
          autoDetectedSubdivision: settings.autoDetectedSubdivision
        }
      })
    } else {
      stateManager.update({ adsInfo: null })
    }
  }

  async function updateRewardsParameters() {
    const { rewardsParameters } = await pageHandler.getRewardsParameters()
    stateManager.update({ rewardsParameters })
  }

  async function updateRecurringContributions() {
    const { contributions } = await pageHandler.getRecurringContributions()
    stateManager.update({
      recurringContributions: contributions.map((item) => ({
        site: {
          id: item.id,
          icon: item.faviconUrl,
          name: item.name,
          url: item.url,
          platform: parseCreatorPlatform(item.provider)
        },
        amount: item.weight,
        nextContributionDate: Number(item.reconcileStamp) * 1000
      }))
    })
  }

  async function updateCurrentCreator() {
    let id = creatorParam
    if (!id) {
      id = (await pageHandler.getPublisherIdForActiveTab()).publisherId
    }

    if (Date.now() - lastPublisherRefresh > 1000 * 10) {
      await pageHandler.refreshPublisher(id)
      lastPublisherRefresh = Date.now()
    }

    const [{ publisherInfo }, { publisherBanner }] = await Promise.all([
      pageHandler.getPublisherInfo(id),
      pageHandler.getPublisherBanner(id)
    ])

    if (!publisherInfo) {
      stateManager.update({ currentCreator: null })
      return
    }

    stateManager.update({
      currentCreator: {
        site: {
          id: publisherInfo.id,
          icon: publisherInfo.faviconUrl,
          name: publisherInfo.name,
          url: publisherInfo.url,
          platform: parseCreatorPlatform(publisherInfo.provider)
        },
        banner: {
          title: publisherBanner?.title || '',
          description: publisherBanner?.description || '',
          background: publisherBanner?.background || '',
          web3URL: publisherBanner?.web3Url || ''
        },
        supportedWalletProviders:
          walletProvidersFromPublisherStatus(publisherInfo.status)
      }
    })
  }

  async function updateCaptchaInfo(opts: { pendingOnly: boolean }) {
    // Captchas can only be solved from the Rewards panel on desktop.
    if (!isBubble || platform !== 'desktop') {
      return
    }

    let { captchaInfo } = await pageHandler.getCaptchaInfo()
    if (opts.pendingOnly) {
      if (captchaInfo &&  captchaInfo.maxAttemptsExceeded) {
        captchaInfo = null
      }
    }

    stateManager.update({ captchaInfo })
  }

  async function updateNotifications() {
    const { notifications } = await pageHandler.getRewardsNotifications()
    const list: Notification[] = []
    for (const item of notifications) {
      const notification = mapNotification(item)
      if (notification) {
        list.push(notification)
      } else {
        await pageHandler.clearRewardsNotification(item.id)
      }
    }
    stateManager.update({ notifications: list })
  }

  async function updateCards() {
    const { cards } = await pageHandler.fetchUICards()
    stateManager.update({ cards: cards || null })
  }

  async function loadData() {
    // Discards the supplied promise so that the `Promise.all` below does not
    // block on the result. Any calls that may be blocked on a network request
    // should be wrapped with this decorator.
    const inBackground = (promise: Promise<unknown>) => null

    await Promise.all([
      updatePaymentId(),
      updateCountryCode(),
      updateExternalWallet(),
      updateExternalWalletProviders(),
      inBackground(updateBalance()),
      updateTosUpdateRequired(),
      updateSelfCustodyProviderInvites(),
      updateSelfCustodyInviteDismissed(),
      updateAdsInfo(),
      updateRecurringContributions(),
      updateRewardsParameters(),
      updateNotifications(),
      inBackground(updateCurrentCreator()),
      inBackground(updateCaptchaInfo({ pendingOnly: true })),
      inBackground(updateCards())
    ])

    stateManager.update({ loading: false })
  }

  browserProxy.callbackRouter.onRewardsStateUpdated.addListener(
    debounce(() => {
      if (document.visibilityState === 'visible') {
        loadData()
      }
    }, 60))

  // When displayed in a bubble, this page may be cached. In order to reset the
  // view state when the bubble is re-opened with cached contents, we update the
  // "openTime" state when the document visibility changes and reload data.
  if (isBubble) {
    document.addEventListener('visibilitychange', () => {
      if (document.visibilityState !== 'visible') {
        return
      }
      const now = Date.now()
      const { openTime } = stateManager.getState()
      if (now - openTime > 200) {
        stateManager.update({ openTime: now })
        loadData()
      }
    })
  }

  loadData()

  return {
    getState: stateManager.getState,

    addListener: stateManager.addListener,

    onAppRendered() {
      pageHandler.onPageReady()
      if (!isBubble) {
        pageHandler.notifyRewardsPageView()
      }
    },

    openTab(url) {
      if (isBubble) {
        pageHandler.openTab(url)
      } else {
        openTab(url)
      }
    },

    getString(key) {
      return loadTimeData.getString(key)
    },

    async getPluralString(key, count) {
      const { pluralString } = await pageHandler.getPluralString(key, count)
      return pluralString
    },

    async enableRewards(countryCode) {
      const { result } = await pageHandler.enableRewards(countryCode)
      scheduleAdsInfoUpdate()
      switch (result) {
        case mojom.CreateRewardsWalletResult.kSuccess:
          updatePaymentId()
          return 'success'
        case mojom.CreateRewardsWalletResult.kWalletGenerationDisabled:
          return 'wallet-generation-disabled'
        case mojom.CreateRewardsWalletResult.kGeoCountryAlreadyDeclared:
          return 'country-already-declared'
        case mojom.CreateRewardsWalletResult.kUnexpected:
          return 'unexpected-error'
        default:
          console.error('Unrecognized result value ' + result)
          return 'unexpected-error'
      }
    },

    async setWebDiscoveryProjectEnabled(enabled) {
      await pageHandler.setWebDiscoveryProjectEnabled(enabled)
    },

    async getAvailableCountries() {
      const { availableCountries } = await pageHandler.getAvailableCountries()
      return availableCountries
    },

    async beginExternalWalletLogin(provider) {
      const { params } = await pageHandler.beginExternalWalletLogin(provider)
      if (!params) {
        return false
      }
      if (isBubble) {
        pageHandler.openTab(params.url)
      } else {
        window.open(params.url, '_self', 'noreferrer')
      }
      return true
    },

    async connectExternalWallet(provider, args) {
      const ResultType = mojom.ConnectExternalWalletResult
      const { result } = await pageHandler.connectExternalWallet(provider, args)
      scheduleAdsInfoUpdate()
      switch (result) {
        case ResultType.kSuccess:
          return 'success'
        case ResultType.kDeviceLimitReached:
          return 'device-limit-reached'
        case ResultType.kFlaggedWallet:
          return 'flagged-wallet'
        case ResultType.kKYCRequired:
          return 'kyc-required'
        case ResultType.kMismatchedCountries:
          return 'mismatched-countries'
        case ResultType.kMismatchedProviderAccounts:
          return 'mismatched-provider-accounts'
        case ResultType.kProviderUnavailable:
          return 'provider-unavailable'
        case ResultType.kRegionNotSupported:
          return 'region-not-supported'
        case ResultType.kRequestSignatureVerificationFailure:
          return 'request-signature-verification-error'
        case ResultType.kUnexpected:
          return 'unexpected-error'
        case ResultType.kUpholdBATNotAllowed:
          return 'uphold-bat-not-allowed'
        case ResultType.kUpholdInsufficientCapabilities:
          return 'uphold-insufficient-capabilities'
        case ResultType.kUpholdTransactionVerificationFailure:
          return 'uphold-transaction-verification-failure'
        default:
          console.error('Unrecognized result value ' + result)
          return 'unexpected-error'
      }
    },

    async resetRewards() {
      await pageHandler.resetRewards()
    },

    async setAdTypeEnabled(adType, enabled) {
      // Before sending the update request to the browser, update the local app
      // state in order to avoid any jitter with nala toggles.
      const { adsInfo } = stateManager.getState()
      if (adsInfo) {
        adsInfo.adsEnabled[adType] = enabled
        stateManager.update({ adsInfo })
      }
      await pageHandler.setAdTypeEnabled(convertAdType(adType), enabled);
    },

    async setNotificationAdsPerHour(adsPerHour) {
      await pageHandler.setNotificationAdsPerHour(adsPerHour);
    },

    async setAdsSubdivision(subdivision) {
      await pageHandler.setAdsSubdivision(subdivision);
    },

    async getAdsHistory() {
      const { history } = await pageHandler.getAdsHistory()
      adsHistoryAdapter.parseData(history)
      return adsHistoryAdapter.getItems()
    },

    async setAdLikeStatus(id, status) {
      const detail = adsHistoryAdapter.getRawDetail(id)
      const previous = adsHistoryAdapter.setAdLikeStatus(id, status)
      switch (status || previous) {
        case 'liked':
          await pageHandler.toggleAdLike(detail)
          break
        case 'disliked':
          await pageHandler.toggleAdDislike(detail)
          break
      }
    },

    async setAdInappropriate(id, value) {
      const detail = adsHistoryAdapter.getRawDetail(id)
      adsHistoryAdapter.setInappropriate(id, value)
      await pageHandler.toggleAdInappropriate(detail)
    },

    async removeRecurringContribution(id) {
      await pageHandler.removeRecurringContribution(id)
      updateRecurringContributions()
    },

    async sendContribution(creatorID, amount, recurring) {
      const { contributionSent } =
        await pageHandler.sendContribution(creatorID, amount, recurring)
      return contributionSent
    },

    async acceptTermsOfServiceUpdate() {
      await pageHandler.acceptTermsOfServiceUpdate()
      stateManager.update({ tosUpdateRequired: false })
    },

    async dismissSelfCustodyInvite() {
      await pageHandler.dismissSelfCustodyInvite()
      stateManager.update({ selfCustodyInviteDismissed: true })
    },

    async onCaptchaResult(success) {
      await pageHandler.onCaptchaResult(success)
      if (!success) {
        await updateCaptchaInfo({ pendingOnly: false })
      }
    },

    async clearNotification(id: string) {
      stateManager.update({
        notifications: stateManager.getState().notifications.filter((item) => {
          return item.id !== id
        })
      })
      await pageHandler.clearRewardsNotification(id)
    }
  }
}
