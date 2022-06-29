// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Types
import { AccountButtonOptionsObjectType } from '../constants/types'

// Icons
import QRIcon from '../assets/svg-icons/qr-code-icon.svg'
import KeyIcon from '../assets/svg-icons/key-icon.svg'
import PencilIcon from '../assets/svg-icons/pencil-icon.svg'
import DetailsIcon from '../assets/svg-icons/details-icon.svg'
import TrashIcon from '../assets/svg-icons/trash-icon.svg'

// Utils
import { getLocale } from '../../common/locale'

export const AccountButtonOptions: AccountButtonOptionsObjectType[] = [
  {
    id: 'export',
    name: getLocale('braveWalletAccountsExport'),
    icon: KeyIcon
  },
  {
    id: 'deposit',
    name: getLocale('braveWalletAccountsDeposit'),
    icon: QRIcon
  },
  {
    id: 'edit',
    name: getLocale('braveWalletAllowSpendEditButton'),
    icon: PencilIcon
  },
  {
    id: 'details',
    name: getLocale('braveWalletAllowSpendDetailsButton'),
    icon: DetailsIcon
  },
  {
    id: 'remove',
    name: getLocale('braveWalletAccountsRemove'),
    icon: TrashIcon
  }
]
