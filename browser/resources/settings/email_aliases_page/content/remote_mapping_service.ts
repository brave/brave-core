// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Alias, MappingService } from './types'
// TODO: uncomment this once we have a real implementation
//import { sendWithPromise } from 'chrome://resources/js/cr.js';

export class RemoteMappingService implements MappingService {
  //private pending_cancellation_ = false
  async getAccountEmail (): Promise<string | undefined> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve("account-email@gmail.com")
    // return sendWithPromise('email_aliases.getAccountEmail')
  }
  async logout (): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
    // await sendWithPromise('email_aliases.logout')
  }
  async requestAccount (_accountEmail: string): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
    // await sendWithPromise('email_aliases.requestAccount', accountEmail)
  }
  async getAliases (): Promise<Alias[]> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve([
      { email: "mock-alias1@sandbox.bravealias.com", note: "Mock Alias 1", status: true },
      { email: "mock-alias2@sandbox.bravealias.com", note: "Mock Alias 2", status: false }
    ])
    // const result = await sendWithPromise('email_aliases.getAliases')
    // return result
  }
  async createAlias (_email: string, _note: string): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
    // await sendWithPromise('email_aliases.createAlias', email, note)
  }
  async updateAlias (_email: string, _note: string, _status: boolean): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
    // await sendWithPromise('email_aliases.updateAlias', email, note, status)
  }
  async deleteAlias (_email: string): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
    // await sendWithPromise('email_aliases.deleteAlias', email)
  }
  generateAlias (): Promise<string> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve("mock-" + Math.random().toString().slice(2,6) + "@sandbox.bravealias.com")
    // return sendWithPromise('email_aliases.generateAlias')
  }
  async onAccountReady(): Promise<boolean> {
    return Promise.resolve(true)
    // TODO: uncomment this once we have a real implementation
    /*
    while (!this.pending_cancellation_) {
      try {
        await sendWithPromise('email_aliases.getSession')
        return true
      } catch (e) {
        console.error(e)
      }
    }
    return false
    */
  }
  cancelAccountRequest(): Promise<void> {
    //this.pending_cancellation_ = true
    return Promise.resolve()
  }
  async closeBubble(): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
    // await sendWithPromise('email_aliases.closeBubble')
  }
  async fillField(_fieldValue: string): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
    // await sendWithPromise('email_aliases.fillField', fieldValue)
  }
  async showSettingsPage(): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
    // await sendWithPromise('email_aliases.showSettingsPage')
  }
}
