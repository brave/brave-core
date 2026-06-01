// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import { StubEmailAliasesService, demoData } from './utils/stubs'
import { ManagePageConnected } from '../email_aliases'
import { SignInPage } from '../content/email_aliases_manage_page'
import {
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import type { AccountState } from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'
import {
  installMockAuthentication,
  restoreMockAuthentication,
} from '../tests/mock_authentication'
import '../content/strings'

const loggedInState = {
  loggedIn: { email: demoData.email },
} as AccountState

const stubEmailAliasesServiceAccountReadyInstance =
  new StubEmailAliasesService()

const stubEmailAliasesServiceListErrorInstance = new StubEmailAliasesService(
  getLocale(S.SETTINGS_EMAIL_ALIASES_INFO_ERROR_MESSAGE),
)

const bindAccountReadyObserver = (
  observer: EmailAliasesServiceObserverInterface,
) => {
  stubEmailAliasesServiceAccountReadyInstance.addObserver(observer)
  return () => {} // Do nothing in this mock implementation.
}

const bindListErrorObserver = (
  observer: EmailAliasesServiceObserverInterface,
) => {
  stubEmailAliasesServiceListErrorInstance.addObserver(observer)
  return () => {}
}

function ManagePageConnectedStory({
  accountState,
  emailAliasesService,
  bindObserver,
}: {
  accountState: AccountState
  emailAliasesService: EmailAliasesServiceInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
}) {
  React.useEffect(() => {
    installMockAuthentication(accountState)
    return () => restoreMockAuthentication()
  }, [accountState])

  return (
    <ManagePageConnected
      emailAliasesService={emailAliasesService}
      bindObserver={bindObserver}
    />
  )
}

export const SignInPageStory = () => {
  return <SignInPage />
}

export const ManageAliasesPage = () => {
  return (
    <ManagePageConnectedStory
      accountState={loggedInState}
      emailAliasesService={stubEmailAliasesServiceAccountReadyInstance}
      bindObserver={bindAccountReadyObserver}
    />
  )
}

export const ManageAliasesPageListLoadError = () => {
  return (
    <ManagePageConnectedStory
      accountState={loggedInState}
      emailAliasesService={stubEmailAliasesServiceListErrorInstance}
      bindObserver={bindListErrorObserver}
    />
  )
}

export default {
  title: 'Email Aliases/ManagePageConnected',
  decorators: [
    (Story: any) => {
      return (
        <div style={{ width: '712px', margin: '0 auto' }}>
          <Story />
        </div>
      )
    },
  ],
}
