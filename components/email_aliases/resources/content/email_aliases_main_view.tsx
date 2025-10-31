// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AliasList } from './email_aliases_list'
import { font, spacing } from '@brave/leo/tokens/css/variables'
import { getLocale } from '$web-common/locale'
import { MainEmailDisplay } from './email_aliases_main_email_display'
import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'
import Row from './styles/Row'
import styled from 'styled-components'

import {
  AuthenticationStatus,
  AuthState,
  Alias,
  EmailAliasesServiceInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

const SpacedRow = styled(Row)`
  gap: ${spacing.m};
  justify-content: center;
  align-items: center;
  font: ${font.default.semibold};
`

export const MainView = ({
  aliasesState,
  authState,
  emailAliasesService,
}: {
  authState: AuthState
  aliasesState: Alias[]
  emailAliasesService: EmailAliasesServiceInterface
}) =>
  authState.status === AuthenticationStatus.kStartup ? (
    <SpacedRow>
      <ProgressRing />
      <div>
        {getLocale(S.SETTINGS_EMAIL_ALIASES_CONNECTING_TO_BRAVE_ACCOUNT)}
      </div>
    </SpacedRow>
  ) : (
    <span>
      <MainEmailDisplay
        email={authState.email}
        emailAliasesService={emailAliasesService}
      />
      <AliasList
        aliases={aliasesState}
        authEmail={authState.email}
        emailAliasesService={emailAliasesService}
      />
    </span>
  )
