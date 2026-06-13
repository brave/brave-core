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

const emptyAliasesUpdate = {
  aliases: [],
  error: undefined,
}

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

export function useBraveAccountState() {
  const [accountState, setAccountState] = React.useState<
    AccountState | undefined
  >(undefined)

  React.useEffect(() => {
    const authentication = Authentication.getRemote()
    const router = new AuthenticationObserverCallbackRouter()
    authentication.addObserver(router.$.bindNewPipeAndPassRemote())
    const listenerId = router.onAccountStateChanged.addListener(setAccountState)
    return () => {
      router.removeListener(listenerId)
    }
  }, [])

  return accountState
}

/**
 * Subscribes to EmailAliasesService observer callbacks and exposes alias
 * updates (`AliasesUpdate`: list payload or load error). Pass a stable
 * `bindObserver` from the host (e.g. one created when the Mojo pipes are set
 * up). Callers should only mount this when the user is authenticated.
 */
export function useEmailAliases(
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void,
) {
  const [aliasesUpdate, setAliasesUpdate] =
    React.useState<AliasesUpdate>(emptyAliasesUpdate)

  React.useEffect(() => {
    const observer: EmailAliasesServiceObserverInterface = {
      onAliasesUpdated: setAliasesUpdate,
    }
    return bindObserver(observer)
  }, [])

  return { aliasesUpdate }
}
