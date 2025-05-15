// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createRoot } from 'react-dom/client';
import { getLocale } from '$web-common/locale'
import { MainEmailEntryForm } from './content/email_aliases_signin_page'
import { spacing, font, radius, typography }
  from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import Card from './content/styles/Card'
import Col from './content/styles/Col'
import { MainView } from './content/email_aliases_main_view'

import SecureLink from '$web-common/SecureLink'
import styled, { StyleSheetManager } from 'styled-components'
import {
  AuthenticationStatus,
  AuthState,
  Alias,
  EmailAliasesServiceInterface,
  EmailAliasesServiceRemote,
  EmailAliasesServiceObserverRemote,
  EmailAliasesServiceObserverInterface
} from "gen/brave/components/email_aliases/email_aliases.mojom.m"

const PageCol = styled(Col)`
  font: ${font.default.regular};
  padding: 0;
  margin: 0;
  & h4 {
    font: ${font.heading.h4};
    line-height: ${typography.heading.h4.lineHeight};
    margin: 0;
  }
`

const SectionTitle = styled(Card)`
  border-radius: ${radius.m};
  padding: ${spacing['2Xl']} ${spacing['2Xl']} ${spacing.l} ${spacing['2Xl']};
  display: flex;
  flex-direction: column;
  row-gap: ${spacing.m};
`

const Introduction = () =>
  <SectionTitle>
      <div>
      <h4>{getLocale('emailAliasesShortDescription')}</h4>
      </div>
      <div>
        {getLocale('emailAliasesDescription')}  {
           /* TODO(https://github.com/brave/brave-browser/issues/45408):
           // Link to the email aliases support page */}
        <SecureLink href="https://support.brave.com" target='_blank'>
          {getLocale('emailAliasesLearnMore')}
        </SecureLink>
      </div>
  </SectionTitle>

export const ManagePage = ({ emailAliasesService, bindObserver }:
  {
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
    <PageCol>
      <Introduction />
      {authState.status === AuthenticationStatus.kUnauthenticated ||
       authState.status === AuthenticationStatus.kAuthenticating
        ? <MainEmailEntryForm
            authState={authState}
            emailAliasesService={emailAliasesService} />
        : <MainView
            authState={authState}
            aliasesState={aliasesState}
            emailAliasesService={emailAliasesService} />}
    </PageCol>
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
      <ManagePage emailAliasesService={emailAliasesService}
                  bindObserver={bindObserver} />
    </StyleSheetManager>
  )
}

  ; (window as any).mountEmailAliases = mount
