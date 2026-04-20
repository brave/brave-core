// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createRoot } from 'react-dom/client'
import { ManagePage } from './content/email_aliases_manage_page'
import { StyleSheetManager } from 'styled-components'
import * as React from 'react'
import { setIconBasePath } from '@brave/leo/react/icon'
import {
  Alias,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverReceiver,
  EmailAliasesService,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

import type { AccountState } from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'
import {
  isAccountLoggedIn,
  useBraveAccountState,
} from './email_aliases_account_state'

function ManagePageConnectedBody({
  emailAliasesService,
  bindObserver,
  accountState,
}: {
  emailAliasesService: EmailAliasesServiceInterface
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
    <ManagePage
      accountState={accountState}
      aliasesState={aliasesState}
      emailAliasesService={emailAliasesService}
    />
  )
}

function ManagePageConnectedLive({
  emailAliasesService,
  bindObserver,
}: {
  emailAliasesService: EmailAliasesServiceInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
}) {
  const accountState = useBraveAccountState()
  return (
    <ManagePageConnectedBody
      emailAliasesService={emailAliasesService}
      bindObserver={bindObserver}
      accountState={accountState}
    />
  )
}

export const ManagePageConnected = ({
  emailAliasesService,
  bindObserver,
  accountStateOverride,
}: {
  emailAliasesService: EmailAliasesServiceInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
  /** For Storybook / tests: skip Authentication.getRemote() and use this state. */
  accountStateOverride?: AccountState | undefined
}) => {
  if (accountStateOverride !== undefined) {
    return (
      <ManagePageConnectedBody
        emailAliasesService={emailAliasesService}
        bindObserver={bindObserver}
        accountState={accountStateOverride}
      />
    )
  }
  return (
    <ManagePageConnectedLive
      emailAliasesService={emailAliasesService}
      bindObserver={bindObserver}
    />
  )
}

export const mount = (at: HTMLElement) => {
  setIconBasePath('//resources/brave-icons')

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
