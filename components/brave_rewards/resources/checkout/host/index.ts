/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { OrderInfo, CheckoutHost, CheckoutHostListener } from '../interfaces'

function parseDialogArgs (): OrderInfo {
  const argString = chrome.getVariableValue('dialogArguments')

  let args: any
  try {
    args = JSON.parse(argString)
  } catch {
    // Ignore
  }

  const { orderInfo } = Object(args)
  // TODO(zenparsing): Throw if orderInfo is invalid?

  return {
    description: orderInfo.description,
    total: orderInfo.total
  }
}

interface BalanceDetails {
  total: number
  rates: Record<string, number>
}

interface ExternalWalletDetails {
  status: number
}

export function createHost (): CheckoutHost {
  let hostListener: CheckoutHostListener | null = null
  let balanceDetails: BalanceDetails | null = null
  let externalWalletDetails: ExternalWalletDetails | null = null
  let orderInfo = parseDialogArgs()

  // TODO(zenparsing): Is this required?
  self.i18nTemplate.process(document, self.loadTimeData)

  function sendWalletInfo () {
    if (hostListener && balanceDetails && externalWalletDetails) {
      const balance = balanceDetails.total
      // TODO(zenparsing): Any other status codes?
      const verified = externalWalletDetails.status === 1
      hostListener.onWalletUpdated({ balance, verified })
    }
  }

  self.cr.addWebUIListener('walletBalanceUpdated', (event: any) => {
    const { details } = event

    // TODO(zenparsing): Details can be empty if rewards
    // are not enabled and the user does not have a wallet.
    // How do we detect this case without ignoring a real
    // error on startup? If we don't have any details, then
    // how do we get the rates which are required for
    // credit card processing?
    if (!details) {
      return
    }

    balanceDetails = details as BalanceDetails
    sendWalletInfo()

    if (hostListener) {
      hostListener.onExchangeRatesUpdated({
        rates: balanceDetails.rates,
        // TODO(zenparsing): Get from the service
        lastUpdated: new Date().toISOString()
      })
    }
  })

  self.cr.addWebUIListener('externalWalletUpdated', (event: any) => {
    const { details } = event
    if (!details) {
      return
    }
    externalWalletDetails = details as ExternalWalletDetails
    sendWalletInfo()
  })

  self.cr.addWebUIListener('rewardsEnabledUpdated', (event: any) => {
    const { rewardsEnabled } = event
    if (hostListener) {
      hostListener.onRewardsEnabledUpdated(rewardsEnabled)
    }
  })

  return {

    getLocaleString (key: string) {
      return self.loadTimeData.getString(key)
    },

    closeDialog () {
      chrome.send('dialogClose', [JSON.stringify({'action': 'cancel'})])
    },

    payWithCreditCard (...args) {
      console.log('payWithCreditCard', ...args)
      // TODO(zenparsing): Send update to service
    },

    payWithWallet (...args) {
      console.log('payWithWallet', ...args)
      chrome.send('paymentRequestComplete')
      chrome.send('dialogClose')
      // TODO(zenparsing): Send update to service
    },

    setListener (listener) {
      hostListener = listener

      chrome.send('getWalletBalance')
      chrome.send('getExternalWallet')
      chrome.send('getRewardsEnabled')

      queueMicrotask(() => {
        if (hostListener) {
          hostListener.onOrderUpdated(orderInfo)
        }
      })

      return () => { hostListener = null }
    }

  }
}
