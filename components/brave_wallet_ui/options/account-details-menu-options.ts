// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AccountButtonOptionsObjectType } from '../constants/types'

export const AccountDetailsMenuOptions: AccountButtonOptionsObjectType[] = [
  {
    id: 'edit',
    name: S.BRAVE_WALLET_ALLOW_SPEND_EDIT_BUTTON,
    icon: 'edit-pencil',
  },
  {
    id: 'explorer',
    name: S.BRAVE_WALLET_TRANSACTION_EXPLORER,
    icon: 'web3-blockexplorer',
  },
  {
    id: 'deposit',
    name: S.BRAVE_WALLET_ACCOUNTS_DEPOSIT,
    icon: 'money-bag-coins',
  },
  {
    id: 'privateKey',
    name: S.BRAVE_WALLET_ACCOUNTS_EXPORT,
    icon: 'key',
  },
  {
    id: 'hide',
    name: S.BRAVE_WALLET_ACCOUNTS_HIDE,
    icon: 'eye-off',
  },
  {
    id: 'remove',
    name: S.BRAVE_WALLET_ACCOUNTS_REMOVE,
    icon: 'trash',
  },
  {
    id: 'resetBirthday',
    name: S.BRAVE_WALLET_RESET_SHIELDED_ACCOUNT_BIRTHDAY,
    icon: 'calendar',
  },
]
