// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { DialogProps } from '@brave/leo/react/dialog'
import * as React from 'react'
import { spacing } from '@brave/leo/tokens/css/variables'

// types
import { BraveWallet } from '../../../../../constants/types'

// utils
import { reduceAddress } from '../../../../../utils/reduce-address'

// components
import { CreateAccountIcon } from '../../../../../components/shared/create-account-icon/create-account-icon'

// styles
import { ContainerButton, Dialog, DialogTitle } from '../shared/style'
import {
  Column,
  Row
} from '../../../../../components/shared/style'
import { AccountAddress, AccountName } from './select_account.style'

interface SelectAccountProps extends DialogProps {
  accounts: BraveWallet.AccountInfo[]
  selectedAccount?: BraveWallet.AccountInfo
  onSelect: (account: BraveWallet.AccountInfo) => void
}

interface AccountProps {
  account: BraveWallet.AccountInfo
  onSelect: (account: BraveWallet.AccountInfo) => void
}

export const Account = ({ account, onSelect }: AccountProps) => {
  return (
    <ContainerButton
      onClick={() => onSelect(account)}
    >
      <Row
        gap={spacing.xl}
        width='100%'
        justifyContent='flex-start'
        alignItems='center'
      >
        <CreateAccountIcon
          account={account}
          size='medium'
        />
        <Column alignItems='flex-start'>
          <AccountName>{account.name}</AccountName>
          <AccountAddress>{reduceAddress(account.address)}</AccountAddress>
        </Column>
      </Row>
    </ContainerButton>
  )
}

export const SelectAccount = (props: SelectAccountProps) => {
  const { accounts, ...rest } = props

  return (
    <Dialog
      showClose
      {...rest}
    >
      <DialogTitle slot='title'>Select Account</DialogTitle>

      <Column width='100%'>
        {accounts.map((account) => (
          <Account
            key={account.accountId.uniqueKey}
            account={account}
            onSelect={props.onSelect}
          />
        ))}
      </Column>
    </Dialog>
  )
}
