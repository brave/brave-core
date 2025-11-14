// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { StubEmailAliasesService, demoData } from './utils/stubs'
import { ManagePageConnected } from '../email_aliases'
import {
  AuthenticationStatus,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

const stubEmailAliasesServiceNoAccountInstance = new StubEmailAliasesService({
  status: AuthenticationStatus.kUnauthenticated,
  email: '',
  errorMessage: undefined,
})

const stubEmailAliasesServiceAccountReadyInstance = new StubEmailAliasesService(
  {
    status: AuthenticationStatus.kAuthenticated,
    email: demoData.email,
    errorMessage: undefined,
  },
)

const bindNoAccountObserver = (
  observer: EmailAliasesServiceObserverInterface,
) => {
  stubEmailAliasesServiceNoAccountInstance.addObserver(observer)
  return () => {} // Do nothing in this mock implementation.
}

const bindAccountReadyObserver = (
  observer: EmailAliasesServiceObserverInterface,
) => {
  stubEmailAliasesServiceAccountReadyInstance.addObserver(observer)
  return () => {} // Do nothing in this mock implementation.
}

export const SignInPage = () => {
  return (
    <ManagePageConnected
      // @ts-expect-error https://github.com/brave/brave-browser/issues/48960
      emailAliasesService={stubEmailAliasesServiceNoAccountInstance}
      bindObserver={bindNoAccountObserver}
    />
  )
}

export const ManageAliasesPage = () => {
  return (
    <ManagePageConnected
      // @ts-expect-error https://github.com/brave/brave-browser/issues/48960
      emailAliasesService={stubEmailAliasesServiceAccountReadyInstance}
      bindObserver={bindAccountReadyObserver}
    />
  )
}

export default {
  title: 'Email Aliases/ManagePageConnected',
}
