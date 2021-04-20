/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { DialogCloseReason, OrderInfo, Host, HostListener } from './interfaces'

function parseOrderInfo (): OrderInfo {
  const argString = chrome.getVariableValue('dialogArguments')

  let args: any
  args = JSON.parse(argString)

  const { orderInfo } = Object(args)

  return {
    total: orderInfo.total
  }
}

interface BalanceDetails {
  total: number
}

interface RateDetails {
  rate: number
  lastUpdated: string
}

interface ExternalWalletDetails {
  status: number
}

export function createHost (): Host {

  let hostListener: HostListener | null = null
  let balanceDetails: BalanceDetails | null = null
  let rateDetails: RateDetails | null = null
  let externalWalletDetails: ExternalWalletDetails | null = null

  let orderInfo = parseOrderInfo()

  self.i18nTemplate.process(document, self.loadTimeData)

  function sendWalletInfo () {
    if (externalWalletDetails && hostListener) {
      const balance = balanceDetails ? balanceDetails.total : 0
      const verified = externalWalletDetails.status === 2
      hostListener.onWalletUpdated({ balance, verified })
    }
  }

  function sendRateInfo () {
    if (hostListener && rateDetails) {
      const rate = rateDetails.rate
      const lastUpdated = new Date(Number(rateDetails.lastUpdated)).toISOString()
      hostListener.onRatesUpdated({ rate, lastUpdated })
    }
  }

  self.cr.addWebUIListener('walletBalanceUpdated', (event: any) => {
    if (!event) {
      return
    }

    balanceDetails = event as BalanceDetails
    sendWalletInfo()
  })

  self.cr.addWebUIListener('rewardsParametersUpdated', (event: any) => {
    if (!event) {
      return
    }
    rateDetails = event as RateDetails
    sendRateInfo()
  })

  self.cr.addWebUIListener('externalWalletUpdated', (event: any) => {
    if (!event) {
      return
    }
    externalWalletDetails = event as ExternalWalletDetails
    sendWalletInfo()
  })

  return {
    getLocaleString (key: string) {
      return self.loadTimeData.getString(key)
    },

    closeDialog (reason: DialogCloseReason) {
      chrome.send('dialogClose', [reason.toString()])
    },

    payWithWallet (...args) {
      chrome.send('paymentRequestComplete')
    },

    setListener (listener) {
      hostListener = listener

      chrome.send('getWalletBalance')
      chrome.send('getRewardsParameters')
      chrome.send('getExternalWallet')

      queueMicrotask(() => {
        if (hostListener) {
          hostListener.onOrderUpdated(orderInfo)
        }
      })

      return () => { hostListener = null }
    }

  }
}
