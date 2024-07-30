// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook } from '@testing-library/react'

// hooks
import { usePasswordAttempts } from './use-password-attempts'

// actions
import { WalletActions } from '../actions'

// mocks
import type { MockedWalletApiProxy } from '../async/__mocks__/bridge'
import getAPIProxy from '../async/bridge'
import {
  makeMockedStoreWithSpy,
  renderHookOptionsWithMockStore
} from '../../utils/test-utils'

describe('usePasswordAttempts hook', () => {
  it('should increment attempts on bad password & lock wallet after 3 failed attempts', async () => {
    const { store, dispatchSpy } = makeMockedStoreWithSpy()
    expect(store).toBeDefined()

    // auto-mocked by compiler
    // called after store creation to avoid api reset
    const proxy = getAPIProxy() as unknown as MockedWalletApiProxy
    proxy.setMockedStore(store)
    expect(proxy.store).toBeDefined()

    const { result } = renderHook(
      () => usePasswordAttempts(),
      renderHookOptionsWithMockStore(store)
    )

    expect(result.current.attempts).toEqual(0)

    // attempt 1
    await act(async () => {
      await result.current.attemptPasswordEntry('pass')
      expect(dispatchSpy).toHaveBeenCalledWith({
        payload: 1,
        type: 'wallet/setPasswordAttempts'
      })
    })

    expect(result.current.attempts).toEqual(1)

    // attempt 2
    await act(async () => {
      await result.current.attemptPasswordEntry('pass2')
      expect(dispatchSpy).toHaveBeenCalledWith({
        payload: 2,
        type: 'wallet/setPasswordAttempts'
      })
    })

    expect(result.current.attempts).toEqual(2)

    // attempt 3 (last before lock)
    await act(async () => {
      await result.current.attemptPasswordEntry('pass3')
      // attempts count is rest before locking
      expect(dispatchSpy).toHaveBeenCalledWith({
        payload: 0,
        type: 'wallet/setPasswordAttempts'
      })
    })

    // Wallet is now locked
    expect(result.current.attempts).toEqual(0)
    expect(dispatchSpy).toHaveBeenCalledWith(WalletActions.locked())
    expect(store.getState().wallet.isWalletLocked).toBe(true)
    // attempts should be reset since the wallet was locked
    expect(result.current.attempts).toEqual(0)
  })

  it('should return "true" for valid password', async () => {
    const { store } = makeMockedStoreWithSpy()

    const { result } = renderHook(
      () => usePasswordAttempts(),
      renderHookOptionsWithMockStore(store)
    )

    expect(result.current.attempts).toEqual(0)

    await act(async () => {
      // enter correct password
      const isValid = await result.current.attemptPasswordEntry('password')
      expect(isValid).toBe(true)
    })
  })

  it('should return "false" for invalid password', async () => {
    const { store } = makeMockedStoreWithSpy()

    const { result } = renderHook(
      () => usePasswordAttempts(),
      renderHookOptionsWithMockStore(store)
    )

    expect(result.current.attempts).toEqual(0)

    await act(async () => {
      // enter incorrect password
      const isValid = await result.current.attemptPasswordEntry('wrong!')
      expect(isValid).toBe(false)
    })
  })
})
