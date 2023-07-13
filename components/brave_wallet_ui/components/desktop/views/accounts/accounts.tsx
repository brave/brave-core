// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { useHistory } from 'react-router'

import {
  WalletAccountType,
  WalletState,
  WalletRoutes,
  BraveWallet
} from '../../../../constants/types'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  getAccountType,
  groupAccountsById,
  sortAccountsByName
} from '../../../../utils/account-utils'

// Styled Components
import {
  SectionTitle
} from './style'

import {
  Column,
  Row
} from '../../../shared/style'

// Components
import AccountListItem from '../../account-list-item'
import {
  WalletPageWrapper
} from '../../wallet-page-wrapper/wallet-page-wrapper'
import {
  AccountsHeader
} from '../../card-headers/accounts-header'

export const Accounts = () => {
  // routing
  const history = useHistory()

  // wallet state
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)

  // methods
  const onSelectAccount = React.useCallback((account: WalletAccountType | undefined) => {
    if (account) {
      history.push(`${WalletRoutes.Accounts}/${account.address}`)
    }
  }, [])

  // memos
  const derivedAccounts = React.useMemo(() => {
    return accounts.filter(
      (account) =>
        account.accountId.kind === BraveWallet.AccountKind.kDerived)
  }, [accounts])

  const importedAccounts = React.useMemo(() => {
    return accounts.filter(
      (account) =>
        account.accountId.kind === BraveWallet.AccountKind.kImported)
  }, [accounts])

  const trezorAccounts = React.useMemo(() => {
    const foundTrezorAccounts = accounts.filter((account) => getAccountType(account) === 'Trezor')
    return groupAccountsById(foundTrezorAccounts, 'deviceId')
  }, [accounts])

  const ledgerAccounts = React.useMemo(() => {
    const foundLedgerAccounts = accounts.filter((account) => getAccountType(account) === 'Ledger')
    return groupAccountsById(foundLedgerAccounts, 'deviceId')
  }, [accounts])

  const trezorKeys = React.useMemo(() => {
    return Object.keys(trezorAccounts)
  }, [trezorAccounts])

  const trezorList = React.useMemo(() => {
    return trezorKeys.map(key => <Column
      fullWidth={true}
      alignItems='flex-start'
      key={key}
    >
      {sortAccountsByName(trezorAccounts[key])
        .map((account: WalletAccountType) => <AccountListItem
          key={account.accountId.uniqueKey}
          onClick={onSelectAccount}
          account={account} />
        )}
    </Column>
    )
  }, [
    trezorKeys,
    trezorAccounts,
    onSelectAccount
  ])

  const ledgerKeys = React.useMemo(() => {
    return Object.keys(ledgerAccounts)
  }, [ledgerAccounts])

  const ledgerList = React.useMemo(() => {
    return ledgerKeys.map(key => <Column
      fullWidth={true}
      alignItems='flex-start'
      key={key}
    >
      {sortAccountsByName(ledgerAccounts[key])
        .map((account: WalletAccountType) => <AccountListItem
          key={account.accountId.uniqueKey}
          onClick={onSelectAccount}
          account={account} />
        )}
    </Column>
    )
  }, [
    ledgerKeys,
    ledgerAccounts,
    onSelectAccount
  ])


  // computed
  const showHardwareWallets = trezorKeys.length !== 0 ||
    ledgerKeys.length !== 0

  // render
  return (
    <WalletPageWrapper
      wrapContentInBox
      cardHeader={
        <AccountsHeader />
      }
    >
      <Row
        padding='8px'
        justifyContent='flex-start'
      >
        <SectionTitle
        >
          {getLocale('braveWalletAccounts')}
        </SectionTitle>
      </Row>
      <Column
        fullWidth={true}
        alignItems='flex-start'
        margin='0px 0px 24px 0px'
      >
        {derivedAccounts.map((account) =>
          <AccountListItem
            key={account.accountId.uniqueKey}
            onClick={onSelectAccount}
            account={account}
          />
        )}
      </Column>

      {importedAccounts.length !== 0 &&
        <>
          <Row
            padding='8px'
            justifyContent='flex-start'
          >
            <SectionTitle>
              {getLocale('braveWalletAccountsSecondary')}
            </SectionTitle>
          </Row>
          <Column
            fullWidth={true}
            alignItems='flex-start'
            margin='0px 0px 24px 0px'
          >
            {importedAccounts.map((account) =>
              <AccountListItem
                key={account.accountId.uniqueKey}
                onClick={onSelectAccount}
                account={account}
              />
            )}
          </Column>
        </>
      }

      {showHardwareWallets &&
        <>
          <Row
            padding='8px'
            justifyContent='flex-start'
          >
            <SectionTitle>
              {getLocale('braveWalletConnectedHardwareWallets')}
            </SectionTitle>
          </Row>
          <Column
            fullWidth={true}
            alignItems='flex-start'
            margin='0px 0px 24px 0px'
          >
            {trezorList}
            {ledgerList}
          </Column>
        </>
      }
    </WalletPageWrapper>
  )
}

export default Accounts
