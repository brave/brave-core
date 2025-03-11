// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Alias, MappingService } from './types'
import { sendWithPromise } from 'chrome://resources/js/cr.js';

export class RemoteMappingService implements MappingService {
  private pending_cancellation_ = false
  async getAccountEmail (): Promise<string | undefined> {
    return sendWithPromise('email_aliases.getAccountEmail')
  }
  async logout (): Promise<void> {
    await sendWithPromise('email_aliases.logout')
  }
  async requestAccount (accountEmail: string): Promise<void> {
    await sendWithPromise('email_aliases.requestAccount', accountEmail)
  }
  async getAliases (): Promise<Alias[]> {
    const result = await sendWithPromise('email_aliases.getAliases')
    return result
  }
  async createAlias (email: string, note: string): Promise<void> {
    await sendWithPromise('email_aliases.createAlias', email, note)
  }
  async updateAlias (email: string, note: string, status: boolean): Promise<void> {
    await sendWithPromise('email_aliases.updateAlias', email, note, status)
  }
  async deleteAlias (email: string): Promise<void> {
    await sendWithPromise('email_aliases.deleteAlias', email)
  }
  /*
  async generateAlias (): Promise<string> {
    return sendWithPromise('email_aliases.generateAlias')
  }*/
  async generateAlias (): Promise<string> {
    return "mock-" + Math.random().toString().slice(2,6) + "@sandbox.bravealias.com"
  }
  async onAccountReady(): Promise<boolean> {
    this.pending_cancellation_ = false
    while (!this.pending_cancellation_) {
      try {
        await sendWithPromise('email_aliases.getSession')
        return true
      } catch (e) {
        console.error(e)
      }
    }
    return false
  }
  async cancelAccountRequest(): Promise<void> {
    this.pending_cancellation_ = true
  }
  async closeBubble(): Promise<void> {
    await sendWithPromise('email_aliases.closeBubble')
  }
  async fillField(fieldValue: string): Promise<void> {
    await sendWithPromise('email_aliases.fillField', fieldValue)
  }
  async showSettingsPage(): Promise<void> {
    await sendWithPromise('email_aliases.showSettingsPage')
  }
}
