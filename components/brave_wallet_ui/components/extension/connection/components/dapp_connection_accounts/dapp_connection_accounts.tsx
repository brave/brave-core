// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Queries
import {
  useGetActiveOriginConnectedAccountIdsQuery //
} from '../../../../../common/slices/api.slice'
import { useAccountsQuery } from '../../../../../common/slices/api.slice.extra'

// Types
import { BraveWallet } from '../../../../../constants/types'

// Components
import {
  ChangeAccountButton //
} from './change_account_button/change_account_button'

// Styled Components
import { Text, Row, Column, ScrollableColumn } from '../../../../shared/style'

interface Props {
  coin: BraveWallet.CoinType
  onChangeAccount: (account: BraveWallet.AccountInfo) => void
}

export const DAppConnectionAccounts = (props: Props) => {
  const { coin, onChangeAccount } = props

  // Queries
  const { accounts } = useAccountsQuery()
  const { data: connectedAccountsIds = [] } =
    useGetActiveOriginConnectedAccountIdsQuery()

  // Memos
  const accountByCoinType = React.useMemo(() => {
    return accounts.filter((account) => account.accountId.coin === coin)
  }, [accounts, coin])

  const connectedAccounts = React.useMemo(() => {
    return accountByCoinType.filter((account) =>
      connectedAccountsIds.some(
        (accountId) => accountId.uniqueKey === account.accountId.uniqueKey
      )
    )
  }, [accountByCoinType, connectedAccountsIds])

  const availableAccounts = React.useMemo(() => {
    return accountByCoinType.filter(
      (account) =>
        !connectedAccountsIds.some(
          (accountId) => accountId.uniqueKey === account.accountId.uniqueKey
        )
    )
  }, [accountByCoinType, connectedAccountsIds])

  return (
    <ScrollableColumn
      padding='8px'
      gap='32px'
      maxHeight='450px'
      margin='25px 0px 0px 0px'
    >
      {connectedAccounts.length !== 0 && (
        <Column
          width='100%'
          justifyContent='flex-start'
        >
          <Row
            justifyContent='flex-start'
            padding='0px 12px'
            marginBottom='8px'
          >
            <Text
              textSize='12px'
              isBold={true}
              textColor='tertiary'
            >
              {getLocale('braveWalletConnectedAccounts')}
            </Text>
          </Row>

          {connectedAccounts.map((account: BraveWallet.AccountInfo) => (
            <ChangeAccountButton
              key={account.accountId.uniqueKey}
              account={account}
              onClick={() => onChangeAccount(account)}
            />
          ))}
        </Column>
      )}

      {availableAccounts.length !== 0 && (
        <Column
          width='100%'
          justifyContent='flex-start'
        >
          <Row
            justifyContent='flex-start'
            padding='0px 12px'
            marginBottom='8px'
          >
            <Text
              textSize='12px'
              isBold={true}
              textColor='tertiary'
            >
              {getLocale('braveWalletAvailableAccounts')}
            </Text>
          </Row>

          {availableAccounts.map((account: BraveWallet.AccountInfo) => (
            <ChangeAccountButton
              key={account.accountId.uniqueKey}
              account={account}
              onClick={() => onChangeAccount(account)}
            />
          ))}
        </Column>
      )}
    </ScrollableColumn>
  )
}
