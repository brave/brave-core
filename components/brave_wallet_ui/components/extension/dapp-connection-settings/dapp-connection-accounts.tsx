// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

// Queries
import {
  useGetActiveOriginConnectedAccountIdsQuery //
} from '../../../common/slices/api.slice'
import {
  useAccountsQuery,
  useSelectedAccountQuery
} from '../../../common/slices/api.slice.extra'

// Types
import { BraveWallet } from '../../../constants/types'
import { DAppConnectionOptionsType } from './dapp-connection-settings'

// Components
import { ChangeAccountButton } from './change-account-button'

// Styled Components
import {
  DescriptionText,
  TitleText,
  BackButton,
  BackIcon
} from './dapp-connection-settings.style'
import {
  Row,
  VerticalDivider,
  VerticalSpace,
  ScrollableColumn
} from '../../shared/style'

interface Props {
  onSelectOption: (option: DAppConnectionOptionsType) => void
  getAccountsFiatValue: (account: BraveWallet.AccountInfo) => Amount
}

export const DAppConnectionAccounts = (props: Props) => {
  const { onSelectOption, getAccountsFiatValue } = props

  // Queries
  const { accounts } = useAccountsQuery()
  const { data: selectedAccount } = useSelectedAccountQuery()
  const { data: connectedAccountsIds = [] } =
    useGetActiveOriginConnectedAccountIdsQuery()

  // Constants
  const selectedCoin = selectedAccount?.accountId.coin

  // Memos
  const accountByCoinType = React.useMemo(() => {
    return accounts.filter((account) => account.accountId.coin === selectedCoin)
  }, [accounts, selectedCoin])

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

  // Methods
  const onClickBack = React.useCallback(() => {
    onSelectOption('main')
  }, [onSelectOption])

  return (
    <>
      <Row
        marginBottom={22}
        justifyContent='flex-start'
      >
        <BackButton onClick={onClickBack}>
          <BackIcon />
        </BackButton>
        <TitleText textSize='22px'>
          {getLocale('braveWalletChangeAccount')}
        </TitleText>
      </Row>

      <ScrollableColumn>
        {connectedAccounts.length !== 0 && (
          <>
            <Row justifyContent='flex-start'>
              <DescriptionText
                textSize='14px'
                isBold={true}
              >
                {getLocale('braveWalletConnectedAccounts')}
              </DescriptionText>
            </Row>

            {connectedAccounts.map((account: BraveWallet.AccountInfo) => (
              <ChangeAccountButton
                key={account.accountId.uniqueKey}
                account={account}
                getAccountsFiatValue={getAccountsFiatValue}
              />
            ))}
          </>
        )}

        {connectedAccounts.length !== 0 && availableAccounts.length !== 0 && (
          <>
            <VerticalSpace space='8px' />
            <VerticalDivider />
            <VerticalSpace space='16px' />
          </>
        )}

        {availableAccounts.length !== 0 && (
          <>
            <Row justifyContent='flex-start'>
              <DescriptionText
                textSize='14px'
                isBold={true}
              >
                {getLocale('braveWalletAvailableAccounts')}
              </DescriptionText>
            </Row>

            {availableAccounts.map((account: BraveWallet.AccountInfo) => (
              <ChangeAccountButton
                key={account.accountId.uniqueKey}
                account={account}
                getAccountsFiatValue={getAccountsFiatValue}
              />
            ))}
          </>
        )}
      </ScrollableColumn>
    </>
  )
}
