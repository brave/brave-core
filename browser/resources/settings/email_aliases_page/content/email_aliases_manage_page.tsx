// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { font, typography } from '@brave/leo/tokens/css/variables'
import { Introduction } from './email_aliases_introduction'
import { MainEmailEntryForm } from './email_aliases_signin_page'
import { MainView } from './email_aliases_main_view'
import * as React from 'react'
import Col from './styles/Col'
import styled from 'styled-components'
import { Alias, AuthState, AuthenticationStatus, EmailAliasesServiceInterface }
  from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

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

export const ManagePage = ({ aliasesState, authState, emailAliasesService }: {
  aliasesState: Alias[],
  authState: AuthState,
  emailAliasesService: EmailAliasesServiceInterface
}) => (
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
