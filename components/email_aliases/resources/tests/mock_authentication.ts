// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  Authentication,
  type AccountState,
  type AuthenticationObserverRemote,
} from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'

export function makeLoggedOutAccountState(): AccountState {
  return { loggedOut: {} } as AccountState
}

export function makeLoggedInAccountState(email: string): AccountState {
  return { loggedIn: { email } } as AccountState
}

export class MockAuthentication {
  private accountState: AccountState | undefined
  private listeners = new Set<(state: AccountState) => void>()

  constructor(initialState?: AccountState) {
    this.accountState = initialState
  }

  // Used by `useBraveAccountState` in tests (no Mojo required).
  subscribeAccountState(listener: (state: AccountState) => void): () => void {
    this.listeners.add(listener)
    if (this.accountState !== undefined) {
      listener(this.accountState)
    }
    return () => {
      this.listeners.delete(listener)
    }
  }

  addObserver(_observer: AuthenticationObserverRemote) {}

  setAccountState(state: AccountState) {
    this.accountState = state
    for (const listener of this.listeners) {
      listener(state)
    }
  }
}

type AuthenticationWithGetRemote = typeof Authentication & {
  getRemote: () => MockAuthentication
}

let savedGetRemote: (() => unknown) | undefined

export function installMockAuthentication(
  initialState?: AccountState,
): MockAuthentication {
  const mock = new MockAuthentication(initialState)
  if (typeof jest !== 'undefined') {
    jest
      .spyOn(Authentication, 'getRemote')
      .mockReturnValue(
        mock as unknown as ReturnType<typeof Authentication.getRemote>,
      )
  } else {
    const auth = Authentication as AuthenticationWithGetRemote
    if (savedGetRemote === undefined) {
      savedGetRemote = auth.getRemote
    }
    auth.getRemote = () => mock
  }
  return mock
}

export function restoreMockAuthentication() {
  if (typeof jest !== 'undefined') {
    jest.restoreAllMocks()
    return
  }
  const auth = Authentication as AuthenticationWithGetRemote
  if (savedGetRemote !== undefined) {
    auth.getRemote = savedGetRemote as AuthenticationWithGetRemote['getRemote']
    savedGetRemote = undefined
  }
}
