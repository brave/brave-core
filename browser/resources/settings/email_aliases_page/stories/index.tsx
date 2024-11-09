// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ManagePage } from '../email_aliases'
import { EmailAliasModal } from '../content/email_aliases_modal'
import { Alias, MappingService } from '../content/types'

import { provideStrings } from '../../../../../.storybook/locale'

type AccountState = 'NoAccount' | 'AccountReady' | 'AwaitingAccount'

provideStrings({
  emailAliasesShortDescription: 'Keep your personal email address private',
  emailAliasesDescription: 'Create unique, random addresses that forward to your Brave account email and can be deleted at any time. Keep your actual email address from being disclosed or used by advertisers.',
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
  emailAliasesCreateDescription: 'Create up to 5 free email aliases to protect your real email address.',
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
  emailAliasesSignInOrCreateAccount: 'To get started, sign in or create a Brave account',
  emailAliasesEnterEmailToGetLoginLink: 'Enter your email address to get a secure login link sent to your email. Clicking this link will either create or access a Brave Account and let you use the free Email Aliases service.',
  emailAliasesGetLoginLinkButton: 'Get login link',
  emailAliasesEmailAddressPlaceholder: 'Email address',
  emailAliasesLoginEmailOnTheWay: 'A login email is on the way to $1',
  emailAliasesClickOnSecureLogin: 'Click on the secure login link in the email to access your account.',
  emailAliasesDontSeeEmail: 'Don\'t see the email? Check your spam folder or $1try again$2.',
  emailAliasesBubbleDescription: 'Create a random email address that forwards to your inbox while keeping your personal email private.',
  emailAliasesBubbleLimitReached: 'You have reached the limit of 5 free email aliases. Click "Manage" to re-use or delete an alias.',
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
    },
    {
      email: 'start.plane.division.laser42@bravealias.com',
      domains: ['x.com'],
      note: 'Twitter account'
    },
    {
      email: 'racoon.pencil.test14@bravealias.com',
      note: 'Marketplace email for Facebook'
    }
  ]
} satisfies { email: string, aliases: Alias[] }

class MockMappingService implements MappingService {
  accountEmail: string
  aliases: Map<string, Alias>
  accountState: AccountState
  accountRequestId: number
  constructor(accountState: AccountState, accountEmail: string) {
    this.accountState = accountState
    this.accountEmail = accountEmail
    this.aliases = new Map<string, Alias>();
    for (const alias of demoData.aliases) {
      this.aliases.set(alias.email, alias)
    }
  }

  async createAlias (email: string, note: string) {
    const alias = { email, note }
    this.aliases.set(email, alias)
  }

  async getAliases () {
    return [...this.aliases.values()]
  }

  async updateAlias (email: string, note: string, status: boolean) {
    const alias = { email, note }
    this.aliases.set(email, alias)
  }

  async deleteAlias (email: string) {
    this.aliases.delete(email)
  }

  async generateAlias () {
    let generated: string = ''
    do {
      generated = "mock-" + Math.random().toString().slice(2,6) + "@bravealias.com"
    } while (this.aliases.has(generated))
    await new Promise(resolve => setTimeout(resolve, 1000))
    return generated
  }

  async getAccountEmail () {
    return this.accountEmail
  }

  async requestAccount (accountEmail: string) {
    this.accountState = 'AwaitingAccount'
    this.accountRequestId = window.setTimeout(() => {
      this.accountEmail = accountEmail
      this.accountState = 'AccountReady'
    }, 5000);
  }

  async onAccountReady () {
    while (this.accountState === 'AwaitingAccount') {
      await new Promise(resolve => setTimeout(resolve, 250));
    }
    return this.accountState === 'AccountReady'
  }

  async cancelAccountRequest () {
    this.accountState = 'NoAccount'
    window.clearTimeout(this.accountRequestId)
  }

  async logout () {
    this.accountState = 'NoAccount'
  }

  async closeBubble () {
    console.log("closeBubble")
  }

  async fillField (fieldValue: string) {
    console.log("fillField", fieldValue)
  }

  async showSettingsPage () {
    console.log("showSettingsPage")
  }
}

const mockMappingServiceNoAccountInstance = new MockMappingService('NoAccount', '')
const mockMappingServiceAccountReadyInstance = new MockMappingService('AccountReady', demoData.email)

export const SignInPage = () => {
  return (
    <ManagePage mappingService={mockMappingServiceNoAccountInstance}></ManagePage>
  )
}

export const SettingsPage = () => {
  return (
    <ManagePage mappingService={mockMappingServiceAccountReadyInstance}></ManagePage>
  )
}

export const Bubble = () => {
  return (
    <EmailAliasModal
      onReturnToMain={() => {}}
      viewState={{ mode: 'Create' }}
      email={demoData.email}
      bubble={true}
      mode={'Create'}
      mappingService={mockMappingServiceAccountReadyInstance}
    />
  )
}
