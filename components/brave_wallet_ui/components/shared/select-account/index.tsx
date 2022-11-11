// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { UserAccountType, WalletAccountType } from '../../../constants/types'

// components
import { SelectAccountItem } from '../select-account-item/index'

export interface Props {
  accounts: WalletAccountType[]
  selectedAccount?: UserAccountType
  onSelectAccount: (account: UserAccountType) => () => void
  showTooltips?: boolean
}

export const SelectAccount = ({
  accounts,
  selectedAccount,
  onSelectAccount,
  showTooltips
}: Props) => {
  return (
    <>
      {accounts.map((account) =>
        <SelectAccountItem
          key={account.id}
          account={account}
          onSelectAccount={onSelectAccount(account)}
          selectedAccount={selectedAccount}
          showTooltips={showTooltips}
        />
      )}
    </>
  )
}

export default SelectAccount
