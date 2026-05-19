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
  AuthenticationStatus,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import '../content/strings'

const stubEmailAliasesServiceAccountReadyInstance = new StubEmailAliasesService(
  {
    status: AuthenticationStatus.kAuthenticated,
    email: demoData.email,
  },
)

const stubEmailAliasesServiceListErrorInstance = new StubEmailAliasesService(
  {
    status: AuthenticationStatus.kAuthenticated,
    email: demoData.email,
  },
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

export const SignInPageStory = () => {
  return <SignInPage />
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

export const ManageAliasesPageListLoadError = () => {
  return (
    <ManagePageConnected
      // @ts-expect-error https://github.com/brave/brave-browser/issues/48960
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
