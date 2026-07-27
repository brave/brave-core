// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook } from '@testing-library/react'
import {
  getLoggedInEmail,
  isAccountLoggedIn,
  useEmailAliases,
} from '../content/use_email_aliases'
import type { AccountState } from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'
import {
  Alias,
  AliasesUpdate,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

const makeLoggedOutAccountState = (): AccountState =>
  ({ loggedOut: {} }) as AccountState
const makeLoggedInAccountState = (email: string): AccountState =>
  ({ loggedIn: { email } }) as AccountState

describe('isAccountLoggedIn', () => {
  it('returns false when account state is undefined', () => {
    expect(isAccountLoggedIn(undefined)).toBe(false)
  })

  it('returns false when logged out', () => {
    expect(isAccountLoggedIn(makeLoggedOutAccountState())).toBe(false)
  })

  it('returns true when logged in', () => {
    expect(isAccountLoggedIn(makeLoggedInAccountState('user@brave.com'))).toBe(
      true,
    )
  })
})

describe('getLoggedInEmail', () => {
  it('returns empty string when logged out', () => {
    expect(getLoggedInEmail(undefined)).toBe('')
    expect(getLoggedInEmail(makeLoggedOutAccountState())).toBe('')
  })

  it('returns the logged-in email', () => {
    expect(getLoggedInEmail(makeLoggedInAccountState('user@brave.com'))).toBe(
      'user@brave.com',
    )
  })
})

describe('useEmailAliases', () => {
  let lastObserver: EmailAliasesServiceObserverInterface | undefined

  const bindObserver = (observer: EmailAliasesServiceObserverInterface) => {
    lastObserver = observer
    return () => {
      lastObserver = undefined
    }
  }

  beforeEach(() => {
    lastObserver = undefined
  })

  it('starts with empty aliases', () => {
    const { result } = renderHook(() => useEmailAliases(bindObserver))

    expect(result.current.aliasesUpdate).toEqual({ aliases: [] })
  })

  it('applies alias list updates', () => {
    const aliases: Alias[] = [
      { email: 'a@brave.com', note: 'n', domains: undefined },
    ]
    const { result } = renderHook(() => useEmailAliases(bindObserver))
    expect(lastObserver).toBeDefined()

    act(() => {
      lastObserver!.onAliasesUpdated({ aliases } as AliasesUpdate)
    })

    expect(result.current.aliasesUpdate).toEqual({ aliases })
  })

  it('applies error payload', () => {
    const { result } = renderHook(() => useEmailAliases(bindObserver))

    act(() => {
      lastObserver!.onAliasesUpdated({
        error: 'load failed',
      } as AliasesUpdate)
    })

    expect(result.current.aliasesUpdate).toEqual({ error: 'load failed' })
  })

  it('runs bindObserver cleanup on unmount', () => {
    const unbind = jest.fn()
    const { unmount } = renderHook(() => useEmailAliases((_observer) => unbind))

    unbind.mockClear()
    unmount()
    expect(unbind).toHaveBeenCalledTimes(1)
  })
})
