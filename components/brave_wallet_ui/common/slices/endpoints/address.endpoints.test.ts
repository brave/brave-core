// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook, waitFor } from '@testing-library/react'

// types
import { BraveWallet } from '../../../constants/types'

// queries
import { useGetAddressFromNameServiceUrlQuery } from '../api.slice'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore,
} from '../../../utils/test-utils'

describe('api slice address endpoints', () => {
  describe('useGetAddressFromNameServiceUrlQuery', () => {
    it('resolves ENS names for the selected coin and network', async () => {
      const store = createMockStore({})

      const { result } = renderHook(
        () =>
          useGetAddressFromNameServiceUrlQuery({
            url: 'alice.eth',
            tokenId: null,
            coin: BraveWallet.CoinType.SOL,
            chainId: BraveWallet.SOLANA_MAINNET,
          }),
        renderHookOptionsWithMockStore(store),
      )

      await waitFor(() => expect(result.current.isSuccess).toBe(true))
      expect(result.current.data?.address).toBe(
        `ens-${BraveWallet.CoinType.SOL}-${BraveWallet.SOLANA_MAINNET}-`
          + 'alice.eth',
      )
    })

    it('passes the EVM chain ID to ENS lookups', async () => {
      const store = createMockStore({})

      const { result } = renderHook(
        () =>
          useGetAddressFromNameServiceUrlQuery({
            url: 'alice.eth',
            tokenId: null,
            coin: BraveWallet.CoinType.ETH,
            chainId: BraveWallet.BASE_MAINNET_CHAIN_ID,
          }),
        renderHookOptionsWithMockStore(store),
      )

      await waitFor(() => expect(result.current.isSuccess).toBe(true))
      expect(result.current.data?.address).toBe(
        `ens-${BraveWallet.CoinType.ETH}-${BraveWallet.BASE_MAINNET_CHAIN_ID}-`
          + 'alice.eth',
      )
    })

    it('fails ENS lookups without a coin and network', async () => {
      const store = createMockStore({})

      const { result } = renderHook(
        () =>
          useGetAddressFromNameServiceUrlQuery({
            url: 'alice.eth',
            tokenId: null,
          }),
        renderHookOptionsWithMockStore(store),
      )

      await waitFor(() => expect(result.current.isError).toBe(true))
      expect(result.current.data).toBeUndefined()
    })
  })
})
