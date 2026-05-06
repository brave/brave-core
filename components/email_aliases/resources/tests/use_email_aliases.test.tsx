// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook } from '@testing-library/react'
import { useEmailAliases } from '../content/use_email_aliases'
import {
  Alias,
  AliasesUpdate,
  AuthenticationStatus,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

describe('useEmailAliases', () => {
  let lastObserver: EmailAliasesServiceObserverInterface | undefined

  const bindObserver = (
    observer: EmailAliasesServiceObserverInterface,
  ) => {
    lastObserver = observer
    return () => {
      lastObserver = undefined
    }
  }

  beforeEach(() => {
    lastObserver = undefined
  })

  it('starts with startup auth and empty aliases', () => {
    const { result } = renderHook(() => useEmailAliases(bindObserver))

    expect(result.current.authState).toEqual({
      status: AuthenticationStatus.kStartup,
      email: '',
    })
    expect(result.current.aliasesUpdate).toEqual({ aliases: [] })
  })

  it('updates auth state from onAuthStateChanged', () => {
    const { result } = renderHook(() => useEmailAliases(bindObserver))
    expect(lastObserver).toBeDefined()

    act(() => {
      lastObserver!.onAuthStateChanged({
        status: AuthenticationStatus.kAuthenticated,
        email: 'user@brave.com',
      })
    })

    expect(result.current.authState).toEqual({
      status: AuthenticationStatus.kAuthenticated,
      email: 'user@brave.com',
    })
  })

  it('applies alias list when authenticated', () => {
    const aliases: Alias[] = [
      { email: 'a@brave.com', note: 'n', domains: undefined },
    ]
    const { result } = renderHook(() => useEmailAliases(bindObserver))

    act(() => {
      lastObserver!.onAuthStateChanged({
        status: AuthenticationStatus.kAuthenticated,
        email: 'user@brave.com',
      })
    })
    act(() => {
      lastObserver!.onAliasesUpdated({ aliases } as AliasesUpdate)
    })

    expect(result.current.aliasesUpdate).toEqual({ aliases })
  })

  it('applies error payload when authenticated', () => {
    const { result } = renderHook(() => useEmailAliases(bindObserver))

    act(() => {
      lastObserver!.onAuthStateChanged({
        status: AuthenticationStatus.kAuthenticated,
        email: 'user@brave.com',
      })
    })
    act(() => {
      lastObserver!.onAliasesUpdated({
        error: 'load failed',
      } as AliasesUpdate)
    })

    expect(result.current.aliasesUpdate).toEqual({ error: 'load failed' })
  })

  it('ignores alias updates while not authenticated', () => {
    const aliases: Alias[] = [
      { email: 'a@brave.com', note: undefined, domains: undefined },
    ]
    const { result } = renderHook(() => useEmailAliases(bindObserver))

    act(() => {
      lastObserver!.onAuthStateChanged({
        status: AuthenticationStatus.kUnauthenticated,
        email: '',
      })
    })
    act(() => {
      lastObserver!.onAliasesUpdated({ aliases } as AliasesUpdate)
    })

    expect(result.current.aliasesUpdate).toEqual({ aliases: [] })
  })

  it('resets aliases when auth leaves authenticated', () => {
    const aliases: Alias[] = [
      { email: 'a@brave.com', note: undefined, domains: undefined },
    ]
    const { result } = renderHook(() => useEmailAliases(bindObserver))

    act(() => {
      lastObserver!.onAuthStateChanged({
        status: AuthenticationStatus.kAuthenticated,
        email: 'user@brave.com',
      })
    })
    act(() => {
      lastObserver!.onAliasesUpdated({ aliases } as AliasesUpdate)
    })
    act(() => {
      lastObserver!.onAuthStateChanged({
        status: AuthenticationStatus.kUnauthenticated,
        email: '',
      })
    })

    expect(result.current.aliasesUpdate).toEqual({ aliases: [] })
  })

  it('runs bindObserver cleanup on unmount', () => {
    const unbind = jest.fn()
    const { unmount } = renderHook(() =>
      useEmailAliases((_observer) => unbind),
    )

    unmount()
    expect(unbind).toHaveBeenCalledTimes(1)
  })
})
