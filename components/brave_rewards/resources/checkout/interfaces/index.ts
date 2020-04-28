/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export interface WalletInfo {
  verified: boolean
  balance: number
}

export interface ExchangeRateInfo {
  rates: Record<string, number>
  lastUpdated: string
}

export interface OrderInfo {
  description: string
  total: number
}

export interface CheckoutHostListener {
  onWalletUpdated: (wallet: WalletInfo) => void
  onExchangeRatesUpdated: (exchangeInfo: ExchangeRateInfo) => void
  onOrderUpdated: (order: OrderInfo) => void
  onRewardsEnabledUpdated: (enabled: boolean) => void
}

export interface CheckoutHost {
  getLocaleString: (key: string) => string
  closeDialog: () => void
  payWithCreditCard: () => void
  payWithWallet: () => void
  setListener: (listener: CheckoutHostListener) => void
}
