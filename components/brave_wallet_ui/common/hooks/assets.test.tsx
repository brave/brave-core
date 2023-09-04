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
import { mockAccount } from '../constants/mocks'
import useAssets from './assets'
import * as MockedLib from '../async/__mocks__/lib'
import { LibContext } from '../context/lib.context'
import { mockWalletState } from '../../stories/mock-data/mock-wallet-state'
import { mockBasicAttentionToken, mockEthToken } from '../../stories/mock-data/mock-asset-options'
import { createMockStore } from '../../utils/test-utils'

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
  it('should return assets by value & network', async () => {
    const { result, waitForNextUpdate } = renderHook(
      () => useAssets(),
      renderHookOptionsWithCustomStore(
        createMockStore({
          walletStateOverride: {
            ...mockWalletState,
            userVisibleTokensInfo: mockVisibleList,
            accounts: [mockAccount]
          }
        },
        { selectedAccountId: mockAccount.accountId }
        )
      )
    )

    await waitForNextUpdate()

    expect(result.current).toEqual(mockVisibleList)
  })

  it('should return empty array for assets if visible assets is empty', () => {
    const { result } = renderHook(
      () => useAssets(),
      renderHookOptionsWithCustomStore(
        createMockStore({
          walletStateOverride: {
            ...mockWalletState,
            userVisibleTokensInfo: [],
            accounts: [mockAccount]
          }
        },
        { selectedAccountId: mockAccount.accountId }
        )
      )
    )
    expect(result.current).toEqual([])
  })
})
