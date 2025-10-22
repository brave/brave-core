// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createRoot } from 'react-dom/client'
import { ManagePage } from './content/email_aliases_manage_page'
import { StyleSheetManager } from 'styled-components'
import * as React from 'react'
import {
  AuthenticationStatus,
  AuthState,
  Alias,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverReceiver,
  EmailAliasesService,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

export const ManagePageConnected = ({
  emailAliasesService,
  bindObserver,
}: {
  emailAliasesService: EmailAliasesServiceInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
}) => {
  const [authState, setAuthState] = React.useState<AuthState>({
    status: AuthenticationStatus.kStartup,
    email: '',
    errorMessage: undefined,
  })
  const [aliasesState, setAliasesState] = React.useState<Alias[]>([])
  React.useEffect(() => {
    // Note: We keep track of the status here so we can avoid setting aliases
    // when the user is not logged in.
    let status: AuthenticationStatus = AuthenticationStatus.kStartup
    const observer: EmailAliasesServiceObserverInterface = {
      onAliasesUpdated: (aliases: Alias[]) => {
        if (status !== AuthenticationStatus.kAuthenticated) {
          return
        }
        setAliasesState(aliases)
      },
      onAuthStateChanged: (state: AuthState) => {
        status = state.status

        setAuthState(state)
        if (status !== AuthenticationStatus.kAuthenticated) {
          setAliasesState([])
        }
      },
    }
    return bindObserver(observer)
  }, [])
  return (
    <ManagePage
      authState={authState}
      aliasesState={aliasesState}
      emailAliasesService={emailAliasesService}
    />
  )
}

export const mount = (at: HTMLElement) => {
  const root = createRoot(at)
  const emailAliasesService = EmailAliasesService.getRemote()

  const bindObserver = (observer: EmailAliasesServiceObserverInterface) => {
    const observerReceiver = new EmailAliasesServiceObserverReceiver(observer)
    const observerRemote = observerReceiver.$.bindNewPipeAndPassRemote()
    emailAliasesService.addObserver(observerRemote)
    return () => {
      observerReceiver.$.close()
    }
  }
  root.render(
    <StyleSheetManager target={at.getRootNode() as ShadowRoot}>
      <ManagePageConnected
        emailAliasesService={emailAliasesService}
        bindObserver={bindObserver}
      />
    </StyleSheetManager>,
  )
}
