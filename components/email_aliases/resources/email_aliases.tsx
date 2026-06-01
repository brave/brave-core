// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// This page contains a copy of Svelte (via @brave/leo) which will attempt
// to register its own trusted-types policy which duplicates the one registered
// by the main page.
import '$web-common/disableDuplicateSvelteTrustedPolicies'

import { createRoot } from 'react-dom/client'
import * as React from 'react'
import { SignInPage, ManagePage } from './content/email_aliases_manage_page'
import { StyleSheetManager } from 'styled-components'
import { setIconBasePath } from '@brave/leo/react/icon'
import {
  EmailAliasesMetrics,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverReceiver,
  EmailAliasesService,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import {
  useEmailAliases,
  useBraveAccountState,
  getLoggedInEmail,
  isAccountLoggedIn,
} from './content/use_email_aliases'

export const ManagePageConnected = ({
  authEmail,
  emailAliasesService,
  bindObserver,
  metrics,
}: {
  authEmail: string
  emailAliasesService: EmailAliasesServiceInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
  metrics?: ReturnType<typeof EmailAliasesMetrics.getRemote>
}) => {
  const { aliasesUpdate } = useEmailAliases(bindObserver)
  return (
    <ManagePage
      authEmail={authEmail}
      aliasesUpdate={aliasesUpdate}
      emailAliasesService={emailAliasesService}
      metrics={metrics}
    />
  )
}

export const EmailAliasesManagePage = ({
  emailAliasesService,
  bindObserver,
  metrics,
}: {
  emailAliasesService: EmailAliasesServiceInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
  metrics?: ReturnType<typeof EmailAliasesMetrics.getRemote>
}) => {
  const accountState = useBraveAccountState()
  if (!isAccountLoggedIn(accountState)) {
    return null
  }
  return (
    <ManagePageConnected
      authEmail={getLoggedInEmail(accountState)}
      emailAliasesService={emailAliasesService}
      bindObserver={bindObserver}
      metrics={metrics}
    />
  )
}

export const mount = (signInElem: HTMLElement, manageElem: HTMLElement) => {
  setIconBasePath('//resources/brave-icons')

  const emailAliasesService = EmailAliasesService.getRemote()
  const emailAliasesMetrics = EmailAliasesMetrics.getRemote()

  const bindObserver = (observer: EmailAliasesServiceObserverInterface) => {
    const observerReceiver = new EmailAliasesServiceObserverReceiver(observer)
    const observerRemote = observerReceiver.$.bindNewPipeAndPassRemote()
    emailAliasesService.addObserver(observerRemote)
    return () => {
      observerReceiver.$.close()
    }
  }

  const signInRoot = createRoot(signInElem)
  signInRoot.render(
    <StyleSheetManager target={signInElem.getRootNode() as ShadowRoot}>
      <SignInPage />
    </StyleSheetManager>,
  )

  const manageRoot = createRoot(manageElem)
  manageRoot.render(
    <StyleSheetManager target={manageElem.getRootNode() as ShadowRoot}>
      <EmailAliasesManagePage
        emailAliasesService={emailAliasesService}
        bindObserver={bindObserver}
        metrics={emailAliasesMetrics}
      />
    </StyleSheetManager>,
  )
}
