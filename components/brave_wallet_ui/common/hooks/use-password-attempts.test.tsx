// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { act, renderHook } from '@testing-library/react-hooks'

import { usePasswordAttempts } from './use-password-attempts'
import { ApiProxyContext } from '../context/api-proxy.context'
import { getMockedAPIProxy, makeMockedStoreWithSpy } from '../async/__mocks__/bridge'
import { WalletActions } from '../actions'

const proxy = getMockedAPIProxy()

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

describe('usePasswordAttempts hook', () => {
  it('should increment attempts on bad password & lock wallet after 3 failed attempts', async () => {
    const { store, dispatchSpy } = makeMockedStoreWithSpy()
    proxy.setMockedStore(store)

    const {
      result
    } = renderHook(() => usePasswordAttempts(), renderHookOptionsWithCustomStore(store))

    expect(result.current.attempts).toEqual(0)

    // attempt 1
    await act(async () => {
      await result.current.attemptPasswordEntry('pass')
    })

    expect(result.current.attempts).toEqual(1)

    // attempt 2
    await act(async () => {
      await result.current.attemptPasswordEntry('pass2')
    })

    expect(result.current.attempts).toEqual(2)

    // attempt 3
    await act(async () => {
      await result.current.attemptPasswordEntry('pass3')
    })

    // Wallet is now locked
    await act(async () => {
      expect(dispatchSpy).toHaveBeenCalledWith(WalletActions.locked())
      expect(store.getState().wallet.isWalletLocked).toBe(true)
    })
    // attempts should be reset since the wallet was locked
    expect(result.current.attempts).toEqual(0)
  })

  it('should return "true" for valid password', async () => {
    const { store } = makeMockedStoreWithSpy()

    const {
      result
    } = renderHook(() => usePasswordAttempts(), renderHookOptionsWithCustomStore(store))

    expect(result.current.attempts).toEqual(0)

    await act(async () => {
      // enter correct password
      const isValid = await result.current.attemptPasswordEntry('password')
      expect(isValid).toBe(true)
    })
  })

  it('should return "false" for invalid password', async () => {
    const { store } = makeMockedStoreWithSpy()

    const {
      result
    } = renderHook(() => usePasswordAttempts(), renderHookOptionsWithCustomStore(store))

    expect(result.current.attempts).toEqual(0)

    await act(async () => {
      // enter incorrect password
      const isValid = await result.current.attemptPasswordEntry('wrong!')
      expect(isValid).toBe(false)
    })
  })
})
