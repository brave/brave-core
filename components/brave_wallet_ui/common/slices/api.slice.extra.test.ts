// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook } from '@testing-library/react-hooks'

// types
import type { BraveWallet } from '../../constants/types'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../utils/test-utils'
import {
  deserializeTransaction,
  makeSerializableTransaction
} from '../../utils/model-serialization-utils'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'

// hooks
import {
  useAccountQuery,
  useGetCombinedTokensListQuery,
  useTransactionQuery,
  useTransactionsNetworkQuery
} from './api.slice.extra'

// mocks
import { mockAccount, mockNetwork } from '../constants/mocks'
import {
  mockAccountAssetOptions,
  mockErc20TokensList
} from '../../stories/mock-data/mock-asset-options'
import {
  mockTransactionInfo //
} from '../../stories/mock-data/mock-transaction-info'

describe('api slice extra hooks', () => {
  describe('useAccountQuery', () => {
    it('finds a transaction from the cache', async () => {
      // setup
      const store = createMockStore(
        {},
        {
          accountInfos: [mockAccount]
        }
      )
      const renderOptions = renderHookOptionsWithMockStore(store)
      const hook = renderHook(
        () => useAccountQuery(mockAccount.address),
        renderOptions
      )
      const hookInstance2 = renderHook(
        () => useAccountQuery(mockAccount.address),
        renderOptions
      )

      // initial state
      expect(hook.result.current.isLoading).toBeDefined()
      expect(hook.result.current.isLoading).toBe(true)
      expect(hook.result.current.account).not.toBeDefined()

      // loading
      await hook.waitFor(() => hook.result.all.length > 2)

      // loaded
      expect(hook.result.current.isLoading).toBe(false)
      expect(hook.result.current.error).not.toBeDefined()
      expect(hook.result.current.account).toBeDefined()
      expect(hook.result.current.account?.address).toEqual(mockAccount.address)

      // additional instances should not
      // create more than one account in memory
      expect(hookInstance2.result.current.account).toBe(
        hook.result.current.account
      )
    })
  })

  describe('useTransactionQuery', () => {
    it('finds a transaction from the cache', async () => {
      // setup
      const mockTx = deserializeTransaction(mockTransactionInfo)
      const mockAccountForMockedTx = {
        ...mockAccount,
        address: mockTransactionInfo.fromAddress
      }
      const store = createMockStore(
        {},
        {
          accountInfos: [mockAccountForMockedTx],
          transactionInfos: [mockTx]
        }
      )
      const renderOptions = renderHookOptionsWithMockStore(store)
      const hook = renderHook(
        () => useTransactionQuery(mockTransactionInfo.id),
        renderOptions
      )
      const hookInstance2 = renderHook(
        () => useTransactionQuery(mockTransactionInfo.id),
        renderOptions
      )

      // initial state
      expect(hook.result.current.isLoading).toBeDefined()
      expect(hook.result.current.isLoading).toBe(true)
      expect(hook.result.current.transaction).not.toBeDefined()

      // loading
      await hook.waitFor(() => hook.result.all.length > 2)

      // loaded
      expect(hook.result.current.isLoading).toBe(false)
      expect(hook.result.current.transaction?.id).toBeDefined()
      expect(hook.result.current.transaction).toEqual(
        makeSerializableTransaction(mockTx)
      )

      // additional instances should not
      // create more than one transaction in memory
      expect(hookInstance2.result.current.transaction).toBe(
        hook.result.current.transaction
      )
    })
  })

  describe('useCombinedTokensList', () => {
    it('returns the combo of user assets and known tokens', async () => {
      const store = createMockStore(
        {},
        {
          blockchainTokens: mockErc20TokensList,
          userAssets: mockAccountAssetOptions
        }
      )
      const renderOptions = renderHookOptionsWithMockStore(store)
      const { result, ...hook } = renderHook(
        () => useGetCombinedTokensListQuery(),
        renderOptions
      )

      // initial state
      expect(result.current.data).toBeDefined()
      expect(result.current.isLoading).toBeDefined()
      expect(result.current.isLoading).toBe(true)

      // loading
      await hook.waitFor(() => result.all.length > 3)

      // done loading
      expect(result.current.isLoading).toBe(false)
      expect(result.current.data).toHaveLength(15)
    })
  })

  describe('useTransactionsNetworkQuery', () => {
    it('returns a network entity for a chainId + coinType', async () => {
      const mockTx = deserializeTransaction(mockTransactionInfo)
      const mockTxNetwork: BraveWallet.NetworkInfo = {
        ...mockNetwork,
        chainId: mockTx.chainId,
        coin: getCoinFromTxDataUnion(mockTx.txDataUnion)
      }
      const store = createMockStore(
        {},
        {
          blockchainTokens: mockErc20TokensList,
          userAssets: mockAccountAssetOptions,
          networks: [mockTxNetwork]
        }
      )

      // check mock data
      expect(getCoinFromTxDataUnion(mockTx.txDataUnion)).toBe(
        mockTxNetwork.coin
      )
      expect(mockTx.chainId).toBe(mockTxNetwork.chainId)

      // render
      const renderOptions = renderHookOptionsWithMockStore(store)
      const { result, ...hook } = renderHook(
        () =>
          useTransactionsNetworkQuery({
            chainId: mockTx.chainId,
            txDataUnion: mockTx.txDataUnion
          }),
        renderOptions
      )

      // initial state
      expect(result.current.isLoading).toBeDefined()
      expect(result.current.isLoading).toBe(true)

      // loading
      await hook.waitFor(() => result.all.length > 2)

      // done loading
      expect(result.current.isLoading).toBe(false)
      expect(result.current.data).toBeDefined()
      expect(result.current.data).toBe(mockTxNetwork)
    })
  })
})
