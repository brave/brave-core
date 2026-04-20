// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  AccountState,
  Authentication,
  AuthenticationObserverCallbackRouter,
} from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'

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

export function useBraveAccountState(): AccountState | undefined {
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
