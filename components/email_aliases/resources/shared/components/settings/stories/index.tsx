/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { ManagePage, EmailAliasModal } from '../../../../../../../browser/resources/settings/email_aliases_page/email_aliases'
import { AccountState, Alias, MappingService, ViewMode } from '../../../../../../../browser/resources/settings/email_aliases_page/types'

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
      mode={ViewMode.Create}
      mappingService={mockMappingServiceSingleton}
    />
  )
}
