// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  AliasesUpdate,
  AuthenticationStatus,
  AuthState,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

const emptyAliasesUpdate = (): AliasesUpdate => ({
  aliases: [],
  error: undefined,
})

/**
 * Subscribes to EmailAliasesService observer callbacks and exposes auth state
 * and alias updates (`AliasesUpdate`: list payload or load error). Pass a
 * stable `bindObserver` from the host (e.g. one created when the Mojo pipes
 * are set up).
 */
export function useEmailAliases(
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void,
) {
  const [authState, setAuthState] = React.useState<AuthState>({
    status: AuthenticationStatus.kStartup,
    email: '',
  })
  const [aliasesUpdate, setAliasesUpdate] =
    React.useState<AliasesUpdate>(emptyAliasesUpdate)

  React.useEffect(() => {
    // Track auth locally so we do not apply alias updates while logged out.
    let status: AuthenticationStatus = AuthenticationStatus.kStartup
    const observer: EmailAliasesServiceObserverInterface = {
      onAliasesUpdated: (update: AliasesUpdate) => {
        if (status !== AuthenticationStatus.kAuthenticated) {
          return
        }
        setAliasesUpdate(update)
      },
      onAuthStateChanged: (state: AuthState) => {
        status = state.status
        setAuthState(state)
        if (status !== AuthenticationStatus.kAuthenticated) {
          setAliasesUpdate(emptyAliasesUpdate())
        }
      },
    }
    return bindObserver(observer)
  }, [])

  return { authState, aliasesUpdate }
}
