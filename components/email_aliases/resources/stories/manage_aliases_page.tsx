// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ManagePageConnected } from '../email_aliases'

import {
  EmailAliasesServiceObserverInterface,
  AuthenticationStatus,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

import { StubEmailAliasesService, demoData } from './utils/stubs'

const stubEmailAliasesServiceAccountReadyInstance = new StubEmailAliasesService(
  {
    status: AuthenticationStatus.kAuthenticated,
    email: demoData.email,
    errorMessage: undefined,
  },
)

const bindAccountReadyObserver = (
  observer: EmailAliasesServiceObserverInterface,
) => {
  stubEmailAliasesServiceAccountReadyInstance.addObserver(observer)
  return () => {} // Do nothing in this mock implementation.
}

export const ManageAliasesPage = () => {
  return (
    <ManagePageConnected
      emailAliasesService={stubEmailAliasesServiceAccountReadyInstance}
      bindObserver={bindAccountReadyObserver}
    />
  )
}

export default {
  title: 'Email Aliases/Manage Aliases Page',
}
