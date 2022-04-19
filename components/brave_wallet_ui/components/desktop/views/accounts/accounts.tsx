// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

import {
  WalletAccountType,
  BraveWallet,
  AddAccountNavTypes,
  WalletState
} from '../../../../constants/types'

// utils
import { getLocale } from '../../../../../common/locale'
import { groupAccountsById, sortAccountsByName } from '../../../../utils/account-utils'

// Styled Components
import {
  StyledWrapper,
  SectionTitle,
  PrimaryListContainer,
  SecondaryListContainer,
  DisclaimerText,
  SubDivider,
  ButtonRow,
  StyledButton,
  HardwareIcon,
  ButtonText,
  WalletIcon
} from './style'

// Components
import {
  AccountListItem,
  AddButton
} from '../..'

export interface Props {
  onClickAddAccount: (tabId: AddAccountNavTypes) => () => void
  onRemoveAccount: (address: string, hardware: boolean, coin: BraveWallet.CoinType) => void
  onSelectAccount: (account: WalletAccountType) => void
}

export const Accounts = ({
  onSelectAccount,
  onClickAddAccount,
  onRemoveAccount
}: Props) => {
  // redux
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)

  // memos
  const primaryAccounts = React.useMemo(() => {
    return accounts.filter((account) => account.accountType === 'Primary')
  }, [accounts])

  const secondaryAccounts = React.useMemo(() => {
    return accounts.filter((account) => account.accountType === 'Secondary')
  }, [accounts])

  const trezorAccounts = React.useMemo(() => {
    const foundTrezorAccounts = accounts.filter((account) => account.accountType === 'Trezor')
    return groupAccountsById(foundTrezorAccounts, 'deviceId')
  }, [accounts])

  const ledgerAccounts = React.useMemo(() => {
    const foundLedgerAccounts = accounts.filter((account) => account.accountType === 'Ledger')
    return groupAccountsById(foundLedgerAccounts, 'deviceId')
  }, [accounts])

  return (
    <StyledWrapper>

      <SectionTitle>{getLocale('braveWalletAccountsPrimary')}</SectionTitle>

      <DisclaimerText>{getLocale('braveWalletAccountsPrimaryDisclaimer')}</DisclaimerText>

      <SubDivider />

      <PrimaryListContainer>
        {primaryAccounts.map((account) =>
          <AccountListItem
            key={account.id}
            isHardwareWallet={false}
            onClick={onSelectAccount}
            onRemoveAccount={onRemoveAccount}
            account={account}
          />
        )}
      </PrimaryListContainer>

      <ButtonRow>
        <AddButton
          buttonType='secondary'
          onSubmit={onClickAddAccount('create')}
          text={getLocale('braveWalletCreateAccountButton')}
        />
      </ButtonRow>

      <SectionTitle>{getLocale('braveWalletAccountsSecondary')}</SectionTitle>

      <DisclaimerText>{getLocale('braveWalletAccountsSecondaryDisclaimer')}</DisclaimerText>

      <SubDivider />

      <SecondaryListContainer isHardwareWallet={false}>
        {secondaryAccounts.map((account) =>
          <AccountListItem
            key={account.id}
            isHardwareWallet={false}
            onClick={onSelectAccount}
            onRemoveAccount={onRemoveAccount}
            account={account}
          />
        )}
      </SecondaryListContainer>

      {Object.keys(trezorAccounts).map(key =>
        <SecondaryListContainer key={key} isHardwareWallet={true}>
          {sortAccountsByName(trezorAccounts[key]).map((account: WalletAccountType) =>
            <AccountListItem
              key={account.id}
              isHardwareWallet={true}
              onClick={onSelectAccount}
              onRemoveAccount={onRemoveAccount}
              account={account}
            />
          )}
        </SecondaryListContainer>
      )}

      {Object.keys(ledgerAccounts).map(key =>
        <SecondaryListContainer key={key} isHardwareWallet={true}>
          {sortAccountsByName(ledgerAccounts[key]).map((account: WalletAccountType) =>
            <AccountListItem
              key={account.id}
              isHardwareWallet={true}
              onClick={onSelectAccount}
              onRemoveAccount={onRemoveAccount}
              account={account}
            />
          )}
        </SecondaryListContainer>
      )}

      <ButtonRow>
        <StyledButton onClick={onClickAddAccount('import')}>
          <WalletIcon />
          <ButtonText>{getLocale('braveWalletAddAccountImport')}</ButtonText>
        </StyledButton>
        <StyledButton onClick={onClickAddAccount('hardware')}>
          <HardwareIcon />
          <ButtonText>{getLocale('braveWalletAddAccountImportHardware')}</ButtonText>
        </StyledButton>
      </ButtonRow>
    </StyledWrapper>
  )
}

export default Accounts
