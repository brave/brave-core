// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AliasList } from './email_aliases_list'
import * as React from 'react'

import {
  AuthState,
  Alias,
  EmailAliasesServiceInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

export const MainView = ({
  aliasesState,
  authState,
  emailAliasesService,
}: {
  authState: AuthState
  aliasesState: Alias[]
  emailAliasesService: EmailAliasesServiceInterface
}) => (
  <span>
    <AliasList
      aliases={aliasesState}
      authEmail={authState.email}
      emailAliasesService={emailAliasesService}
    />
  </span>
)
