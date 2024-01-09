// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook } from '@testing-library/react-hooks'

// queries
import { useGetRewardsInfoQuery } from '../api.slice'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../../utils/test-utils'
import { BraveRewardsProxyOverrides } from '../../async/__mocks__/brave_rewards_api_proxy'
import { WalletStatus } from '../../async/brave_rewards_api_proxy'

const mockedRewardsData: BraveRewardsProxyOverrides = {
  externalWallet: {
    links: { account: '' },
    provider: 'uphold',
    status: WalletStatus.kConnected,
    username: 'DeadBeef'
  },
  balance: 1234,
  rewardsEnabled: true
}

describe('api slice Brave Rewards endpoints', () => {
  describe('useGetRewardsInfoQuery', () => {
    it('should fetch & cache external rewards wallet data', async () => {
      const store = createMockStore({}, {}, mockedRewardsData)

      const { result, waitForValueToChange } = renderHook(
        () => useGetRewardsInfoQuery(),
        renderHookOptionsWithMockStore(store)
      )

      await waitForValueToChange(() => result.current.isLoading)
      const { data: rewardsInfo, isLoading, error } = result.current
      const { provider } = rewardsInfo || {}

      expect(isLoading).toBe(false)
      expect(error).not.toBeDefined()
      expect(provider).toBeDefined()
      expect(provider).toBe('uphold')
    })
    it('should fetch & cache rewards balances', async () => {
      const store = createMockStore({}, {}, mockedRewardsData)

      const { result, waitForValueToChange } = renderHook(
        () => useGetRewardsInfoQuery(),
        renderHookOptionsWithMockStore(store)
      )

      await waitForValueToChange(() => result.current.isLoading)
      const { data: rewardsInfo, isLoading, error } = result.current
      const { balance } = rewardsInfo || {}

      expect(isLoading).toBe(false)
      expect(error).not.toBeDefined()
      expect(balance).toBeDefined()
      expect(balance).toBe(mockedRewardsData.balance)
    })
    it('should fetch & cache rewards enabled check', async () => {
      const store = createMockStore({}, {}, mockedRewardsData)

      const { result, waitForValueToChange } = renderHook(
        () => useGetRewardsInfoQuery(),
        renderHookOptionsWithMockStore(store)
      )

      await waitForValueToChange(() => result.current.isLoading)
      const { data: rewardsInfo, isLoading, error } = result.current
      const { isRewardsEnabled } = rewardsInfo || {}

      expect(isLoading).toBe(false)
      expect(error).not.toBeDefined()
      expect(isRewardsEnabled).toBeDefined()
      expect(isRewardsEnabled).toBe(mockedRewardsData.rewardsEnabled)
    })
  })
})
