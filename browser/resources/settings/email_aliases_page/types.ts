// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export type Alias = {
  email: string,
  note?: string,
  domains?: string[]
}

export interface MappingService {
  createAlias(email: string, note: string): Promise<void>
  getAliases(): Promise<Alias[]>
  updateAlias(email: string, note: string, status: boolean): Promise<void>
  deleteAlias(email: string): Promise<void>
  generateAlias(): Promise<string>
  getAccountEmail (): Promise<string | undefined>
  requestAccount (accountEmail: string): Promise<void>
  getAccountState (): Promise<AccountState>
  onAccountReady (): Promise<boolean>
  cancelAccountRequest (): Promise<void>
  logout (): Promise<void>
  closeBubble (): Promise<void>
  fillField(fieldValue: string): Promise<void>
  showSettingsPage(): Promise<void>
}

export enum ViewMode {
  Startup,
  Main,
  Create,
  Edit,
  Delete,
  SignUp,
  AwaitingAuthorization
}

export enum AccountState {
  NoAccount,
  AwaitingAccount,
  AccountReady
}