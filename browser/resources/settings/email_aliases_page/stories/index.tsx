// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ManagePage } from '../email_aliases'
import { EmailAliasModal } from '../content/email_aliases_modal'
import {
  Alias,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverRemote
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import { provideStrings } from '../../../../../.storybook/locale'

type AccountState = 'NoAccount' | 'AccountReady' | 'AwaitingAccount'

provideStrings({
  emailAliasesShortDescription: 'Keep your personal email address private',
  emailAliasesDescription:
    'Create unique, random addresses that forward to your Brave account ' +
    'email and can be deleted at any time. Keep your actual email address ' +
    'from being disclosed or used by advertisers.',
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
  emailAliasesCreateDescription: 'Create up to 5 free email aliases to ' +
    'protect your real email address.',
  emailAliasesListTitle: 'Your Email aliases',
  emailAliasesCreateAliasTitle: 'New email alias',
  emailAliasesCreateAliasLabel: 'New alias',
  emailAliasesRefreshButtonTitle: 'Suggest another email alias',
  emailAliasesGeneratingNewAlias: 'Generating new alias...',
  emailAliasesNoteLabel: 'Note',
  emailAliasesEditNotePlaceholder: 'Enter a note for your address (optional)',
  emailAliasesCancelButton: 'Cancel',
  emailAliasesManageButton: 'Manage',
  emailAliasesAliasLabel: 'Email alias',
  emailAliasesEmailsWillBeForwardedTo: 'Emails will be forwarded to $1',
  emailAliasesEditAliasTitle: 'Edit email alias',
  emailAliasesCreateAliasButton: 'Create alias',
  emailAliasesSaveAliasButton: 'Save',
  emailAliasesSignInOrCreateAccount: 'To get started, sign in or create a ' +
    'Brave account',
  emailAliasesEnterEmailToGetLoginLink: 'Enter your email address to get a ' +
    'secure login link sent to your email. Clicking this link will either ' +
    'create or access a Brave Account and let you use the free Email Aliases ' +
    'service.',
  emailAliasesGetLoginLinkButton: 'Get login link',
  emailAliasesEmailAddressPlaceholder: 'Email address',
  emailAliasesLoginEmailOnTheWay: 'A login email is on the way to $1',
  emailAliasesClickOnSecureLogin: 'Click on the secure login link in the ' +
    'email to access your account.',
  emailAliasesDontSeeEmail: 'Don\'t see the email? Check your spam folder ' +
    'or $1try again$2.',
  emailAliasesBubbleDescription: 'Create a random email address that ' +
    'forwards to your inbox while keeping your personal email private.',
  emailAliasesBubbleLimitReached: 'You have reached the limit of 5 free ' +
    'email aliases. Click "Manage" to re-use or delete an alias.',
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
      domains: undefined
    },
    {
      email: 'start.plane.division.laser42@bravealias.com',
      domains: ['x.com'],
      note: 'Twitter account'
    },
    {
      email: 'racoon.pencil.test14@bravealias.com',
      note: 'Marketplace email for Facebook',
      domains: undefined
    }
  ]
} satisfies { email: string, aliases: Alias[] }

class MockEmailAliasesService implements EmailAliasesServiceInterface {
  accountEmail: string
  aliases: Map<string, Alias>
  accountState: AccountState
  accountRequestId: number
  observers: Set<EmailAliasesServiceObserverRemote |
                 EmailAliasesServiceObserverInterface>
  constructor(accountState: AccountState, accountEmail: string) {
    this.accountState = accountState
    this.accountEmail = accountEmail
    this.observers = new Set<EmailAliasesServiceObserverRemote |
                             EmailAliasesServiceObserverInterface>()
    this.aliases = new Map<string, Alias>();
    for (const alias of demoData.aliases) {
      this.aliases.set(alias.email, alias)
    }
  }

  addObserver (observer: EmailAliasesServiceObserverRemote |
                         EmailAliasesServiceObserverInterface) {
    this.observers.add(observer)
    switch (this.accountState) {
      case 'NoAccount':
        observer.onLoggedOut()
        break
      case 'AccountReady':
        observer.onLoggedIn(this.accountEmail)
        break
      case 'AwaitingAccount':
        observer.onVerificationPending(this.accountEmail)
        break
    }
    observer.onAliasesUpdated([...this.aliases.values()])
  }

  removeObserver (observer: EmailAliasesServiceObserverRemote |
                            EmailAliasesServiceObserverInterface) {
    this.observers.delete(observer)
  }

  updateAlias (email: string, note: string) {
    const alias = { email, note, domains: undefined }
    this.aliases.set(email, alias)
    this.observers.forEach(observer => {
      observer.onAliasesUpdated([...this.aliases.values()])
    })
  }

  deleteAlias (email: string) {
    this.aliases.delete(email)
    this.observers.forEach(observer => {
      observer.onAliasesUpdated([...this.aliases.values()])
    })
  }

  async generateAlias () {
    let generated: string = ''
    do {
      generated = "mock-" + Math.random().toString().slice(2,6) +
        "@bravealias.com"
    } while (this.aliases.has(generated))
    await new Promise(resolve => setTimeout(resolve, 1000))
    return { alias: generated }
  }

  requestPrimaryEmailVerification (email: string) {
    this.observers.forEach(observer => {
      observer.onVerificationPending(email)
    })
    this.accountRequestId = window.setTimeout(() => {
      this.observers.forEach(observer => {
        observer.onLoggedIn(email)
      })
    }, 5000);
  }

  cancelPrimaryEmailVerification () {
    window.clearTimeout(this.accountRequestId)
    this.observers.forEach(observer => {
      observer.onLoggedOut()
    })
  }

  logout () {
    this.observers.forEach(observer => {
      observer.onLoggedOut()
    })
  }

  showSettingsPage () {
    // Do nothing in this mock implementation.
  }

}

const mockEmailAliasesServiceNoAccountInstance =
  new MockEmailAliasesService('NoAccount', '')

const mockEmailAliasesServiceAccountReadyInstance =
  new MockEmailAliasesService('AccountReady', demoData.email)

const bindNoAccountObserver =
  (observer: EmailAliasesServiceObserverInterface) => {
    mockEmailAliasesServiceNoAccountInstance.addObserver(observer)
    return () => {
      mockEmailAliasesServiceNoAccountInstance.removeObserver(observer)
    }
  }

const bindAccountReadyObserver =
  (observer: EmailAliasesServiceObserverInterface) => {
    mockEmailAliasesServiceAccountReadyInstance.addObserver(observer)
    return () => {
      mockEmailAliasesServiceAccountReadyInstance.removeObserver(observer)
    }
  }

export const SignInPage = () => {
  return (
    <ManagePage emailAliasesService={mockEmailAliasesServiceNoAccountInstance}
                bindObserver={bindNoAccountObserver}>
    </ManagePage>
  )
}

export const SettingsPage = () => {
  return (
    <ManagePage
      emailAliasesService={mockEmailAliasesServiceAccountReadyInstance}
      bindObserver={bindAccountReadyObserver}>
    </ManagePage>
  )
}

export const Bubble = () => {
  return (
    <EmailAliasModal
      aliasCount={demoData.aliases.length}
      onReturnToMain={() => {}}
      viewState={{ mode: 'Create' }}
      mainEmail={demoData.email}
      bubble={true}
      emailAliasesService={mockEmailAliasesServiceAccountReadyInstance}
    />
  )
}
