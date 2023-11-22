// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BaseQueryCache,
  resetCache,
  setApiProxyFetcher
} from './base-query-cache'

// mocks
import { getMockedAPIProxy } from './__mocks__/bridge'

describe('BaseQueryCache', () => {
  beforeAll(() => {
    setApiProxyFetcher(getMockedAPIProxy)
  })

  beforeEach(() => {
    resetCache()
  })

  it('should cache WalletInfo after fetching', async () => {
    const getWalletInfoSpy = jest.spyOn(
      getMockedAPIProxy().walletHandler,
      'getWalletInfo'
    )
    expect(getWalletInfoSpy).toHaveBeenCalledTimes(0)

    expect(BaseQueryCache).toBeDefined()

    const cache = new BaseQueryCache()

    expect(cache).toBeDefined()

    // access the uncached WalletInfo
    const accounts = await cache.getWalletInfo()
    expect(accounts).toBeDefined()
    expect(getWalletInfoSpy).toHaveBeenCalledTimes(1)

    // re-access the WalletInfo, this time from cache
    const cachedAccounts = await cache.getWalletInfo()
    expect(cachedAccounts).toBeDefined()
    // no additional calls made
    expect(getWalletInfoSpy).toHaveBeenCalledTimes(1)

    // clear the cache manually
    cache.clearWalletInfo()

    // access again, repopulating cache with fresh value
    const reCachedAccounts = await cache.getWalletInfo()
    expect(reCachedAccounts).toBeDefined()
    expect(getWalletInfoSpy).toHaveBeenCalledTimes(2)

    // reset spy
    getWalletInfoSpy.mockReset()
    getWalletInfoSpy.mockRestore()
  })

  it('should cache accounts after fetching', async () => {
    const getAllAcountsSpy = jest.spyOn(
      getMockedAPIProxy().keyringService,
      'getAllAccounts'
    )
    expect(getAllAcountsSpy).toHaveBeenCalledTimes(0)

    const cache = new BaseQueryCache()

    // access the uncached registry
    const accounts = await cache.getAccountsRegistry()
    expect(accounts).toBeDefined()
    expect(getAllAcountsSpy).toHaveBeenCalledTimes(1)

    // re-access the registry, this time from cache
    const cachedAccounts = await cache.getAccountsRegistry()
    expect(cachedAccounts).toBeDefined()
    // no additional calls made
    expect(getAllAcountsSpy).toHaveBeenCalledTimes(1)

    // clear the cache manually
    cache.clearWalletInfo()

    // access again, repopulating cache with fresh value
    const reCachedAccounts = await cache.getAccountsRegistry()
    expect(reCachedAccounts).toBeDefined()
    expect(getAllAcountsSpy).toHaveBeenCalledTimes(2)

    // reset spy
    getAllAcountsSpy.mockReset()
    getAllAcountsSpy.mockRestore()
  })

  it('should cache networks after fetching', async () => {
    const getWalletInfoSpy = jest.spyOn(
      getMockedAPIProxy().walletHandler,
      'getWalletInfo'
    )
    const getAllNetworksSpy = jest.spyOn(
      getMockedAPIProxy().jsonRpcService,
      'getAllNetworks'
    )
    const getHiddenNetworksSpy = jest.spyOn(
      getMockedAPIProxy().jsonRpcService,
      'getHiddenNetworks'
    )
    expect(getWalletInfoSpy).toHaveBeenCalledTimes(0)
    expect(getAllNetworksSpy).toHaveBeenCalledTimes(0)
    expect(getHiddenNetworksSpy).toHaveBeenCalledTimes(0)

    const cache = new BaseQueryCache()

    // access the uncached registry
    const registry = await cache.getNetworksRegistry()
    expect(registry.entities).toBeDefined()
    // once per coin type (ETH, FIL, SOL, BTC, ZEC)
    const numberOfCoins = 5
    expect(getWalletInfoSpy).toHaveBeenCalledTimes(1)
    expect(getAllNetworksSpy).toHaveBeenCalledTimes(numberOfCoins)
    expect(getHiddenNetworksSpy).toHaveBeenCalledTimes(numberOfCoins)

    // re-access the registry, this time from cache
    const cachedRegistry = await cache.getNetworksRegistry()
    expect(cachedRegistry.entities[cachedRegistry.ids[0]]).toBeDefined()
    // no additional calls made
    expect(getWalletInfoSpy).toHaveBeenCalledTimes(1)
    expect(getAllNetworksSpy).toHaveBeenCalledTimes(numberOfCoins)
    expect(getHiddenNetworksSpy).toHaveBeenCalledTimes(numberOfCoins)

    // clear the cache manually
    cache.clearNetworksRegistry()

    // access again, repopulating cache with fresh value
    const reCachedRegistry = await cache.getNetworksRegistry()
    expect(reCachedRegistry).toBeDefined()
    expect(getAllNetworksSpy).toHaveBeenCalledTimes(numberOfCoins * 2)
    expect(getHiddenNetworksSpy).toHaveBeenCalledTimes(numberOfCoins * 2)
    // no need to update wallet-info
    expect(getWalletInfoSpy).toHaveBeenCalledTimes(1)

    // reset spies
    getWalletInfoSpy.mockReset()
    getWalletInfoSpy.mockRestore()
    getAllNetworksSpy.mockReset()
    getAllNetworksSpy.mockRestore()
    getHiddenNetworksSpy.mockReset()
    getHiddenNetworksSpy.mockRestore()
  })
})
