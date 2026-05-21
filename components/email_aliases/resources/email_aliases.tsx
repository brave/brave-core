// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createRoot } from 'react-dom/client'
import { SignInPage, ManagePage } from './content/email_aliases_manage_page'
import { StyleSheetManager } from 'styled-components'
import * as React from 'react'
import { setIconBasePath } from '@brave/leo/react/icon'
import {
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverReceiver,
  EmailAliasesService,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import { useEmailAliases } from './content/use_email_aliases'

export const ManagePageConnected = ({
  emailAliasesService,
  bindObserver,
}: {
  emailAliasesService: EmailAliasesServiceInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
}) => {
  const { authState, aliasesUpdate } = useEmailAliases(bindObserver)
  return (
    <ManagePage
      authState={authState}
      aliasesUpdate={aliasesUpdate}
      emailAliasesService={emailAliasesService}
    />
  )
}

export const mount = (signInElem: HTMLElement, manageElem: HTMLElement) => {
  setIconBasePath('//resources/brave-icons')

  const emailAliasesService = EmailAliasesService.getRemote()

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
      <ManagePageConnected
        emailAliasesService={emailAliasesService}
        bindObserver={bindObserver}
      />
    </StyleSheetManager>,
  )
}
