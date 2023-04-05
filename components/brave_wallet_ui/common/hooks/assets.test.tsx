// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { Provider } from 'react-redux'
import { TextEncoder, TextDecoder } from 'util'
// @ts-expect-error
global.TextDecoder = TextDecoder
global.TextEncoder = TextEncoder
import { renderHook } from '@testing-library/react-hooks'
import { mockAccount, mockAssetPrices } from '../constants/mocks'
import useAssets from './assets'
import { WalletAccountType } from '../../constants/types'
import * as MockedLib from '../async/__mocks__/lib'
import { LibContext } from '../context/lib.context'
import { mockWalletState } from '../../stories/mock-data/mock-wallet-state'
import { mockBasicAttentionToken, mockEthToken } from '../../stories/mock-data/mock-asset-options'
import { createMockStore } from '../../utils/test-utils'

const mockAccounts = [
  {
    ...mockAccount,
    tokenBalanceRegistry: {
      [mockEthToken.contractAddress.toLowerCase()]: '238699740940532500',
      [mockBasicAttentionToken.contractAddress.toLowerCase()]: '0'
    }
  } as WalletAccountType,
  {
    ...mockAccount,
    balance: '',
    tokenBalanceRegistry: {
      [mockEthToken.contractAddress.toLowerCase()]: '0',
      [mockBasicAttentionToken.contractAddress.toLowerCase()]: '0'
    }
  } as WalletAccountType
]

const mockVisibleList = [
  mockEthToken,
  mockBasicAttentionToken
]

const renderHookOptionsWithCustomStore = (store: any) => ({
  wrapper: ({ children }: { children?: React.ReactChildren }) =>
    <Provider store={store}>
      <LibContext.Provider value={MockedLib as any}>
        {children}
      </LibContext.Provider>
    </Provider>
})

describe('useAssets hook', () => {
  it('should return panel user assets by value & network', async () => {
    const { result, waitForNextUpdate } = renderHook(
      () => useAssets(),
      renderHookOptionsWithCustomStore(
        createMockStore({
          walletStateOverride: {
            ...mockWalletState,
            userVisibleTokensInfo: mockVisibleList,
            selectedAccount: mockAccounts[0],
            accounts: mockAccounts,
            transactionSpotPrices: mockAssetPrices
          }
        })
      )
    )

    await waitForNextUpdate()

    expect(result.current.panelUserAssetList).toEqual(mockVisibleList)
  })

  it('should return empty array for panelUserAssetList if visible assets is empty', () => {
    const { result } = renderHook(
      () => useAssets(),
      renderHookOptionsWithCustomStore(
        createMockStore({
          walletStateOverride: {
            ...mockWalletState,
            userVisibleTokensInfo: [],
            selectedAccount: mockAccounts[0],
            accounts: mockAccounts,
            transactionSpotPrices: mockAssetPrices
          }
        })
      )
    )
    expect(result.current.panelUserAssetList).toEqual([])
  })
})
