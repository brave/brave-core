// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ManagePage, EmailAliasModal } from '../../../../../../../browser/resources/settings/email_aliases_page/email_aliases'
import { AccountState, Alias, MappingService, ViewMode } from '../../../../../../../browser/resources/settings/email_aliases_page/types'

import { provideStrings } from '../../../../../../../.storybook/locale'

provideStrings({
  emailAliasesShortDescription: 'Email Aliases',
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
  emailAliasesListTitle: 'Your email aliases',
  emailAliasesCreateAliasTitle: 'Create a new alias email',
  emailAliasesCreateAliasLabel: 'New alias',
  emailAliasesRefreshButtonTitle: 'Suggest another email alias',
  emailAliasesNoteLabel: 'Note',
  emailAliasesEditNotePlaceholder: 'Enter a note for your address (optional)',
  emailAliasesCancelButton: 'Cancel',
  emailAliasesManageButton: 'Manage',
  emailAliasesAliasLabel: 'Email alias',
  emailAliasesEmailsWillBeForwardedTo: 'Emails will be forwarded to $1',
  emailAliasesEditAliasTitle: 'Edit email alias',
  emailAliasesCreateAliasButton: 'Create',
  emailAliasesSaveAliasButton: 'Save',
  emailAliasesSignInOrCreateAccount: 'To get started, sign in or create a Brave account',
  emailAliasesEnterEmailToGetLoginLink: 'Enter your email address to get a secure login link sent to your email. Clicking this link will either create or access a Brave Account and let you use the free Email Aliases service.',
  emailAliasesGetLoginLinkButton: 'Get login link',
  emailAliasesEmailAddressPlaceholder: 'Email address',
  emailAliasesLoginEmailOnTheWay: 'A login email is on the way to $1',
  emailAliasesClickOnSecureLogin: 'Click on the secure login link in the email to access your account.',
  emailAliasesDontSeeEmail: 'Don\'t see the email? Check your spam folder or',
  emailAliasesTryAgain: 'try again.',
  emailAliasesBubbleDescription: 'Create a random email address that forwards to your inbox while keeping your personal email private.',
  emailAliasesBubbleLimitReached: 'You have reached the limit of 5 free email aliases. Click "Manage" to re-use or delete an alias.',
})

export default {
  title: 'Email Aliases',
}

const demoData = {
  email: 'aruiz@brave.com',
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
}

class MockMappingService implements MappingService {
  accountEmail_: string
  aliases_ : Map<string, Alias>
  accountState_ : AccountState = AccountState.NoAccount
  accountRequestId_ : number
  constructor() {
    this.aliases_ = new Map<string, Alias>();
    for (const alias of demoData.aliases) {
      this.aliases_.set(alias.email, alias)
    }
  }

  async createAlias (email: string, note: string): Promise<void> {
    const alias = { email, note }
    this.aliases_.set(email, alias)
  }

  async getAliases (): Promise<Alias[]> {
    return [...this.aliases_.values()]
  }

  async updateAlias (email: string, note: string, status: boolean): Promise<void> {
    const alias = { email, note }
    this.aliases_.set(email, alias)
  }

  async deleteAlias (email: string): Promise<void> {
    console.log("attempting to delete!!!")
    this.aliases_.delete(email)
  }

  async generateAlias (): Promise<string> {
    let generated: string = ''
    do {
      generated = "mock-" + Math.random().toString().slice(2,6) + "@bravealias.com"
    } while (this.aliases_.has(generated))
    return generated
  }

  async getAccountEmail (): Promise<string | undefined> {
    return this.accountEmail_
  }

  async requestAccount (accountEmail: string): Promise<void> {
    this.accountState_ = AccountState.AwaitingAccount
    this.accountRequestId_ = window.setTimeout(() => {
      this.accountEmail_ = accountEmail
      this.accountState_ = AccountState.AccountReady
    }, 5000);
  }

  async getAccountState (): Promise<AccountState> {
    return this.accountState_
  }

  async onAccountReady (): Promise<boolean> {
    while (this.accountState_ === AccountState.AwaitingAccount) {
      await new Promise(resolve => setTimeout(resolve, 250));
    }
    return this.accountState_ === AccountState.AccountReady
  }

  async cancelAccountRequest (): Promise<void> {
    this.accountState_ = AccountState.NoAccount
    window.clearTimeout(this.accountRequestId_)
  }

  async logout (): Promise<void> {
    this.accountState_ = AccountState.NoAccount
  }

  async closeBubble (): Promise<void> {
    console.log("closeBubble")
  }

  async fillField (fieldValue: string): Promise<void> {
    console.log("fillField", fieldValue)
  }

  async showSettingsPage (): Promise<void> {
    console.log("showSettingsPage")
  }
}

const mockMappingServiceSingleton = new MockMappingService()

export const SettingsPage = () => {
  return (
    <ManagePage mappingService={mockMappingServiceSingleton}></ManagePage>
  )
}

export const Bubble = () => {
  return (
    <EmailAliasModal
      returnToMain={() => {}}
      viewState={{ mode: ViewMode.Create }}
      email={demoData.email}
      bubble={true}
      mode={ViewMode.Create}
      mappingService={mockMappingServiceSingleton}
    />
  )
}
