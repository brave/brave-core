// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createRoot } from 'react-dom/client'
import * as React from 'react'
import { StyleSheetManager } from 'styled-components'
import {
  EmailAliasModal,
  EmailAliasModalActionType,
  EmailAliasModalAction,
} from './content/email_aliases_modal'
import BraveCoreThemeProvider from '$web-common/BraveCoreThemeProvider'
import {
  AuthenticationStatus,
  AuthState,
  Alias,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverReceiver,
  EmailAliasesService,
  EmailAliasesPanelHandlerInterface,
  EmailAliasesPanelHandler,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

const EmailAliasesPanelConnected = ({
  emailAliasesService,
  emailAliasesPanelHandler,
  bindObserver,
}: {
  emailAliasesService: EmailAliasesServiceInterface
  emailAliasesPanelHandler: EmailAliasesPanelHandlerInterface
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
      onReturnToMain={(action: EmailAliasModalAction) => {
        switch (action.type) {
          case EmailAliasModalActionType.Cancel:
            emailAliasesPanelHandler.onCancel()
            break
          case EmailAliasModalActionType.Manage:
            emailAliasesPanelHandler.onManageAliases()
            break
          case EmailAliasModalActionType.Alias:
            emailAliasesPanelHandler.onAliasCreationComplete(action.email)
            break
        }
      }}
      editing={false}
      mainEmail={authState.email}
      aliasCount={aliasesState.length}
      emailAliasesService={emailAliasesService}
      bubble={true}
    />
  )
}

const mount = () => {
  const rootElement = document
    .getElementById('mountPoint')!
    .attachShadow({ mode: 'open' })
  const emailAliasesService = EmailAliasesService.getRemote()
  const emailAliasesPanelHandler = EmailAliasesPanelHandler.getRemote()
  const bindObserver = (observer: EmailAliasesServiceObserverInterface) => {
    const observerReceiver = new EmailAliasesServiceObserverReceiver(observer)
    const observerRemote = observerReceiver.$.bindNewPipeAndPassRemote()
    emailAliasesService.addObserver(observerRemote)
    return () => observerReceiver.$.close()
  }
  createRoot(rootElement).render(
    <StyleSheetManager target={rootElement.getRootNode() as ShadowRoot}>
      <BraveCoreThemeProvider>
        <EmailAliasesPanelConnected
          emailAliasesService={emailAliasesService}
          emailAliasesPanelHandler={emailAliasesPanelHandler}
          bindObserver={bindObserver}
        />
      </BraveCoreThemeProvider>
    </StyleSheetManager>,
  )
}

document.addEventListener('DOMContentLoaded', mount)
