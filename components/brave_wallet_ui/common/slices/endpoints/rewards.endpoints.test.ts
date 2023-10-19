// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook } from '@testing-library/react-hooks'

// queries
import {
  useGetExternalRewardsWalletQuery,
  useGetRewardsBalanceQuery,
  useGetRewardsEnabledQuery
} from '../api.slice'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../../utils/test-utils'
import { BraveRewardsProxyOverrides } from '../../async/__mocks__/brave_rewards_api_proxy'
import { WalletStatus } from '../../async/brave_rewards_api_proxy'

const mockedRewardsData: BraveRewardsProxyOverrides = {
  externalWallet: {
    links: { account: '', reconnect: '' },
    provider: 'uphold',
    status: WalletStatus.kConnected,
    username: 'DeadBeef'
  },
  balance: 1234,
  rewardsEnabled: true
}

describe('api slice Brave Rewards endpoints', () => {
  describe('useGetExternalRewardsWalletQuery', () => {
    it('should fetch & cache external rewards wallet data', async () => {
      const store = createMockStore({}, {}, mockedRewardsData)

      const { result, waitForValueToChange } = renderHook(
        () => useGetExternalRewardsWalletQuery(),
        renderHookOptionsWithMockStore(store)
      )

      await waitForValueToChange(() => result.current.isLoading)
      const { data: wallet, isLoading, error } = result.current

      expect(isLoading).toBe(false)
      expect(error).not.toBeDefined()
      expect(wallet).toBeDefined()
      expect(wallet).toBe(mockedRewardsData.externalWallet)
    })
  })

  describe('useGetRewardsBalanceQuery', () => {
    it('should fetch & cache rewards balances', async () => {
      const store = createMockStore({}, {}, mockedRewardsData)

      const { result, waitForValueToChange } = renderHook(
        () => useGetRewardsBalanceQuery(),
        renderHookOptionsWithMockStore(store)
      )

      await waitForValueToChange(() => result.current.isLoading)
      const { data: balance, isLoading, error } = result.current

      expect(isLoading).toBe(false)
      expect(error).not.toBeDefined()
      expect(balance).toBeDefined()
      expect(balance).toBe(mockedRewardsData.balance)
    })
  })

  describe('useGetRewardsEnabledQuery', () => {
    it('should fetch & cache rewards enabled check', async () => {
      const store = createMockStore({}, {}, mockedRewardsData)

      const { result, waitForValueToChange } = renderHook(
        () => useGetRewardsEnabledQuery(),
        renderHookOptionsWithMockStore(store)
      )

      await waitForValueToChange(() => result.current.isLoading)
      const { data: isEnabled, isLoading, error } = result.current

      expect(isLoading).toBe(false)
      expect(error).not.toBeDefined()
      expect(isEnabled).toBeDefined()
      expect(isEnabled).toBe(mockedRewardsData.rewardsEnabled)
    })
  })
})
