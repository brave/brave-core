// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { font, typography } from '@brave/leo/tokens/css/variables'
import { Introduction } from './email_aliases_introduction'
import { MainView } from './email_aliases_main_view'
import * as React from 'react'
import Col from './styles/Col'
import styled from 'styled-components'
import {
  Alias,
  AuthState,
  AuthenticationStatus,
  EmailAliasesServiceInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

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

const BraveAccountSignIn = () => {
  const ref = React.useRef(null)
  return React.createElement('settings-brave-account-row', {
    ref,
  })
}

export const ManagePage = ({
  aliasesState,
  authState,
  emailAliasesService,
}: {
  aliasesState: Alias[]
  authState: AuthState
  emailAliasesService: EmailAliasesServiceInterface
}) => (
  <PageCol>
    <Introduction />
    <BraveAccountSignIn />
    {authState.status === AuthenticationStatus.kAuthenticated && (
      <MainView
        authState={authState}
        aliasesState={aliasesState}
        emailAliasesService={emailAliasesService}
      />
    )}
  </PageCol>
)
