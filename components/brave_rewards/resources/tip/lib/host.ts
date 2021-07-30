/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { addWebUIListener } from '../../../../common/cr'
import { loadTimeData } from '../../../../common/loadTimeData'
import { OnboardingCompletedStore } from '../../shared/lib/onboarding_completed_store'

import { createStateManager } from './state_manager'

import {
  Host,
  HostState,
  PublisherInfo,
  RewardsParameters,
  BalanceInfo,
  ExternalWalletInfo,
  DialogArgs,
  EntryPoint,
  TipKind,
  ShareTarget,
  OnboardingResult
} from './interfaces'

interface RecurringTipInfo {
  publisherKey: string
  amount: number
}

function getEntryPoint (value: string): EntryPoint {
  switch (value) {
    case 'set-monthly': return value
    case 'clear-monthly': return value
    default: return 'one-time'
  }
}

function getDialogArgs (): DialogArgs {
  let args: any = {}
  try {
    args = Object(JSON.parse(chrome.getVariableValue('dialogArguments')))
  } catch (error) {
    console.error(error)
  }

  return {
    url: String(args.url || ''),
    publisherKey: String(args.publisherKey || ''),
    entryPoint: getEntryPoint(String(args.entryPoint)),
    mediaMetaData: args.mediaMetaData
      ? Object(args.mediaMetaData)
      : { mediaType: 'none' }
  }
}

function addWebUIListeners (listeners: Record<string, any>) {
  for (const [name, listener] of Object.entries(listeners)) {
    addWebUIListener(name, listener)
  }
}

export function createHost (): Host {
  const stateManager = createStateManager<HostState>({})
  const dialogArgs = getDialogArgs()
  const onboardingCompleted = new OnboardingCompletedStore()

  addWebUIListeners({

    rewardsInitialized () {
      const { publisherKey } = dialogArgs

      if (!publisherKey) {
        stateManager.update({
          hostError: { type: 'ERR_INVALID_DIALOG_ARGS' }
        })
        return
      }

      chrome.send('getRewardsParameters')
      chrome.send('fetchBalance')
      chrome.send('getReconcileStamp')
      chrome.send('getAutoContributeAmount')
      chrome.send('getAdsPerHour')
      chrome.send('getExternalWallet')
      chrome.send('getRecurringTips')
      chrome.send('getPublisherBanner', [publisherKey])
      chrome.send('getOnboardingStatus')
    },

    externalWalletUpdated (externalWalletInfo: ExternalWalletInfo) {
      stateManager.update({ externalWalletInfo })
    },

    publisherBannerUpdated (publisherInfo: PublisherInfo) {
      stateManager.update({ publisherInfo })
    },

    rewardsParametersUpdated (rewardsParameters: RewardsParameters) {
      stateManager.update({ rewardsParameters })
    },

    onboardingStatusUpdated (result: { showOnboarding: boolean }) {
      stateManager.update({
        showOnboarding: result.showOnboarding && !onboardingCompleted.load()
      })
    },

    recurringTipsUpdated (tips?: RecurringTipInfo[]) {
      if (!tips) {
        return
      }
      for (const tip of tips) {
        if (tip.publisherKey === dialogArgs.publisherKey) {
          stateManager.update({
            currentMonthlyTip: Number(tip.amount) || 0
          })
          break
        }
      }
    },

    reconcileStampUpdated (stamp: number) {
      stateManager.update({ nextReconcileDate: new Date(stamp * 1000) })
    },

    autoContributeAmountUpdated (autoContributeAmount: number) {
      stateManager.update({ autoContributeAmount })
    },

    adsPerHourUpdated (adsPerHour: number) {
      stateManager.update({ adsPerHour })
    },

    balanceUpdated (result: { status: number, balance: BalanceInfo }) {
      if (result.status === 0) {
        stateManager.update({
          balanceInfo: result.balance
        })
      } else {
        stateManager.update({
          hostError: {
            type: 'ERR_FETCH_BALANCE',
            code: result.status
          }
        })
      }
    },

    recurringTipRemoved (success: boolean) {
      chrome.send('getRecurringTips')
    },

    recurringTipSaved (success: boolean) {
      chrome.send('getRecurringTips')
    },

    reconcileCompleted (data: { type: number, result: number }) {
      if (data.result === 0) {
        chrome.send('fetchBalance')
      }
    },

    unblindedTokensReady () {
      chrome.send('fetchBalance')
    }

  })

  // Expose a symbol-keyed method for testing the display of
  // error messaging which may be difficult to reproduce.
  self[Symbol.for('setTipDialogErrorForTesting')] =
    (type: unknown, code: unknown) => {
      stateManager.update({
        hostError: {
          type: String(type),
          code: code === undefined ? code : Number(code)
        }
      })
    }

  chrome.send('dialogReady')

  return {

    get state () {
      return stateManager.getState()
    },

    getString (key: string) {
      return loadTimeData.getString(key)
    },

    getDialogArgs () {
      return dialogArgs
    },

    closeDialog () {
      chrome.send('dialogClose')
    },

    setAutoContributeAmount (autoContributeAmount: number) {
      chrome.send('setAutoContributeAmount', [autoContributeAmount])
      stateManager.update({ autoContributeAmount })
    },

    setAdsPerHour (adsPerHour: number) {
      chrome.send('setAdsPerHour', [adsPerHour])
      stateManager.update({ adsPerHour })
    },

    saveOnboardingResult (result: OnboardingResult) {
      chrome.send('saveOnboardingResult', [result])
      stateManager.update({ showOnboarding: false })
      onboardingCompleted.save()
    },

    processTip (amount: number, kind: TipKind) {
      if (!dialogArgs.publisherKey) {
        stateManager.update({
          hostError: { type: 'ERR_INVALID_PUBLISHER_KEY' }
        })
        return
      }

      if (amount < 0 || kind === 'one-time' && amount === 0) {
        stateManager.update({
          hostError: { type: 'ERR_INVALID_TIP_AMOUNT' }
        })
        return
      }

      chrome.send('onTip', [
        dialogArgs.publisherKey,
        amount,
        kind === 'monthly' ? true : false
      ])

      stateManager.update({
        tipProcessed: true,
        tipAmount: amount
      })
    },

    shareTip (target: ShareTarget) {
      const { publisherInfo } = stateManager.getState()
      if (!publisherInfo) {
        stateManager.update({
          hostError: { type: 'ERR_PUBLISHER_INFO_UNAVAILABLE' }
        })
        return
      }

      let name = publisherInfo.name
      let postId = ''

      if (dialogArgs.mediaMetaData.mediaType === 'twitter') {
        name = '@' + dialogArgs.mediaMetaData.publisherName
        postId = dialogArgs.mediaMetaData.postId
      } else if (publisherInfo.provider === 'twitter' && dialogArgs.url) {
        name = '@' + dialogArgs.url.replace(/^.*\//, '')
      }

      chrome.send('tweetTip', [name, postId])
      chrome.send('dialogClose')
    },

    addListener: stateManager.addListener

  }
}
