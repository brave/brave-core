/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export interface WalletInfo {
  verified: boolean
  balance: number
}

export interface ExchangeRateInfo {
  rate: number
  lastUpdated: string
}

export interface OrderInfo {
  total: number
}

export interface BalanceInfo {
  total: number
  wallets: Record<string, number>
}

export interface HostError {
  type: string
  code?: number
}

export interface HostState {
  balanceInfo?: BalanceInfo
  hostError?: HostError
}

export interface HostListener {
  onWalletUpdated: (wallet: WalletInfo) => void
  onRatesUpdated: (exchangeInfo: ExchangeRateInfo) => void
  onOrderUpdated: (order: OrderInfo) => void
}

export enum DialogCloseReason {
  Complete,
  InsufficientBalance,
  UnverifiedWallet,
  UserCancelled
}

export interface Host {
  getLocaleString: (key: string) => string
  closeDialog: (reason: DialogCloseReason) => void
  payWithWallet: () => void
  setListener: (listener: HostListener) => void
}
