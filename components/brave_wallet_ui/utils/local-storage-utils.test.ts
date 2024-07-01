// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'
import {
  parseJSONFromLocalStorage,
  makeInitialFilteredOutNetworkKeys
} from './local-storage-utils'
import { networkEntityAdapter } from '../common/slices/entities/network.entity'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'

const mockInitialFilteredOutNetworkKeys = [
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.SEPOLIA_CHAIN_ID,
      coin: BraveWallet.CoinType.ETH
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.SOLANA_DEVNET,
      coin: BraveWallet.CoinType.SOL
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.SOLANA_TESTNET,
      coin: BraveWallet.CoinType.SOL
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.FILECOIN_TESTNET,
      coin: BraveWallet.CoinType.FIL
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID,
      coin: BraveWallet.CoinType.ETH
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.BITCOIN_TESTNET,
      coin: BraveWallet.CoinType.BTC
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.Z_CASH_TESTNET,
      coin: BraveWallet.CoinType.ZEC
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.LOCALHOST_CHAIN_ID,
      coin: BraveWallet.CoinType.SOL
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.LOCALHOST_CHAIN_ID,
      coin: BraveWallet.CoinType.ETH
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.LOCALHOST_CHAIN_ID,
      coin: BraveWallet.CoinType.FIL
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.LOCALHOST_CHAIN_ID,
      coin: BraveWallet.CoinType.BTC
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.LOCALHOST_CHAIN_ID,
      coin: BraveWallet.CoinType.ZEC
    })
    .toString()
]

describe('Test parseJSONFromLocalStorage', () => {
  const key = 'FILTERED_OUT_PORTFOLIO_NETWORK_KEYS'
  const initialNetworkKeys = makeInitialFilteredOutNetworkKeys()
  const mockValue = JSON.stringify(initialNetworkKeys)
  let mockLocalStorageGet = jest.fn()
  Object.defineProperty(window, 'localStorage', {
    value: {
      getItem: mockLocalStorageGet
    }
  })
  it('getItem be called and the value should be correctly parsed', () => {
    mockLocalStorageGet.mockReturnValue(mockValue)
    expect(parseJSONFromLocalStorage(key, mockValue)).toEqual(
      initialNetworkKeys
    )
    expect(window.localStorage.getItem).toHaveBeenCalledWith(
      LOCAL_STORAGE_KEYS[key]
    )
    expect(jest.isMockFunction(window.localStorage.getItem)).toBe(true)
    expect(mockLocalStorageGet.mock.results[0].value).toBe(mockValue)
  })
  it('getItem should return null, fallback should be returned', () => {
    mockLocalStorageGet.mockReturnValue(null)
    expect(parseJSONFromLocalStorage(key, initialNetworkKeys)).toEqual(
      initialNetworkKeys
    )
    expect(window.localStorage.getItem).toHaveBeenCalledWith(
      LOCAL_STORAGE_KEYS[key]
    )
    expect(jest.isMockFunction(window.localStorage.getItem)).toBe(true)
    expect(mockLocalStorageGet.mock.results[0].value).toBe(null)
  })
})

describe('Test makeInitialFilteredOutNetworkKeys', () => {
  it('Should construct a string array of test network keys', () => {
    expect(makeInitialFilteredOutNetworkKeys()).toEqual(
      mockInitialFilteredOutNetworkKeys
    )
  })
})
