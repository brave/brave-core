// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createRoot } from 'react-dom/client';
import { ManagePage } from './content/email_aliases_manage_page'
import { StyleSheetManager } from 'styled-components'
import * as React from 'react'
import {
  AuthenticationStatus,
  AuthState,
  Alias,
  EmailAliasesServiceInterface,
  EmailAliasesServiceRemote,
  EmailAliasesServiceObserverRemote,
  EmailAliasesServiceObserverInterface
} from "gen/brave/components/email_aliases/email_aliases.mojom.m"

export const ManagePageConnected = ({ emailAliasesService, bindObserver }: {
    emailAliasesService: EmailAliasesServiceInterface,
    bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
  }) => {
  const [authState, setAuthState] = React.useState<AuthState>(
      { status: AuthenticationStatus.kStartup, email: '' })
  const [aliasesState, setAliasesState] = React.useState<Alias[]>([]);
  React.useEffect(() => {
    const observer : EmailAliasesServiceObserverInterface = {
      onAliasesUpdated: (aliases: Alias[]) => {
        setAliasesState(aliases)
      },
      onAuthStateChanged: (state: AuthState) => {
        setAuthState(state)
      },
    }
    return bindObserver(observer)
  }, [] /* Only run at mount. */)
  return (
    <ManagePage
      authState={authState}
      aliasesState={aliasesState}
      emailAliasesService={emailAliasesService} />
  )
}

export const mount = (at: HTMLElement) => {
  const root = createRoot(at);
  const emailAliasesService = new EmailAliasesServiceRemote()
  const bindObserver = (observer: EmailAliasesServiceObserverInterface) => {
    const observerRemote = new EmailAliasesServiceObserverRemote(observer)
    emailAliasesService.addObserver(observerRemote)
    return () => emailAliasesService.removeObserver(observerRemote)
  }
  root.render(
    <StyleSheetManager target={at}>
      <ManagePageConnected
        emailAliasesService={emailAliasesService}
        bindObserver={bindObserver} />
    </StyleSheetManager>
  )
}

  ; (window as any).mountEmailAliases = mount
