// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'
import {
  parseJSONFromLocalStorage,
  makeInitialFilteredOutNetworkKeys,
  getPersistedSpotPrices,
  mergeAndPersistSpotPrices,
} from './local-storage-utils'
import { networkEntityAdapter } from '../common/slices/entities/network.entity'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'

const mockInitialFilteredOutNetworkKeys = [
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.SEPOLIA_CHAIN_ID,
      coin: BraveWallet.CoinType.ETH,
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.SOLANA_DEVNET,
      coin: BraveWallet.CoinType.SOL,
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.SOLANA_TESTNET,
      coin: BraveWallet.CoinType.SOL,
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.FILECOIN_TESTNET,
      coin: BraveWallet.CoinType.FIL,
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID,
      coin: BraveWallet.CoinType.ETH,
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.BITCOIN_TESTNET,
      coin: BraveWallet.CoinType.BTC,
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.Z_CASH_TESTNET,
      coin: BraveWallet.CoinType.ZEC,
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.CARDANO_TESTNET,
      coin: BraveWallet.CoinType.ADA,
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.POLKADOT_TESTNET,
      coin: BraveWallet.CoinType.DOT,
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.LOCALHOST_CHAIN_ID,
      coin: BraveWallet.CoinType.ETH,
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.LOCALHOST_CHAIN_ID,
      coin: BraveWallet.CoinType.SOL,
    })
    .toString(),
  networkEntityAdapter
    .selectId({
      chainId: BraveWallet.LOCALHOST_CHAIN_ID,
      coin: BraveWallet.CoinType.FIL,
    })
    .toString(),
]

describe('Test parseJSONFromLocalStorage', () => {
  const key = 'FILTERED_OUT_PORTFOLIO_NETWORK_KEYS'
  const initialNetworkKeys = makeInitialFilteredOutNetworkKeys()
  const mockValue = JSON.stringify(initialNetworkKeys)
  let mockLocalStorageGet = jest.fn()
  Object.defineProperty(window, 'localStorage', {
    value: {
      getItem: mockLocalStorageGet,
    },
  })
  it('getItem be called and the value should be correctly parsed', () => {
    mockLocalStorageGet.mockReturnValue(mockValue)
    expect(parseJSONFromLocalStorage(key, mockValue)).toEqual(
      initialNetworkKeys,
    )
    expect(window.localStorage.getItem).toHaveBeenCalledWith(
      LOCAL_STORAGE_KEYS[key],
    )
    expect(jest.isMockFunction(window.localStorage.getItem)).toBe(true)
    expect(mockLocalStorageGet.mock.results[0].value).toBe(mockValue)
  })
  it('getItem should return null, fallback should be returned', () => {
    mockLocalStorageGet.mockReturnValue(null)
    expect(parseJSONFromLocalStorage(key, initialNetworkKeys)).toEqual(
      initialNetworkKeys,
    )
    expect(window.localStorage.getItem).toHaveBeenCalledWith(
      LOCAL_STORAGE_KEYS[key],
    )
    expect(jest.isMockFunction(window.localStorage.getItem)).toBe(true)
    expect(mockLocalStorageGet.mock.results[0].value).toBe(null)
  })
})

describe('Test makeInitialFilteredOutNetworkKeys', () => {
  it('Should construct a string array of test network keys', () => {
    expect(makeInitialFilteredOutNetworkKeys()).toEqual(
      mockInitialFilteredOutNetworkKeys,
    )
  })
})

const mockEthPrice: BraveWallet.AssetPrice = {
  coin: BraveWallet.CoinType.ETH,
  chainId: '0x1',
  address: '0x0000000000000000000000000000000000000000',
  price: '4000',
  percentageChange24h: '2.5',
  vsCurrency: 'USD',
  cacheStatus: BraveWallet.Gate3CacheStatus.kHit,
  source: BraveWallet.AssetPriceSource.kCoingecko,
}

const mockBatPrice: BraveWallet.AssetPrice = {
  coin: BraveWallet.CoinType.ETH,
  chainId: '0x1',
  address: '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
  price: '0.88',
  percentageChange24h: '-1.2',
  vsCurrency: 'USD',
  cacheStatus: BraveWallet.Gate3CacheStatus.kHit,
  source: BraveWallet.AssetPriceSource.kCoingecko,
}

const mockSolPrice: BraveWallet.AssetPrice = {
  coin: BraveWallet.CoinType.SOL,
  chainId: '0x65',
  address: '',
  price: '150',
  percentageChange24h: '5.0',
  vsCurrency: 'USD',
  cacheStatus: BraveWallet.Gate3CacheStatus.kHit,
  source: BraveWallet.AssetPriceSource.kCoingecko,
}

describe('getPersistedSpotPrices', () => {
  const mockLocalStorageGet = window.localStorage.getItem as jest.Mock

  it('returns an empty array when localStorage is empty', () => {
    mockLocalStorageGet.mockReturnValue(null)
    expect(getPersistedSpotPrices()).toEqual([])
  })

  it('returns persisted prices from localStorage', () => {
    const stored: Record<string, BraveWallet.AssetPrice> = {
      [`${mockEthPrice.coin}-${mockEthPrice.chainId}-${mockEthPrice.address}`]:
        mockEthPrice,
    }
    mockLocalStorageGet.mockReturnValue(JSON.stringify(stored))
    expect(getPersistedSpotPrices()).toEqual([mockEthPrice])
  })

  it('returns an empty array on malformed JSON', () => {
    const spy = jest.spyOn(console, 'error').mockImplementation()
    mockLocalStorageGet.mockReturnValue('not valid json')
    expect(getPersistedSpotPrices()).toEqual([])
    expect(spy).toHaveBeenCalled()
    spy.mockRestore()
  })
})

describe('mergeAndPersistSpotPrices', () => {
  const mockLocalStorageGet = window.localStorage.getItem as jest.Mock
  const mockLocalStorageSet = jest.fn()

  beforeEach(() => {
    ;(window.localStorage as any).setItem = mockLocalStorageSet
    mockLocalStorageSet.mockClear()
  })

  it('persists prices to an empty store', () => {
    mockLocalStorageGet.mockReturnValue(null)
    mergeAndPersistSpotPrices([mockEthPrice])

    expect(mockLocalStorageSet).toHaveBeenCalledWith(
      LOCAL_STORAGE_KEYS.TOKEN_SPOT_PRICES,
      expect.any(String),
    )
    const stored = JSON.parse(mockLocalStorageSet.mock.calls[0][1])
    expect(Object.values(stored)).toEqual([mockEthPrice])
  })

  it('merges new prices without removing existing ones', () => {
    const key =
      `${mockEthPrice.coin}`
      + `-${mockEthPrice.chainId}`
      + `-${mockEthPrice.address}`
    const existing: Record<string, BraveWallet.AssetPrice> = {
      [key]: mockEthPrice,
    }
    mockLocalStorageGet.mockReturnValue(JSON.stringify(existing))

    mergeAndPersistSpotPrices([mockBatPrice])

    const stored = JSON.parse(mockLocalStorageSet.mock.calls[0][1])
    expect(Object.values(stored)).toHaveLength(2)
    expect(Object.values(stored)).toContainEqual(mockEthPrice)
    expect(Object.values(stored)).toContainEqual(mockBatPrice)
  })

  it('updates the price for an existing token', () => {
    const key =
      `${mockEthPrice.coin}`
      + `-${mockEthPrice.chainId}`
      + `-${mockEthPrice.address}`
    const existing: Record<string, BraveWallet.AssetPrice> = {
      [key]: mockEthPrice,
    }
    mockLocalStorageGet.mockReturnValue(JSON.stringify(existing))

    const updatedEthPrice = { ...mockEthPrice, price: '4200' }
    mergeAndPersistSpotPrices([updatedEthPrice])

    const stored = JSON.parse(mockLocalStorageSet.mock.calls[0][1])
    expect(Object.values(stored)).toHaveLength(1)
    expect(Object.values(stored)).toContainEqual(updatedEthPrice)
  })

  it('handles multiple fresh prices across different coins', () => {
    mockLocalStorageGet.mockReturnValue(null)
    mergeAndPersistSpotPrices([mockEthPrice, mockBatPrice, mockSolPrice])

    const stored = JSON.parse(mockLocalStorageSet.mock.calls[0][1])
    expect(Object.values(stored)).toHaveLength(3)
    expect(Object.values(stored)).toContainEqual(mockEthPrice)
    expect(Object.values(stored)).toContainEqual(mockBatPrice)
    expect(Object.values(stored)).toContainEqual(mockSolPrice)
  })

  it('normalizes address to lowercase for key consistency', () => {
    mockLocalStorageGet.mockReturnValue(null)
    const upperCaseAddress = {
      ...mockBatPrice,
      address: '0x0D8775F648430679A709E98D2B0CB6250D2887EF',
    }
    mergeAndPersistSpotPrices([upperCaseAddress])

    const stored = JSON.parse(mockLocalStorageSet.mock.calls[0][1])
    const keys = Object.keys(stored)
    expect(keys[0]).toContain('0x0d8775f648430679a709e98d2b0cb6250d2887ef')
  })
})
