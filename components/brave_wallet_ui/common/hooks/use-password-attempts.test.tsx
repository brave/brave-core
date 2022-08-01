// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createStore, combineReducers } from 'redux'
import { Provider } from 'react-redux'
import { act, renderHook } from '@testing-library/react-hooks'

import { createWalletReducer } from '../reducers/wallet_reducer'
import { usePasswordAttempts } from './use-password-attempts'
import { mockWalletState } from '../../stories/mock-data/mock-wallet-state'
import { ApiProxyContext } from '../context/api-proxy.context'
import { getMockedAPIProxy } from '../async/__mocks__/bridge'

const proxy = getMockedAPIProxy()
proxy.keyringService.lock = jest.fn(proxy.keyringService.lock)

const makeStore = () => {
  const store = createStore(combineReducers({
    wallet: createWalletReducer(mockWalletState)
  }))

  store.dispatch = jest.fn(store.dispatch)
  return store
}

function renderHookOptionsWithCustomStore (store: any) {
  return {
    wrapper: ({ children }: { children?: React.ReactChildren }) =>
    <ApiProxyContext.Provider value={proxy}>
      <Provider store={store}>
        {children}
      </Provider>
    </ApiProxyContext.Provider>
  }
}

const MAX_ATTEMPTS = 3

describe('useTransactionParser hook', () => {
  it('should increment attempts on bad password ', async () => {
    const store = makeStore()

    const {
      result
    } = renderHook(() => usePasswordAttempts({
      maxAttempts: MAX_ATTEMPTS
    }), renderHookOptionsWithCustomStore(store))

    expect(result.current.attempts).toEqual(0)

    // attempt 1
    await act(async () => {
      await result.current.attemptPasswordEntry('pass')
    })

    expect(result.current.attempts).toEqual(1)

    // attempt 2
    await act(async () => {
      await result.current.attemptPasswordEntry('pass')
    })

    expect(result.current.attempts).toEqual(2)

    // attempt 3
    await act(async () => {
      await result.current.attemptPasswordEntry('pass')
    })

    // Wallet is now locked
    expect(proxy.keyringService.lock).toHaveBeenCalled()

    // attempts should be reset since wallet was locked
    expect(result.current.attempts).toEqual(0)
  })
})
