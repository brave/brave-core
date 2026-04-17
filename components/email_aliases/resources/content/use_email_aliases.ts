// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  AccountState,
  Authentication,
  AuthenticationObserverCallbackRouter,
} from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'
import {
  AliasesUpdate,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

const emptyAliasesUpdate = (): AliasesUpdate => ({
  aliases: [],
  error: undefined,
})

export function isAccountLoggedIn(
  accountState: AccountState | undefined,
): boolean {
  return accountState !== undefined && accountState.loggedIn !== undefined
}

export function getLoggedInEmail(
  accountState: AccountState | undefined,
): string {
  if (!isAccountLoggedIn(accountState)) {
    return ''
  }
  return accountState!.loggedIn!.email
}

function useBraveAccountState(): AccountState | undefined {
  const [accountState, setAccountState] = React.useState<
    AccountState | undefined
  >(undefined)

  React.useEffect(() => {
    const authentication = Authentication.getRemote()
    const router = new AuthenticationObserverCallbackRouter()
    authentication.addObserver(router.$.bindNewPipeAndPassRemote())
    const listenerId = router.onAccountStateChanged.addListener(
      (state: AccountState) => {
        setAccountState(state)
      },
    )
    return () => {
      router.removeListener(listenerId)
    }
  }, [])

  return accountState
}

/**
 * Subscribes to Brave Account state and EmailAliasesService observer callbacks.
 * Exposes account state and alias updates (`AliasesUpdate`: list payload or load
 * error). Pass a stable `bindObserver` from the host (e.g. one created when
 * the Mojo pipes are set up).
 */
export function useEmailAliases(
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void,
) {
  const accountState = useBraveAccountState()

  const [aliasesUpdate, setAliasesUpdate] =
    React.useState<AliasesUpdate>(emptyAliasesUpdate())

  const accountStateRef = React.useRef(accountState)
  accountStateRef.current = accountState

  React.useEffect(() => {
    const observer: EmailAliasesServiceObserverInterface = {
      onAliasesUpdated: (update: AliasesUpdate) => {
        if (!isAccountLoggedIn(accountStateRef.current)) {
          return
        }
        setAliasesUpdate(update)
      },
    }
    return bindObserver(observer)
  }, [bindObserver])

  React.useEffect(() => {
    if (!isAccountLoggedIn(accountState)) {
      setAliasesUpdate(emptyAliasesUpdate())
    }
  }, [accountState])

  return { accountState, aliasesUpdate }
}
