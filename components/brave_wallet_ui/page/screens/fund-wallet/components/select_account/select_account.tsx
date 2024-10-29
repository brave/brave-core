// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { DialogProps } from '@brave/leo/react/dialog'
import { spacing } from '@brave/leo/tokens/css/variables'

// Selectors
import {
  useSafeUISelector //
} from '../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../common/selectors'

// Types
import { BraveWallet, MeldCryptoCurrency } from '../../../../../constants/types'

// Utils
import { reduceAddress } from '../../../../../utils/reduce-address'
import { getLocale } from '../../../../../../common/locale'

// Components
import {
  CreateAccountIcon //
} from '../../../../../components/shared/create-account-icon/create-account-icon'
import {
  BottomSheet //
} from '../../../../../components/shared/bottom_sheet/bottom_sheet'

// Styled Components
import { ContainerButton, Dialog, DialogTitle } from '../shared/style'
import {
  Column,
  Row,
  ScrollableColumn
} from '../../../../../components/shared/style'
import { AccountAddress, AccountName } from './select_account.style'
import { getMeldTokensCoinType } from '../../../../../utils/meld_utils'

const testnetAccountKeyringIds = [
  BraveWallet.KeyringId.kBitcoin84Testnet,
  BraveWallet.KeyringId.kBitcoinHardwareTestnet,
  BraveWallet.KeyringId.kBitcoinImportTestnet,
  BraveWallet.KeyringId.kFilecoinTestnet,
  BraveWallet.KeyringId.kZCashTestnet
]

interface SelectAccountProps extends DialogProps {
  accounts: BraveWallet.AccountInfo[]
  selectedAccount?: BraveWallet.AccountInfo
  selectedAsset?: MeldCryptoCurrency
  isOpen: boolean
  onClose: () => void
  onSelect: (account: BraveWallet.AccountInfo) => void
}

interface AccountProps {
  account: BraveWallet.AccountInfo
  onSelect: (account: BraveWallet.AccountInfo) => void
}

export const Account = ({ account, onSelect }: AccountProps) => {
  return (
    <ContainerButton onClick={() => onSelect(account)}>
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
  const { accounts, selectedAsset, onSelect, isOpen, onClose, ...rest } = props

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // Memos
  const accountByCoinType = React.useMemo(() => {
    if (selectedAsset) {
      return accounts.filter(
        (account) =>
          account.accountId.coin === getMeldTokensCoinType(selectedAsset) &&
          !testnetAccountKeyringIds.includes(account.accountId.keyringId)
      )
    }
    return accounts
  }, [selectedAsset, accounts])

  const selectAccountContent = React.useMemo(() => {
    return (
      <>
        <DialogTitle slot='title'>
          {getLocale('braveWalletSelectAccount')}
        </DialogTitle>

        <ScrollableColumn
          padding={isPanel ? '0px 0px 12px 0px' : undefined}
          margin={isPanel ? '12px 0px 0px 0px' : undefined}
        >
          {accountByCoinType.map((account) => (
            <Account
              key={account.accountId.uniqueKey}
              account={account}
              onSelect={onSelect}
            />
          ))}
        </ScrollableColumn>
      </>
    )
  }, [accountByCoinType, onSelect, isPanel])

  if (isPanel) {
    return (
      <BottomSheet
        onClose={onClose}
        isOpen={isOpen}
      >
        <Column
          fullWidth={true}
          padding='0px 16px'
          height='90vh'
        >
          {selectAccountContent}
        </Column>
      </BottomSheet>
    )
  }

  return (
    <Dialog
      isOpen={isOpen}
      onClose={onClose}
      showClose
      {...rest}
    >
      {selectAccountContent}
    </Dialog>
  )
}
