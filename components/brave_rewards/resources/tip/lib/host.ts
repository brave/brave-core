/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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
    self.cr.addWebUIListener(name, listener)
  }
}

export function createHost (): Host {
  const stateManager = createStateManager<HostState>({})
  const dialogArgs = getDialogArgs()

  addWebUIListeners({

    rewardsInitialized () {
      const { publisherKey } = dialogArgs

      if (!publisherKey) {
        stateManager.update({
          hostError: { type: 'INVALID_DIALOG_ARGS' }
        })
        return
      }

      chrome.send('getRewardsParameters')
      chrome.send('fetchBalance')
      chrome.send('getReconcileStamp')
      chrome.send('getExternalWallet', ['uphold'])
      chrome.send('getOnlyAnonWallet')
      chrome.send('getRecurringTips')
      chrome.send('getPublisherBanner', [publisherKey])
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
            type: 'ERROR_FETCHING_BALANCE',
            code: result.status
          }
        })
      }
    },

    onlyAnonWalletUpdated (onlyAnonWallet: boolean) {
      stateManager.update({ onlyAnonWallet })
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

  self.i18nTemplate.process(document, self.loadTimeData)

  chrome.send('dialogReady')

  return {

    get state () {
      return stateManager.getState()
    },

    getString (key: string) {
      return self.loadTimeData.getString(key)
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
          hostError: { type: 'INVALID_PUBLISHER_KEY' }
        })
        return
      }

      if (amount < 0 || kind === 'one-time' && amount === 0) {
        stateManager.update({
          hostError: { type: 'INVALID_TIP_AMOUNT' }
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
          hostError: { type: 'PUBLISHER_INFO_UNAVAILABLE' }
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
