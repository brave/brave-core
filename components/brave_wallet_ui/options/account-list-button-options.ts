// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { AccountButtonOptionsObjectType } from '../constants/types'

export const BuyButtonOption: AccountButtonOptionsObjectType = {
  id: 'buy',
  name: S.BRAVE_WALLET_BUY,
  icon: 'coins-alt1',
}

export const DepositButtonOption: AccountButtonOptionsObjectType = {
  id: 'deposit',
  name: S.BRAVE_WALLET_ACCOUNTS_DEPOSIT,
  icon: 'money-bag-coins',
}

export const AccountButtonOptions: AccountButtonOptionsObjectType[] = [
  {
    id: 'details',
    name: S.BRAVE_WALLET_ALLOW_SPEND_DETAILS_BUTTON,
    icon: 'info-outline',
  },
  {
    id: 'edit',
    name: S.BRAVE_WALLET_ALLOW_SPEND_EDIT_BUTTON,
    icon: 'edit-pencil',
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
  DepositButtonOption,
  {
    id: 'remove',
    name: S.BRAVE_WALLET_ACCOUNTS_REMOVE,
    icon: 'trash',
  },
  {
    id: 'shield',
    name: S.BRAVE_WALLET_SWITCH_TO_SHIELDED_ACCOUNT,
    icon: 'shield-done',
  },
  {
    id: 'resetBirthday',
    name: S.BRAVE_WALLET_RESET_SHIELDED_ACCOUNT_BIRTHDAY,
    icon: 'calendar',
  },
]
