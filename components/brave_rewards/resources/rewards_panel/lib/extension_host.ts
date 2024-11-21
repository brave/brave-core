/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_rewards/common/mojom/rewards_panel.mojom.m.js'

import { Host } from './interfaces'
import { OpenLinkAction } from '../../shared/components/notifications'
import { ExternalWalletAction } from '../../shared/components/wallet_card'
import { getInitialState } from './initial_state'
import { createStateManager } from '../../shared/lib/state_manager'
import { createLocalStorageScope } from '../../shared/lib/local_storage_scope'
import { RewardsPanelProxy } from './rewards_panel_proxy'

import * as apiAdapter from './extension_api_adapter'
import * as urls from '../../shared/lib/rewards_urls'

type LocalStorageKey = 'load-adaptive-captcha'

function getCurrentTabInfo () {
  interface TabInfo {
    id: number
  }

  return new Promise<TabInfo | null>((resolve) => {
    chrome.tabs.query({ active: true, currentWindow: true }, (tabs) => {
      if (tabs.length > 0) {
        const [tab] = tabs
        if (typeof tab.id === 'number') {
          resolve({ id: tab.id })
          return
        }
      }
      resolve(null)
    })
  })
}

function openTab (url: string) {
  if (!url) {
    console.error(new Error('Cannot open a tab with an empty URL'))
    return
  }
  chrome.tabs.create({ url })
}

export function createHost (): Host {
  const stateManager = createStateManager(getInitialState())
  const storage = createLocalStorageScope<LocalStorageKey>('rewards-panel')

  const proxy = RewardsPanelProxy.getInstance()

  function closePanel () {
    proxy.handler.closeUI()
  }

  async function updatePublisherInfo () {
    const tabInfo = await getCurrentTabInfo()
    if (tabInfo) {
      stateManager.update({
        publisherInfo: await apiAdapter.getPublisherInfo(tabInfo.id)
      })
    }
  }

  function clearAdaptiveCaptcha () {
    stateManager.update({ adaptiveCaptchaInfo: null })
    storage.writeJSON('load-adaptive-captcha', false)
  }

  function loadAdaptiveCaptcha () {
    chrome.braveRewards.getScheduledCaptchaInfo((scheduledCaptchaInfo) => {
      if (!scheduledCaptchaInfo.url) {
        clearAdaptiveCaptcha()
        return
      }
      stateManager.update({
        adaptiveCaptchaInfo: {
          url: scheduledCaptchaInfo.url,
          status: scheduledCaptchaInfo.maxAttemptsExceeded
            ? 'max-attempts-exceeded'
            : 'pending'
        }
      })

      // Store the adaptive captcha loading state so that if the user closes and
      // reopens the panel they can attempt the same captcha.
      storage.writeJSON('load-adaptive-captcha', true)
    })
  }

  function getExternalWalletActionURL (action: ExternalWalletAction) {
    const { externalWallet } = stateManager.getState()
    if (!externalWallet) {
      return urls.connectURL
    }

    switch (action) {
      case 'reconnect':
        return urls.reconnectURL
      case 'verify':
        return urls.connectURL
      case 'view-account':
        return externalWallet.url
    }
  }

  function handleExternalWalletAction (action: ExternalWalletAction) {
    const url = getExternalWalletActionURL(action)
    if (!url) {
      console.error(new Error(`Action URL does not exist for '${action}`))
      return
    }

    openTab(url)
  }

  function loadPersistedState () {
    const shouldLoadAdaptiveCaptcha = storage.readJSON('load-adaptive-captcha')
    if (shouldLoadAdaptiveCaptcha) {
      loadAdaptiveCaptcha()
      return true
    }

    return false
  }

  function handleRewardsPanelArgs (args: mojom.RewardsPanelArgs) {
    switch (args.view) {
      case mojom.RewardsPanelView.kRewardsSetup:
        stateManager.update({ requestedView: 'rewards-setup' })
        break
      case mojom.RewardsPanelView.kAdaptiveCaptcha:
        loadAdaptiveCaptcha()
        break
    }
  }

  function updateBalance () {
    apiAdapter.getRewardsBalance().then((balance) => {
      stateManager.update({ balance })
    }).catch(console.error)

    apiAdapter.getRewardsSummaryData().then((summaryData) => {
      stateManager.update({ summaryData })
    }).catch(console.error)
  }

  function updateUserType () {
    apiAdapter.getUserType().then((userType) => {
      stateManager.update({ userType })
    })
  }

  function updateNotifications () {
    apiAdapter.getNotifications().then((notifications) => {
      stateManager.update({ notifications })
    }).catch(console.error)
  }

  function setLoadingTimer () {
    // Set a maximum time to display the loading indicator. Several calls to
    // the `braveRewards` extension API can block on network requests. If the
    // network is unavailable or an endpoint is unresponsive, we want to display
    // the data that we have, rather than a stalled loading indicator.
    setTimeout(() => { stateManager.update({ loading: false }) }, 3000)
  }

  function startRevealTimer () {
    let called = false

    // When the panel is displayed using a cached `window`, we need to "reveal"
    // it by updating `openTime`. Ideally, we do not want to reveal it before
    // we've updated the UI, but if the browser is taking a long time to return
    // data we need to show the panel with a loading indicator.
    let revealTimeout = setTimeout(() => {
      called = true
      setLoadingTimer()
      stateManager.update({ openTime: Date.now(), loading: true })
    }, 750)

    return () => {
      if (!called) {
        clearTimeout(revealTimeout)
      }
    }
  }

  function addListeners () {
    // If a Rewards panel request occurs when we are still open or cached,
    // reload data and re-render the app.
    proxy.callbackRouter.onRewardsPanelRequested.addListener(
      (panelArgs: mojom.RewardsPanelArgs) => {
        let cancelRevealTimer = startRevealTimer()

        loadPanelData().then(() => {
          cancelRevealTimer()

          stateManager.update({
            openTime: Date.now(),
            requestedView: null,
            loading: false
          })

          handleRewardsPanelArgs(panelArgs)
        }).catch(console.error)
      })

    apiAdapter.onPublisherDataUpdated(() => {
      updatePublisherInfo().catch(console.error)
    })

    // Update user settings and other data after rewards has been enabled.
    chrome.braveRewards.onRewardsWalletCreated.addListener(() => {
      loadPanelData().catch(console.error)
    })

    // Update the balance when when tips have been processed, or when the user's
    // wallet is logged out.
    chrome.braveRewards.onReconcileComplete.addListener(updateBalance)
    chrome.braveRewards.onExternalWalletLoggedOut.addListener(updateBalance)

    // Update user type when the user's wallet is disconnected.
    chrome.braveRewards.onExternalWalletDisconnected.addListener(updateUserType)

    // Update the notification list when notifications are added or removed.
    chrome.rewardsNotifications.onAllNotificationsDeleted.addListener(
      updateNotifications)
    chrome.rewardsNotifications.onNotificationAdded.addListener(
      updateNotifications)
    chrome.rewardsNotifications.onNotificationDeleted.addListener(
      updateNotifications)
  }

  async function loadPanelData () {
    await Promise.all([
      apiAdapter.getRewardsEnabled().then((rewardsEnabled) => {
        stateManager.update({ rewardsEnabled })
      }),
      apiAdapter.getUserType().then((userType) => {
        stateManager.update({ userType })
      }),
      apiAdapter.getPublishersVisitedCount().then((publishersVisitedCount) => {
        stateManager.update({ publishersVisitedCount })
      }),
      apiAdapter.getDeclaredCountry().then((declaredCountry) => {
        stateManager.update({ declaredCountry })
      }),
      apiAdapter.getAvailableCountries().then((availableCountries) => {
        stateManager.update({ availableCountries })
      }),
      apiAdapter.getDefaultCountry().then((defaultCountry) => {
        stateManager.update({ defaultCountry })
      }),
      apiAdapter.getRewardsBalance().then((balance) => {
        stateManager.update({ balance })
      }),
      apiAdapter.getRewardsParameters().then((params) => {
        const { options, exchangeInfo, payoutStatus } = params
        stateManager.update({ options, exchangeInfo, payoutStatus })
      }),
      apiAdapter.getSelfCustodyInviteDismissed().then((dismissed) => {
        stateManager.update({ selfCustodyInviteDismissed: dismissed })
      }),
      apiAdapter.isTermsOfServiceUpdateRequired().then((updateRequired) => {
        stateManager.update({ isTermsOfServiceUpdateRequired: updateRequired })
      }),
      apiAdapter.getExternalWalletProviders().then((providers) => {
        stateManager.update({ externalWalletProviders: providers })
      }),
      apiAdapter.getExternalWallet().then((externalWallet) => {
        stateManager.update({ externalWallet })
      }),
      apiAdapter.getEarningsInfo().then((earningsInfo) => {
        if (earningsInfo) {
          stateManager.update({ earningsInfo })
        }
      }),
      apiAdapter.getRewardsSummaryData().then((summaryData) => {
        stateManager.update({ summaryData })
      }),
      updatePublisherInfo()
    ])

    updateNotifications()
  }

  async function initialize () {
    // Expose the state manager for debugging purposes.
    Object.assign(window, {
      braveRewardsPanel: { stateManager }
    })

    addListeners()
    setLoadingTimer()

    await loadPanelData()

    loadPersistedState()
    handleRewardsPanelArgs((await proxy.handler.getRewardsPanelArgs()).args)

    stateManager.update({ loading: false })
  }

  initialize().catch(console.error)

  return {

    get state () { return stateManager.getState() },

    addListener: stateManager.addListener,

    enableRewards (country: string) {
      return apiAdapter.createRewardsWallet(country)
    },

    openAdaptiveCaptchaSupport () {
      openTab('https://support.brave.com/')
    },

    openRewardsSettings () {
      openTab('chrome://rewards')
    },

    refreshPublisherStatus () {
      const { publisherInfo } = stateManager.getState()
      if (!publisherInfo) {
        return
      }

      stateManager.update({ publisherRefreshing: true })

      chrome.braveRewards.refreshPublisher(publisherInfo.id, () => {
        updatePublisherInfo().then(() => {
          stateManager.update({ publisherRefreshing: false })
        }).catch(console.error)
      })
    },

    sendTip () {
      getCurrentTabInfo().then((tabInfo) => {
        if (!tabInfo) {
          return
        }
        const { publisherInfo } = stateManager.getState()
        if (publisherInfo) {
          chrome.braveRewards.tipSite(tabInfo.id, publisherInfo.id, 'one-time')
          closePanel()
        }
      }).catch(console.error)
    },

    handleExternalWalletAction,

    handleNotificationAction (action) {
      switch (action.type) {
        case 'open-link':
          openTab((action as OpenLinkAction).url)
          break
        case 'reconnect-external-wallet':
          handleExternalWalletAction('reconnect')
          break
      }
    },

    dismissNotification (notification) {
      chrome.rewardsNotifications.deleteNotification(notification.id)
      const { notifications } = stateManager.getState()
      stateManager.update({
        notifications: notifications.filter(n => n.id !== notification.id)
      })
    },

    dismissSelfCustodyInvite () {
      chrome.braveRewards.dismissSelfCustodyInvite()
      stateManager.update({ selfCustodyInviteDismissed: true })
    },

    acceptTermsOfServiceUpdate () {
      chrome.braveRewards.acceptTermsOfServiceUpdate()
      stateManager.update({
        isTermsOfServiceUpdateRequired: false
      })
    },

    resetRewards () {
      openTab(urls.resetURL)
    },

    clearAdaptiveCaptcha,

    handleAdaptiveCaptchaResult (result) {
      const { adaptiveCaptchaInfo } = stateManager.getState()
      if (!adaptiveCaptchaInfo) {
        return
      }

      switch (result) {
        case 'success':
          chrome.braveRewards.updateScheduledCaptchaResult(true)
          stateManager.update({
            adaptiveCaptchaInfo: { ...adaptiveCaptchaInfo, status: 'success' }
          })
          break
        case 'failure':
        case 'error':
          chrome.braveRewards.updateScheduledCaptchaResult(false)
          loadAdaptiveCaptcha()
          break
      }
    },

    closePanel,

    onAppRendered () {
      proxy.handler.showUI()
    }
  }
}
