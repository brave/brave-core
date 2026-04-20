// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createRoot } from 'react-dom/client'
import * as React from 'react'
import { StyleSheetManager } from 'styled-components'
import { setIconBasePath } from '@brave/leo/react/icon'
import {
  EmailAliasModal,
  EmailAliasModalResultType,
  EmailAliasModalResult,
} from './content/email_aliases_modal'
import {
  Alias,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverReceiver,
  EmailAliasesService,
  EmailAliasesPanelHandlerInterface,
  EmailAliasesPanelHandler,
  MAX_ALIASES,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

import type { AccountState } from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'
import {
  getLoggedInEmail,
  isAccountLoggedIn,
  useBraveAccountState,
} from './email_aliases_account_state'

function EmailAliasesPanelConnectedBody({
  emailAliasesService,
  emailAliasesPanelHandler,
  bindObserver,
  accountState,
}: {
  emailAliasesService: EmailAliasesServiceInterface
  emailAliasesPanelHandler: EmailAliasesPanelHandlerInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
  accountState: AccountState | undefined
}) {
  const accountStateRef = React.useRef(accountState)
  accountStateRef.current = accountState

  const [aliasesState, setAliasesState] = React.useState<Alias[]>([])
  React.useEffect(() => {
    if (!isAccountLoggedIn(accountState)) {
      setAliasesState([])
    }
  }, [accountState])

  React.useEffect(() => {
    const observer: EmailAliasesServiceObserverInterface = {
      onAliasesUpdated: (aliases: Alias[]) => {
        if (!isAccountLoggedIn(accountStateRef.current)) {
          return
        }
        setAliasesState(aliases)
      },
    }
    return bindObserver(observer)
  }, [bindObserver])

  return (
    <EmailAliasModal
      aliases={aliasesState}
      aliasLimit={MAX_ALIASES}
      onReturnToMain={(action: EmailAliasModalResult) => {
        switch (action.type) {
          case EmailAliasModalResultType.Cancelled:
            emailAliasesPanelHandler.onCancelAliasCreation()
            break
          case EmailAliasModalResultType.ShouldManageAliases:
            emailAliasesPanelHandler.onManageAliases()
            break
          case EmailAliasModalResultType.AliasCreated:
            emailAliasesPanelHandler.onAliasCreated(action.email)
            break
        }
      }}
      editing={false}
      mainEmail={getLoggedInEmail(accountState)}
      emailAliasesService={emailAliasesService}
      bubble
    />
  )
}

function EmailAliasesPanelConnectedLive({
  emailAliasesService,
  emailAliasesPanelHandler,
  bindObserver,
}: {
  emailAliasesService: EmailAliasesServiceInterface
  emailAliasesPanelHandler: EmailAliasesPanelHandlerInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
}) {
  const accountState = useBraveAccountState()
  return (
    <EmailAliasesPanelConnectedBody
      emailAliasesService={emailAliasesService}
      emailAliasesPanelHandler={emailAliasesPanelHandler}
      bindObserver={bindObserver}
      accountState={accountState}
    />
  )
}

export const EmailAliasesPanelConnected = ({
  emailAliasesService,
  emailAliasesPanelHandler,
  bindObserver,
  accountStateOverride,
}: {
  emailAliasesService: EmailAliasesServiceInterface
  emailAliasesPanelHandler: EmailAliasesPanelHandlerInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
  accountStateOverride?: AccountState | undefined
}) => {
  if (accountStateOverride !== undefined) {
    return (
      <EmailAliasesPanelConnectedBody
        emailAliasesService={emailAliasesService}
        emailAliasesPanelHandler={emailAliasesPanelHandler}
        bindObserver={bindObserver}
        accountState={accountStateOverride}
      />
    )
  }
  return (
    <EmailAliasesPanelConnectedLive
      emailAliasesService={emailAliasesService}
      emailAliasesPanelHandler={emailAliasesPanelHandler}
      bindObserver={bindObserver}
    />
  )
}

const mount = () => {
  const rootElement = document.getElementById('mountPoint')!
  const emailAliasesService = EmailAliasesService.getRemote()
  const emailAliasesPanelHandler = EmailAliasesPanelHandler.getRemote()
  const bindObserver = (observer: EmailAliasesServiceObserverInterface) => {
    const observerReceiver = new EmailAliasesServiceObserverReceiver(observer)
    const observerRemote = observerReceiver.$.bindNewPipeAndPassRemote()
    emailAliasesService.addObserver(observerRemote)
    return () => observerReceiver.$.close()
  }
  setIconBasePath('//resources/brave-icons')
  createRoot(rootElement).render(
    <StyleSheetManager>
      <EmailAliasesPanelConnected
        emailAliasesService={emailAliasesService}
        emailAliasesPanelHandler={emailAliasesPanelHandler}
        bindObserver={bindObserver}
      />
    </StyleSheetManager>,
  )
}

document.addEventListener('DOMContentLoaded', mount)
