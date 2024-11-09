// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Alias, MappingService } from './types'

export class RemoteMappingService implements MappingService {
  async getAccountEmail (): Promise<string | undefined> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve("account-email@gmail.com")
  }
  async logout (): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
  }
  async requestAccount (_accountEmail: string): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
  }
  async getAliases (): Promise<Alias[]> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve([
      { email: "mock-alias1@sandbox.bravealias.com", note: "Mock Alias 1", status: true },
      { email: "mock-alias2@sandbox.bravealias.com", note: "Mock Alias 2", status: false }
    ])
  }
  async createAlias (_email: string, _note: string): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
  }
  async updateAlias (_email: string, _note: string, _status: boolean): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
  }
  async deleteAlias (_email: string): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
  }
  generateAlias (): Promise<string> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve("mock-" + Math.random().toString().slice(2,6) + "@sandbox.bravealias.com")
  }
  async onAccountReady(): Promise<boolean> {
    return Promise.resolve(true)
  }
  cancelAccountRequest(): Promise<void> {
    return Promise.resolve()
  }
  async closeBubble(): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
  }
  async fillField(_fieldValue: string): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
  }
  async showSettingsPage(): Promise<void> {
    // TODO: remove this once we have a real implementation
    return Promise.resolve()
  }
}
