// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AccountButtonOptionsObjectType } from '../constants/types'

export const AccountDetailsMenuOptions: AccountButtonOptionsObjectType[] = [
  {
    id: 'edit',
    name: 'braveWalletAllowSpendEditButton',
    icon: 'edit-pencil'
  },
  {
    id: 'deposit',
    name: 'braveWalletAccountsDeposit',
    icon: 'qr-code'
  },
  {
    id: 'privateKey',
    name: 'braveWalletAccountsExport',
    icon: 'key'
  },
  {
    id: 'remove',
    name: 'braveWalletAccountsRemove',
    icon: 'trash'
  }
]
