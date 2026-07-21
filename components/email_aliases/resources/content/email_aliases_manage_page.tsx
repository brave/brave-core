// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { font, typography } from '@brave/leo/tokens/css/variables'
import { Introduction } from './email_aliases_introduction'
import { AliasList } from './email_aliases_list'
import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Col from './styles/Col'
import styled from 'styled-components'
import {
  AliasesUpdate,
  EmailAliasesMetricsRemote,
  EmailAliasesServiceInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import { useBraveAccountState, isAccountLoggedIn } from './use_email_aliases'

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
  return React.createElement('settings-brave-account-row', {
    'initiating-service-name': 'email-aliases',
  })
}

const AutofillSuggestionToggle = ({ prefs }: { prefs: object }) => {
  return React.createElement('settings-email-aliases-autofill-toggle', {
    prefs,
  })
}

export const SignInPage = ({ prefs }: { prefs: object }) => {
  const accountState = useBraveAccountState()
  return (
    <PageCol>
      <Introduction />
      <BraveAccountSignIn />
      {isAccountLoggedIn(accountState) && (
        <AutofillSuggestionToggle prefs={prefs} />
      )}
    </PageCol>
  )
}

export const ManagePage = ({
  aliasesUpdate,
  authEmail,
  emailAliasesService,
  metrics,
}: {
  aliasesUpdate: AliasesUpdate
  authEmail: string
  emailAliasesService: EmailAliasesServiceInterface
  metrics?: EmailAliasesMetricsRemote
}) => (
  <PageCol>
    {aliasesUpdate.error ? (
      <Alert type='error'>{aliasesUpdate.error}</Alert>
    ) : (
      <AliasList
        aliases={aliasesUpdate.aliases!}
        authEmail={authEmail}
        emailAliasesService={emailAliasesService}
        metrics={metrics}
      />
    )}
  </PageCol>
)
