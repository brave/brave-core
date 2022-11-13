// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { AccountButtonOptionsObjectType } from '../constants/types'

// Icons
import QRIcon from '../assets/svg-icons/qr-code-icon.svg'
import KeyIcon from '../assets/svg-icons/key-icon.svg'
import PencilIcon from '../assets/svg-icons/pencil-icon.svg'
import DetailsIcon from '../assets/svg-icons/details-icon.svg'
import TrashIcon from '../assets/svg-icons/trash-icon.svg'

export const BuyButtonOption: AccountButtonOptionsObjectType = {
  id: 'buy',
  name: 'braveWalletBuy',
  icon: ''
}

export const DepositButtonOption: AccountButtonOptionsObjectType = {
  id: 'deposit',
  name: 'braveWalletAccountsDeposit',
  icon: QRIcon
}

export const AccountButtonOptions: AccountButtonOptionsObjectType[] = [
  {
    id: 'privateKey',
    name: 'braveWalletAccountsExport',
    icon: KeyIcon
  },
  DepositButtonOption,
  {
    id: 'edit',
    name: 'braveWalletAllowSpendEditButton',
    icon: PencilIcon
  },
  {
    id: 'details',
    name: 'braveWalletAllowSpendDetailsButton',
    icon: DetailsIcon
  },
  {
    id: 'remove',
    name: 'braveWalletAccountsRemove',
    icon: TrashIcon
  }
]
