/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {sendWithPromise} from 'chrome://resources/js/cr.js'

export enum CoinType {
  ETH = 60,
  FIL = 461,
  SOL = 501
}

export type Currency = {
  symbol: string
  name: string
  decimals: number
}

export type NetworkInfo = {
  chainId: string
  chainName: string
  blockExplorerUrls: string[]
  iconUrls: string[]
  activeRpcEndpointIndex: number
  rpcUrls: string[]
  coin: CoinType
  is_eip1559: boolean
  nativeCurrency: Currency
}

export type NetworksList = {
  defaultNetwork: string
  networks: NetworkInfo[]
  knownNetworks: string[]
  customNetworks: string[]
  hiddenNetworks: string[]
}

export type Option = {
  name: string
  value: number
}

export type SolanaProvider = Option

export interface BraveWalletBrowserProxy {
  resetWallet: () => void
  getWeb3ProviderList: () => Promise<string>
  getSolanaProviderOptions: () => Promise<SolanaProvider[]>
  getTransactionSimulationOptInStatusOptions: () => Promise<Option[]>
  isNativeWalletEnabled: () => Promise<boolean>
  isBitcoinEnabled: () => Promise<boolean>
  isZCashEnabled: () => Promise<boolean>
  getAutoLockMinutes: () => Promise<number>
  getNetworksList: (coin: number) => Promise<NetworksList>
  getPrepopulatedNetworksList: () => Promise<NetworkInfo[]>
  removeChain: (chainId: string, coin: number) => Promise<boolean>
  resetChain: (chainId: string, coin: number) => Promise<boolean>
  addChain: (value: NetworkInfo) => Promise<[boolean, string]>
  addHiddenNetwork: (chainId: string, coin: number) => Promise<boolean>
  removeHiddenNetwork: (chainId: string, coin: number) => Promise<boolean>
  setDefaultNetwork: (chainId: string, coin: number) => Promise<boolean>
  resetTransactionInfo: () => void
  isTransactionSimulationsFeatureEnabled: () => Promise<boolean>
  getWalletInPrivateWindowsEnabled: () => Promise<boolean>
  setWalletInPrivateWindowsEnabled: (enabled: boolean) => Promise<boolean>
}

export class BraveWalletBrowserProxyImpl implements BraveWalletBrowserProxy {
  resetWallet () {
    chrome.send('resetWallet', [])
  }

  resetTransactionInfo () {
    chrome.send('resetTransactionInfo', [])
  }

  getNetworksList (coin: number) {
    return sendWithPromise('getNetworksList', coin)
  }

  getPrepopulatedNetworksList () {
    return sendWithPromise('getPrepopulatedNetworksList')
  }

  setDefaultNetwork (chainId: string, coin: number) {
    return sendWithPromise('setDefaultNetwork', chainId, coin)
  }

  removeChain (chainId: string, coin: number) {
    return sendWithPromise('removeChain', chainId, coin)
  }

  resetChain (chainId: string, coin: number) {
    return sendWithPromise('resetChain', chainId, coin)
  }

  addChain (payload: NetworkInfo) {
    return sendWithPromise('addChain', payload)
  }

  addHiddenNetwork (chainId: string, coin: number) {
    return sendWithPromise('addHiddenNetwork', chainId, coin)
  }

  removeHiddenNetwork (chainId: string, coin: number) {
    return sendWithPromise('removeHiddenNetwork', chainId, coin)
  }

  getWeb3ProviderList () {
    return new Promise<string>(
      resolve => chrome.braveWallet.getWeb3ProviderList(resolve))
  }

  isNativeWalletEnabled() {
    return new Promise<boolean>(
      resolve => chrome.braveWallet.isNativeWalletEnabled(resolve))
  }

  getAutoLockMinutes () {
    return sendWithPromise('getAutoLockMinutes')
  }

  getSolanaProviderOptions() {
    return sendWithPromise('getSolanaProviderOptions')
  }


  isBitcoinEnabled() {
    return sendWithPromise('isBitcoinEnabled')
  }

  isZCashEnabled() {
    return sendWithPromise('isZCashEnabled')
  }

  getTransactionSimulationOptInStatusOptions() {
    return sendWithPromise('getTransactionSimulationOptInStatusOptions')
  }

  isTransactionSimulationsFeatureEnabled() {
    return sendWithPromise('isTransactionSimulationsFeatureEnabled')
  }

  getWalletInPrivateWindowsEnabled() {
    return sendWithPromise('getWalletInPrivateWindowsEnabled')
  }

  setWalletInPrivateWindowsEnabled(value: boolean) {
    return sendWithPromise('setWalletInPrivateWindowsEnabled', value)
  }

  static getInstance() {
    return instance || (instance = new BraveWalletBrowserProxyImpl())
  }
}

let instance: BraveWalletBrowserProxy|null = null
