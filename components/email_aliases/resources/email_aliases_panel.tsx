// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createRoot } from 'react-dom/client'
import * as React from 'react'
import { StyleSheetManager } from 'styled-components'
import { EmailAliasModal } from './content/email_aliases_modal'
import {
  AuthenticationStatus,
  AuthState,
  Alias,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverReceiver,
  EmailAliasesService,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

const rootEl = document!.getElementById('mountPoint')!
const shadow = rootEl.attachShadow({ mode: 'open' })

const EmailAliasesPanelConnected = ({
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
    <EmailAliasModal
      onReturnToMain={(chosenEmail) => {
        emailAliasesService.notifyAliasCreationComplete(chosenEmail ?? null)
      }}
      editing={false}
      mainEmail={authState.email}
      aliasCount={aliasesState.length}
      emailAliasesService={emailAliasesService}
      bubble={true}
    />
  )
}

const mount = (at: ShadowRoot) => {
  const mountPoint = document.createElement('div')
  at.appendChild(mountPoint)
  const root = createRoot(mountPoint)
  const emailAliasesService = EmailAliasesService.getRemote()
  const bindObserver = (observer: EmailAliasesServiceObserverInterface) => {
    const observerReceiver = new EmailAliasesServiceObserverReceiver(observer)
    const observerRemote = observerReceiver.$.bindNewPipeAndPassRemote()
    emailAliasesService.addObserver(observerRemote)
    return () => observerReceiver.$.close()
  }
  root.render(
    <StyleSheetManager target={at}>
      <EmailAliasesPanelConnected
        emailAliasesService={emailAliasesService}
        bindObserver={bindObserver}
      />
    </StyleSheetManager>,
  )
}

mount(shadow)
