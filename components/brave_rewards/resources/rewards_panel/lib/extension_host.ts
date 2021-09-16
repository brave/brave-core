/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Host, GrantInfo, GrantCaptchaStatus } from './interfaces'
import { OpenLinkAction, ClaimGrantAction } from '../../shared/components/notifications'
import { ExternalWalletAction } from '../../shared/components/wallet_card'
import { getInitialState } from './initial_state'
import { createStateManager } from '../../shared/lib/state_manager'
import { createLocalStorageScope } from '../../shared/lib/local_storage_scope'
import * as apiAdapter from './extension_api_adapter'

type LocaleStorageKey = 'catcha-grant-id'

function closePanel () {
  window.close()
}

function openTab (url: string) {
  if (!url) {
    console.error(new Error('Cannot open a tab with an empty URL'))
    return
  }
  chrome.tabs.create({ url }, () => { closePanel() })
}

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

export function createHost (): Host {
  const stateManager = createStateManager(getInitialState())
  const storage = createLocalStorageScope<LocaleStorageKey>('rewards-panel')
  const grants = new Map<string, GrantInfo>()

  async function updatePublisherInfo () {
    const tabInfo = await getCurrentTabInfo()
    if (tabInfo) {
      const publisherInfo = await apiAdapter.getPublisherInfo(tabInfo.id)
      if (publisherInfo) {
        stateManager.update({ publisherInfo })
      }
    }
  }

  function clearGrantCaptcha () {
    stateManager.update({ grantCaptchaInfo: null })
    storage.writeJSON('catcha-grant-id', '')
  }

  function loadCaptcha (grantId: string, status: GrantCaptchaStatus) {
    const grantInfo = grants.get(grantId)
    if (!grantInfo) {
      clearGrantCaptcha()
      return
    }

    stateManager.update({
      grantCaptchaInfo: {
        id: '',
        hint: '',
        imageURL: '',
        status,
        grantInfo
      }
    })

    // Store the grant ID so that if the user closes and reopens the panel they
    // can attempt the same captcha.
    storage.writeJSON('catcha-grant-id', grantId)

    chrome.braveRewards.claimPromotion(grantId, (properties) => {
      stateManager.update({
        grantCaptchaInfo: {
          id: properties.captchaId,
          hint: properties.hint,
          imageURL: properties.captchaImage,
          status,
          grantInfo
        }
      })
    })
  }

  function handleExternalWalletAction (action: ExternalWalletAction) {
    const { externalWallet } = stateManager.getState()

    if (action === 'disconnect') {
      if (externalWallet) {
        chrome.braveRewards.disconnectWallet()
        stateManager.update({ externalWallet: null })
      }
      return
    }

    Promise.resolve().then(() => {
      const verifyURL = 'chrome://rewards#verify'
      if (!externalWallet) {
        return verifyURL
      }

      const { links } = externalWallet
      switch (action) {
        case 'add-funds':
          return links.addFunds || links.account
        case 'complete-verification':
          return links.completeVerification || links.account
        case 'reconnect':
          return apiAdapter.getExternalWalletLoginURL(externalWallet.provider)
        case 'verify':
          return verifyURL
        case 'view-account':
          return links.account
      }
    }).then(openTab, console.error)
  }

  function handleStartupParameters () {
    const { hash } = location

    const grantMatch = hash.match(/^#?grant_([\s\S]+)$/i)
    if (grantMatch) {
      location.hash = ''
      loadCaptcha(grantMatch[1], 'pending')
      return
    }

    const grantId = storage.readJSON('catcha-grant-id')
    if (grantId && typeof grantId === 'string') {
      location.hash = ''
      loadCaptcha(grantId, 'pending')
      return
    }
  }

  function updateGrants (list: GrantInfo[]) {
    grants.clear()
    for (const grant of list) {
      grants.set(grant.id, grant)
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

  function updateNotifications () {
    apiAdapter.getNotifications().then((notifications) => {
      stateManager.update({ notifications })
    }).catch(console.error)
  }

  async function requestPublisherInfo () {
    const tabInfo = await getCurrentTabInfo()
    if (tabInfo) {
      await apiAdapter.fetchPublisherInfo(tabInfo.id)
    }
  }

  function addListeners () {
    chrome.tabs.onUpdated.addListener((tabId, changeInfo, tab) => {
      if (tab.active && changeInfo.status === 'complete') {
        requestPublisherInfo().catch(console.error)
      }
    })

    apiAdapter.onPublisherDataUpdated(() => {
      updatePublisherInfo().catch(console.error)
    })

    // Update user settings after rewards has been enabled.
    apiAdapter.onRewardsEnabled(() => {
      apiAdapter.getSettings().then((settings) => {
        stateManager.update({ settings })
      }).catch(console.error)
    })

    apiAdapter.onGrantsUpdated(updateGrants)

    // Update the balance when a grant has been processed, when tips have been
    // processed, or when the user disconnects their wallet.
    chrome.braveRewards.onReconcileComplete.addListener(updateBalance)
    chrome.braveRewards.onUnblindedTokensReady.addListener(updateBalance)
    chrome.braveRewards.onDisconnectWallet.addListener(updateBalance)

    // Update the notification list when notifications are added or removed.
    chrome.rewardsNotifications.onAllNotificationsDeleted.addListener(
      updateNotifications)
    chrome.rewardsNotifications.onNotificationAdded.addListener(
      updateNotifications)
    chrome.rewardsNotifications.onNotificationDeleted.addListener(
      updateNotifications)
  }

  async function initialize () {
    // Expose the state manager for debugging purposes
    Object.assign(window, {
      braveRewardsPanel: { stateManager }
    })

    addListeners()

    await Promise.all([
      apiAdapter.getGrants().then((list) => {
        updateGrants(list)
      }),
      apiAdapter.getRewardsEnabled().then((rewardsEnabled) => {
        stateManager.update({ rewardsEnabled })
      }),
      apiAdapter.getRewardsBalance().then((balance) => {
        stateManager.update({ balance })
      }),
      apiAdapter.getRewardsParameters().then((params) => {
        const { options, exchangeInfo } = params
        stateManager.update({ options, exchangeInfo })
      }),
      apiAdapter.getSettings().then((settings) => {
        stateManager.update({ settings })
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
      apiAdapter.getNotifications().then((notifications) => {
        stateManager.update({ notifications })
      }),
      updatePublisherInfo()
    ])

    requestPublisherInfo().catch(console.error)

    handleStartupParameters()

    stateManager.update({ loading: false })
  }

  initialize().catch(console.error)

  return {

    get state () { return stateManager.getState() },

    addListener: stateManager.addListener,

    getString (key) {
      // In order to normalize messages across extensions and WebUI, replace all
      // chrome.i18n message placeholders with $N marker patterns. UI components
      // are responsible for replacing these markers with appropriate text or
      // using the markers to build HTML.
      return chrome.i18n.getMessage(key,
        ['$1', '$2', '$3', '$4', '$5', '$6', '$7', '$8', '$9'])
    },

    enableRewards () {
      chrome.braveRewards.enableRewards()
      stateManager.update({ rewardsEnabled: true })
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
        requestPublisherInfo().then(() => {
          stateManager.update({ publisherRefreshing: false })
        }).catch(console.error)
      })
    },

    setIncludeInAutoContribute (include) {
      const { publisherInfo } = stateManager.getState()
      if (!publisherInfo) {
        return
      }

      // Confusingly, |includeInAutoContribution| takes an "exclude" parameter.
      chrome.braveRewards.includeInAutoContribution(publisherInfo.id, !include)

      stateManager.update({
        publisherInfo: {
          ...publisherInfo,
          autoContributeEnabled: include
        }
      })
    },

    setAutoContributeAmount (amount) {
      chrome.braveRewards.updatePrefs({ autoContributeAmount: amount })

      stateManager.update({
        settings: {
          ...stateManager.getState().settings,
          autoContributeAmount: amount
        }
      })
    },

    setAdsPerHour (adsPerHour) {
      chrome.braveRewards.updatePrefs({ adsPerHour })

      stateManager.update({
        settings: {
          ...stateManager.getState().settings,
          adsPerHour
        }
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

    handleMonthlyTipAction (action) {
      getCurrentTabInfo().then((tabInfo) => {
        if (!tabInfo) {
          return
        }

        const { publisherInfo } = stateManager.getState()
        if (!publisherInfo) {
          return
        }

        const tabId = tabInfo.id
        const publisherId = publisherInfo.id

        switch (action) {
          case 'update':
            chrome.braveRewards.tipSite(tabId, publisherId, 'set-monthly')
            break
          case 'cancel':
            chrome.braveRewards.tipSite(tabId, publisherId, 'clear-monthly')
            break
        }

        closePanel()

      }).catch(console.error)
    },

    handleExternalWalletAction,

    handleNotificationAction (action) {
      switch (action.type) {
        case 'open-link':
          openTab((action as OpenLinkAction).url)
          break
        case 'backup-wallet':
          openTab('chrome://rewards#manage-wallet')
          break
        case 'claim-grant':
          loadCaptcha((action as ClaimGrantAction).grantId, 'pending')
          break
        case 'add-funds':
          handleExternalWalletAction('add-funds')
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

    solveGrantCaptcha (solution) {
      const { grantCaptchaInfo } = stateManager.getState()
      if (!grantCaptchaInfo) {
        return
      }

      function mapResult (result: number): GrantCaptchaStatus {
        switch (result) {
          case 0: return 'passed'
          case 6: return 'failed'
          default: return 'error'
        }
      }

      const json = JSON.stringify({
        captchaId: grantCaptchaInfo.id,
        x: Math.round(solution.x),
        y: Math.round(solution.y)
      })

      const grantId = grantCaptchaInfo.grantInfo.id

      chrome.braveRewards.attestPromotion(grantId, json, (result) => {
        const { grantCaptchaInfo } = stateManager.getState()
        if (!grantCaptchaInfo || grantCaptchaInfo.grantInfo.id !== grantId) {
          return
        }

        const status = mapResult(result)

        stateManager.update({
          grantCaptchaInfo: { ...grantCaptchaInfo, status }
        })

        // If the user failed the captcha, request a new captcha for the user.
        if (status === 'failed') {
          loadCaptcha(grantId, 'failed')
        }
      })
    },

    clearGrantCaptcha
  }
}
