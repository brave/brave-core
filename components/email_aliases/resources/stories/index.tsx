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
import { provideStrings } from '../../../../.storybook/locale'

provideStrings({
  emailAliasesShortDescription: 'Keep your personal email address private',
  emailAliasesDescription:
    'Create unique, random addresses that forward to your Brave account '
    + 'email and can be deleted at any time. Keep your actual email address '
    + 'from being disclosed or used by advertisers.',
  emailAliasesLearnMore: 'Learn More',
  emailAliasesSignOut: 'Sign Out',
  emailAliasesSignOutTitle: 'Sign Out of Email Aliases',
  emailAliasesConnectingToBraveAccount: 'Connecting to Brave Account...',
  emailAliasesBraveAccount: 'Brave Account',
  emailAliasesCopiedToClipboard: 'Copied to clipboard',
  emailAliasesClickToCopyAlias: 'Click to copy alias',
  emailAliasesUsedBy: 'Used by $1',
  emailAliasesEdit: 'Edit',
  emailAliasesDelete: 'Delete',
  emailAliasesCreateDescription:
    'Create up to 5 free email aliases to '
    + 'protect your real email address.',
  emailAliasesListTitle: 'Your Email aliases',
  emailAliasesCreateAliasTitle: 'New email alias',
  emailAliasesCreateAliasLabel: 'New alias',
  emailAliasesRefreshButtonTitle: 'Suggest another email alias',
  emailAliasesGeneratingNewAlias: 'Generating new alias...',
  emailAliasesGenerateError:
    'Error generating alias. Check your internet '
    + 'connection and try again.',
  emailAliasesNoteLabel: 'Note',
  emailAliasesEditNotePlaceholder: 'Enter a note for your address (optional)',
  emailAliasesCancelButton: 'Cancel',
  emailAliasesManageButton: 'Manage',
  emailAliasesAliasLabel: 'Email alias',
  emailAliasesEmailsWillBeForwardedTo: 'Emails will be forwarded to $1',
  emailAliasesEditAliasTitle: 'Edit email alias',
  emailAliasesCreateAliasButton: 'Create alias',
  emailAliasesUpdateAliasError:
    'Error saving alias. Check your internet ' + 'connection and try again.',
  emailAliasesSaveAliasButton: 'Save',
  emailAliasesDeleteAliasTitle: 'Delete email alias',
  emailAliasesDeleteAliasDescription:
    'Removing $1 as an alias will result '
    + 'in the loss of your ability to receive emails.',
  emailAliasesDeleteWarning:
    'This action is irreversible. Please ensure you '
    + 'want to proceed before continuing.',
  emailAliasesDeleteAliasButton: 'Delete',
  emailAliasesDeleteAliasError:
    'Error deleting alias. Check your internet ' + 'connection and try again.',
  emailAliasesSignInOrCreateAccount:
    'To get started, sign in or create a ' + 'Brave account',
  emailAliasesEnterEmailToGetLoginLink:
    'Enter your email address to get a '
    + 'secure login link sent to your email. Clicking this link will either '
    + 'create or access a Brave Account and let you use the free Email Aliases '
    + 'service.',
  emailAliasesGetLoginLinkButton: 'Get login link',
  emailAliasesRequestAuthenticationError:
    'Error requesting authentication. Check your internet connection and try '
    + 'again.',
  emailAliasesEmailAddressPlaceholder: 'Email address',
  emailAliasesLoginEmailOnTheWay: 'A login email is on the way to $1',
  emailAliasesClickOnSecureLogin:
    'Click on the secure login link in the ' + 'email to access your account.',
  emailAliasesDontSeeEmail:
    "Don't see the email? Check your spam folder " + 'or try again.',
  emailAliasesAuthError:
    'Error authenticating with Brave Account. ' + 'Please try again.',
  emailAliasesAuthTryAgainButton: 'Start over',
  emailAliasesBubbleDescription:
    'Create a random email address that '
    + 'forwards to your inbox while keeping your personal email private.',
  emailAliasesBubbleLimitReached:
    'You have reached the limit of 5 free '
    + 'email aliases. Click "Manage" to re-use or delete an alias.',
})

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
      throw new Error(getLocale('emailAliasesUpdateAliasError'))
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
      throw new Error(getLocale('emailAliasesDeleteAliasError'))
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
      throw new Error(getLocale('emailAliasesGenerateError'))
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
      throw new Error(getLocale('emailAliasesRequestAuthenticationError'))
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
    <EmailAliasModal
      aliasCount={demoData.aliases.length}
      onReturnToMain={() => {}}
      editing={false}
      mainEmail={demoData.email}
      bubble={true}
      emailAliasesService={stubEmailAliasesServiceAccountReadyInstance}
    />
  )
}
