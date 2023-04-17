// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { renderHook } from '@testing-library/react-hooks'

import { useGetAllTransactionInfosForAddressCoinTypeQuery } from './api.slice'

import { mockAccount } from '../constants/mocks'
import { createMockStore } from '../../utils/test-utils'
import { mockTransactionInfo } from '../../stories/mock-data/mock-transaction-info'

function renderHookOptionsWithCustomStore (store: any) {
  return {
    wrapper: ({ children }: { children?: React.ReactChildren }) =>
      <Provider store={store}>
          {children}
      </Provider>
  }
}

describe('api slice: getAllTransactionInfosForAddressCoinType', () => {
  it('should fetch & cache transaction infos for an account', async () => {
    const store = createMockStore({})

    const { result, waitForValueToChange } = renderHook(
      () =>
        useGetAllTransactionInfosForAddressCoinTypeQuery({
          address: mockAccount.address,
          coinType: mockAccount.coin
        }),
      renderHookOptionsWithCustomStore(store)
    )

    await waitForValueToChange(() => result.current.isLoading)
    const { data: txs, isLoading, error } = result.current

    expect(isLoading).toBe(false)
    expect(error).not.toBeDefined()
    expect(txs).toBeDefined()
    expect(txs?.[0]).toBeDefined()
    expect(txs?.[0]).toEqual(mockTransactionInfo)
    expect(txs?.[0].fromAddress).toEqual(mockTransactionInfo.fromAddress)
  })
})