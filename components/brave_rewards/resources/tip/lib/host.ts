/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { addWebUIListener } from 'chrome://resources/js/cr.m'
import { loadTimeData } from '../../../../common/loadTimeData'

import { createStateManager } from '../../shared/lib/state_manager'

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
  ShareTarget
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
  let tipResultPending = false

  function checkPendingTipResult (result: number) {
    if (tipResultPending) {
      tipResultPending = false
      if (result === 0) {
        stateManager.update({ tipProcessed: true })
      } else {
        stateManager.update({ hostError: { type: 'ERR_TIP_FAILED' } })
      }
      return true
    }
    return false
  }

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
      chrome.send('getExternalWallet')
      chrome.send('getRecurringTips')
      chrome.send('getPublisherBanner', [publisherKey])
    },

    externalWalletUpdated (externalWalletInfo: ExternalWalletInfo) {
      stateManager.update({ externalWalletInfo })
    },

    tipProcessed (amount: number) {
      if (!tipResultPending) {
        stateManager.update({ tipProcessed: true })
      }
    },

    tipFailed () {
      tipResultPending = false
      stateManager.update({
        hostError: { type: 'ERR_TIP_FAILED' }
      })
    },

    publisherBannerUpdated (publisherInfo: PublisherInfo) {
      stateManager.update({ publisherInfo })
    },

    rewardsParametersUpdated (rewardsParameters: RewardsParameters) {
      stateManager.update({ rewardsParameters })
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
      if (data.type === 8) { // RewardsType::ONE_TIME_TIP
        checkPendingTipResult(data.result)
      }
    },

    pendingContributionSaved (result: number) {
      if (checkPendingTipResult(result)) {
        stateManager.update({ tipPending: true })
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

      stateManager.update({ tipAmount: amount })

      chrome.send('onTip', [
        dialogArgs.publisherKey,
        amount,
        kind === 'monthly'
      ])

      // For one-time tips, we need to listen for additional events to know
      // whether the tip was successful because the `tipProcessed` event may
      // only indicate that the tip was added to the contribution queue.
      tipResultPending = kind === 'one-time'

      // If we are waiting for confirmation that a one-time tip was successful
      // and we don't receive an indication of success or failure within a
      // reasonable amount of time, then assume that the tip is now sitting
      // within the contribution queue and will be processed shortly. Display a
      // success message to the user.
      if (tipResultPending) {
        setTimeout(() => {
          tipResultPending = false
          stateManager.update({ tipProcessed: true })
        }, 3000)
      }
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

      chrome.send('tweetTip', [name, postId, publisherInfo.status])
      chrome.send('dialogClose')
    },

    addListener: stateManager.addListener

  }
}
