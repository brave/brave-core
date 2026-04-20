// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { StubEmailAliasesService, demoData } from './utils/stubs'
import { ManagePageConnected } from '../email_aliases'
import { EmailAliasesServiceObserverInterface } from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import type { AccountState } from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'

const stubEmailAliasesServiceNoAccountInstance = new StubEmailAliasesService()

const stubEmailAliasesServiceAccountReadyInstance =
  new StubEmailAliasesService()

const loggedOutState = { loggedOut: {} } as AccountState

const loggedInState = {
  loggedIn: { email: demoData.email },
} as AccountState

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
      accountStateOverride={loggedOutState}
    />
  )
}

export const ManageAliasesPage = () => {
  return (
    <ManagePageConnected
      // @ts-expect-error https://github.com/brave/brave-browser/issues/48960
      emailAliasesService={stubEmailAliasesServiceAccountReadyInstance}
      bindObserver={bindAccountReadyObserver}
      accountStateOverride={loggedInState}
    />
  )
}

export default {
  title: 'Email Aliases/ManagePageConnected',
}
