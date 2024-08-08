// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook, waitFor } from '@testing-library/react'

// queries
import { useGetRewardsInfoQuery } from '../api.slice'

// constants
import { WalletStatus } from '../../../constants/types'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../../utils/test-utils'
import { BraveRewardsProxyOverrides } from '../../../constants/testing_types'

const mockedRewardsData: BraveRewardsProxyOverrides = {
  externalWallet: {
    url: '',
    name: 'DeadBeef',
    provider: 'uphold',
    status: WalletStatus.kConnected
  },
  balance: 1234,
  rewardsEnabled: true
}

describe('api slice Brave Rewards endpoints', () => {
  describe('useGetRewardsInfoQuery', () => {
    it('should fetch & cache external rewards wallet data', async () => {
      const store = createMockStore({}, {}, mockedRewardsData)

      const { result } = renderHook(
        () => useGetRewardsInfoQuery(),
        renderHookOptionsWithMockStore(store)
      )

      await waitFor(() =>
        expect(result.current.data && !result.current.isLoading).toBeTruthy()
      )
      const { data: rewardsInfo, isLoading, error } = result.current
      const { provider } = rewardsInfo || {}

      expect(isLoading).toBe(false)
      expect(error).not.toBeDefined()
      expect(provider).toBeDefined()
      expect(provider).toBe('uphold')
    })
    it('should fetch & cache rewards balances', async () => {
      const store = createMockStore({}, {}, mockedRewardsData)

      const { result } = renderHook(
        () => useGetRewardsInfoQuery(),
        renderHookOptionsWithMockStore(store)
      )

      // loading
      await waitFor(() =>
        expect(result.current.data && !result.current.isLoading).toBeTruthy()
      )

      const { data: rewardsInfo, isLoading, error } = result.current
      const { balance } = rewardsInfo || {}

      expect(isLoading).toBe(false)
      expect(error).not.toBeDefined()
      expect(balance).toBeDefined()
      expect(balance).toBe(mockedRewardsData.balance)
    })
    it('should fetch & cache rewards enabled check', async () => {
      const store = createMockStore({}, {}, mockedRewardsData)

      const { result } = renderHook(
        () => useGetRewardsInfoQuery(),
        renderHookOptionsWithMockStore(store)
      )

      // loading
      await waitFor(() =>
        expect(result.current.data && !result.current.isLoading).toBeTruthy()
      )

      const { data: rewardsInfo, isLoading, error } = result.current
      const { isRewardsEnabled } = rewardsInfo || {}

      expect(isLoading).toBe(false)
      expect(error).not.toBeDefined()
      expect(isRewardsEnabled).toBeDefined()
      expect(isRewardsEnabled).toBe(mockedRewardsData.rewardsEnabled)
    })
  })
})
