// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ManagePageConnected } from '../email_aliases'
import { EmailAliasModal } from '../content/email_aliases_modal'
import { getLocale } from '$web-common/locale'
import {
  Alias,
  AuthenticationStatus,
  AuthState,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverRemote,
  EmailAliasesService_UpdateAlias_ResponseParam_Result,
  EmailAliasesService_DeleteAlias_ResponseParam_Result,
  EmailAliasesService_GenerateAlias_ResponseParam_Result,
  EmailAliasesService_RequestAuthentication_ResponseParam_Result,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

export default {
  title: 'Email Aliases',
}

const demoData = {
  email: 'aguscr182@gmail.com',
  aliases: [
    {
      email: 'horse.radish.record57@bravealias.com',
      note: 'Alias for all my newsletters',
      domains: undefined,
    },
    {
      email: 'start.plane.division.laser42@bravealias.com',
      domains: ['x.com'],
      note: 'Twitter account',
    },
    {
      email: 'racoon.pencil.test14@bravealias.com',
      note: 'Marketplace email for Facebook',
      domains: undefined,
    },
  ],
} satisfies { email: string; aliases: Alias[] }

class StubEmailAliasesService implements EmailAliasesServiceInterface {
  aliases: Map<string, Alias>
  authState: AuthState
  accountRequestId: number
  observers: Set<
    EmailAliasesServiceObserverRemote | EmailAliasesServiceObserverInterface
  >

  constructor(authState: AuthState) {
    this.authState = authState
    this.observers = new Set<
      EmailAliasesServiceObserverRemote | EmailAliasesServiceObserverInterface
    >()
    this.aliases = new Map<string, Alias>()
    for (const alias of demoData.aliases) {
      this.aliases.set(alias.email, alias)
    }
  }

  addObserver(
    observer:
      | EmailAliasesServiceObserverRemote
      | EmailAliasesServiceObserverInterface,
  ) {
    this.observers.add(observer)
    observer.onAuthStateChanged(this.authState)
    observer.onAliasesUpdated([...this.aliases.values()])
  }

  async updateAlias(
    aliasEmail: string,
    note: string | null,
  ): Promise<{ result: EmailAliasesService_UpdateAlias_ResponseParam_Result }> {
    if (Math.random() < 1 / 3) {
      return Promise.reject(
        getLocale(S.SETTINGS_EMAIL_ALIASES_UPDATE_ALIAS_ERROR),
      )
    }
    const alias = { email: aliasEmail, note: note ?? '', domains: undefined }
    this.aliases.set(aliasEmail, alias)
    this.observers.forEach((observer) => {
      observer.onAliasesUpdated([...this.aliases.values()])
    })
    return { result: { success: {} as any, failure: undefined } }
  }

  async deleteAlias(
    aliasEmail: string,
  ): Promise<{ result: EmailAliasesService_DeleteAlias_ResponseParam_Result }> {
    if (Math.random() < 1 / 3) {
      return Promise.reject(
        getLocale(S.SETTINGS_EMAIL_ALIASES_DELETE_ALIAS_ERROR),
      )
    }
    this.aliases.delete(aliasEmail)
    this.observers.forEach((observer) => {
      observer.onAliasesUpdated([...this.aliases.values()])
    })
    return { result: { success: {} as any, failure: undefined } }
  }

  async generateAlias(): Promise<{
    result: EmailAliasesService_GenerateAlias_ResponseParam_Result
  }> {
    await new Promise((resolve) => setTimeout(resolve, 1000))
    if (Math.random() < 1 / 3) {
      return Promise.reject(getLocale(S.SETTINGS_EMAIL_ALIASES_GENERATE_ERROR))
    }
    let aliasEmail: string = ''
    do {
      aliasEmail =
        'mock-' + Math.random().toString().slice(2, 6) + '@bravealias.com'
    } while (this.aliases.has(aliasEmail))

    return { result: { success: aliasEmail, failure: undefined } }
  }

  async requestAuthentication(authEmail: string): Promise<{
    result: EmailAliasesService_RequestAuthentication_ResponseParam_Result
  }> {
    if (Math.random() < 1 / 3) {
      return Promise.reject(
        getLocale(S.SETTINGS_EMAIL_ALIASES_REQUEST_AUTHENTICATION_ERROR),
      )
    }
    this.observers.forEach((observer) => {
      observer.onAuthStateChanged({
        status: AuthenticationStatus.kAuthenticating,
        email: authEmail,
        errorMessage: undefined,
      })
    })
    this.accountRequestId = window.setTimeout(() => {
      this.observers.forEach((observer) => {
        observer.onAuthStateChanged({
          status: AuthenticationStatus.kAuthenticated,
          email: authEmail,
          errorMessage: undefined,
        })
      })
    }, 5000)
    return { result: { success: {} as any, failure: undefined } }
  }

  async cancelAuthenticationOrLogout() {
    window.clearTimeout(this.accountRequestId)
    this.observers.forEach((observer) => {
      observer.onAuthStateChanged({
        status: AuthenticationStatus.kUnauthenticated,
        email: '',
        errorMessage: undefined,
      })
    })
  }

  showSettingsPage() {
    // Do nothing in this mock implementation.
  }
}

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
      emailAliasesService={stubEmailAliasesServiceNoAccountInstance}
      bindObserver={bindNoAccountObserver}
    />
  )
}

export const SettingsPage = () => {
  return (
    <ManagePageConnected
      emailAliasesService={stubEmailAliasesServiceAccountReadyInstance}
      bindObserver={bindAccountReadyObserver}
    />
  )
}

export const Bubble = () => {
  return (
    <div style={{ width: '420px', margin: '0 auto' }}>
      <EmailAliasModal
        aliasCount={demoData.aliases.length}
        onReturnToMain={() => {}}
        editing={false}
        mainEmail={demoData.email}
        bubble={true}
        emailAliasesService={stubEmailAliasesServiceAccountReadyInstance}
      />
    </div>
  )
}
