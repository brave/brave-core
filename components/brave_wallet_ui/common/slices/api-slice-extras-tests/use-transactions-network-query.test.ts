// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook, waitFor } from '@testing-library/react'

// types
import type { BraveWallet } from '../../../constants/types'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../../utils/test-utils'
import { deserializeTransaction } from '../../../utils/model-serialization-utils'
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'

// hooks
import { useTransactionsNetworkQuery } from '../api.slice.extra'

// mocks
import { mockNetwork } from '../../constants/mocks'
import {
  mockAccountAssetOptions,
  mockErc20TokensList
} from '../../../stories/mock-data/mock-asset-options'
import {
  mockTransactionInfo //
} from '../../../stories/mock-data/mock-transaction-info'

describe('api slice extra hooks', () => {
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
      const { result } = renderHook(
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
      await waitFor(() => !result.current.isLoading)
      await waitFor(() => result.current.data)

      // done loading
      expect(result.current.isLoading).toBe(false)
      expect(result.current.data).toBeDefined()
      expect(result.current.data).toBe(mockTxNetwork)
    })
  })
})
