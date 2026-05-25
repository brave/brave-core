// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook } from '@testing-library/react'
import { useEmailAliases } from '../content/use_email_aliases'
import {
  Alias,
  AliasesUpdate,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import {
  installMockAuthentication,
  makeLoggedInAccountState,
  makeLoggedOutAccountState,
  restoreMockAuthentication,
} from './mock_authentication'

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
    installMockAuthentication(makeLoggedOutAccountState())
  })

  afterEach(() => {
    restoreMockAuthentication()
  })

  it('starts with empty aliases when logged out', () => {
    const { result } = renderHook(() => useEmailAliases(bindObserver))

    expect(result.current.accountState).toEqual(makeLoggedOutAccountState())
    expect(result.current.aliasesUpdate).toEqual({ aliases: [] })
  })

  it('applies alias list when logged in', () => {
    const aliases: Alias[] = [
      { email: 'a@brave.com', note: 'n', domains: undefined },
    ]
    installMockAuthentication(makeLoggedInAccountState('user@brave.com'))
    const { result } = renderHook(() => useEmailAliases(bindObserver))
    expect(lastObserver).toBeDefined()

    act(() => {
      lastObserver!.onAliasesUpdated({ aliases } as AliasesUpdate)
    })

    expect(result.current.aliasesUpdate).toEqual({ aliases })
  })

  it('applies error payload when logged in', () => {
    installMockAuthentication(makeLoggedInAccountState('user@brave.com'))
    const { result } = renderHook(() => useEmailAliases(bindObserver))

    act(() => {
      lastObserver!.onAliasesUpdated({
        error: 'load failed',
      } as AliasesUpdate)
    })

    expect(result.current.aliasesUpdate).toEqual({ error: 'load failed' })
  })

  it('ignores alias updates while logged out', () => {
    const aliases: Alias[] = [
      { email: 'a@brave.com', note: undefined, domains: undefined },
    ]
    const { result } = renderHook(() => useEmailAliases(bindObserver))

    act(() => {
      lastObserver!.onAliasesUpdated({ aliases } as AliasesUpdate)
    })

    expect(result.current.aliasesUpdate).toEqual({ aliases: [] })
  })

  it('resets aliases when account logs out', () => {
    const aliases: Alias[] = [
      { email: 'a@brave.com', note: undefined, domains: undefined },
    ]
    const mockAuth = installMockAuthentication(
      makeLoggedInAccountState('user@brave.com'),
    )
    const { result } = renderHook(() => useEmailAliases(bindObserver))

    act(() => {
      lastObserver!.onAliasesUpdated({ aliases } as AliasesUpdate)
    })
    expect(result.current.aliasesUpdate).toEqual({ aliases })

    act(() => {
      mockAuth.setAccountState(makeLoggedOutAccountState())
    })

    expect(result.current.aliasesUpdate).toEqual({ aliases: [] })
  })

  it('runs bindObserver cleanup on unmount', () => {
    const unbind = jest.fn()
    const bindObserver = (_observer: EmailAliasesServiceObserverInterface) =>
      unbind
    const { unmount } = renderHook(() => useEmailAliases(bindObserver))

    unbind.mockClear()
    unmount()
    expect(unbind).toHaveBeenCalledTimes(1)
  })
})
