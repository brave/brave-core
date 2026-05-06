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
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverReceiver,
  EmailAliasesService,
  EmailAliasesPanelHandlerInterface,
  EmailAliasesPanelHandler,
  MAX_ALIASES,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import { useEmailAliasesObserver } from './content/use_email_aliases_observer'

export const EmailAliasesPanelConnected = ({
  emailAliasesService,
  emailAliasesPanelHandler,
  bindObserver,
}: {
  emailAliasesService: EmailAliasesServiceInterface
  emailAliasesPanelHandler: EmailAliasesPanelHandlerInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
}) => {
  const { authState, aliasesUpdate } = useEmailAliasesObserver(bindObserver)
  const aliases = aliasesUpdate.error ? [] : (aliasesUpdate.aliases ?? [])
  return (
    <EmailAliasModal
      aliases={aliases}
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
      mainEmail={authState.email}
      emailAliasesService={emailAliasesService}
      bubble
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
