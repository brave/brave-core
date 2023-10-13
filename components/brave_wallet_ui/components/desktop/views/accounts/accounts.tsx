// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

import { BraveWallet, AccountPageTabs } from '../../../../constants/types'
import {
  querySubscriptionOptions60s
} from '../../../../common/slices/constants'

// Selectors
import {
  useUnsafeWalletSelector
} from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../common/selectors'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  getAccountType,
  groupAccountsById,
  sortAccountsByName
} from '../../../../utils/account-utils'
import { makeAccountRoute } from '../../../../utils/routes-utils'
import {
  getPriceIdForToken
} from '../../../../utils/api-utils'
import {
  getNormalizedExternalRewardsWallet
} from '../../../../utils/rewards_utils'

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

// Hooks
import {
  useBalancesFetcher
} from '../../../../common/hooks/use-balances-fetcher'
import {
  useGetDefaultFiatCurrencyQuery,
  useGetVisibleNetworksQuery,
  useGetTokenSpotPricesQuery,
  useGetRewardsEnabledQuery,
  useGetExternalRewardsWalletQuery
} from '../../../../common/slices/api.slice'
import { useAccountsQuery } from '../../../../common/slices/api.slice.extra'

export const Accounts = () => {
  // routing
  const history = useHistory()

  // wallet state
  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )

  // queries
  const { accounts } = useAccountsQuery()
  const { data: isRewardsEnabled } = useGetRewardsEnabledQuery()
  const { data: externalRewardsInfo } = useGetExternalRewardsWalletQuery()

  // methods
  const onSelectAccount = React.useCallback(
    (account: BraveWallet.AccountInfo | undefined) => {
      if (account) {
        history.push(makeAccountRoute(account, AccountPageTabs.AccountAssetsSub))
      }
    },
    [history]
  )

  // memos && computed
  const externalRewardsAccount =
    isRewardsEnabled
      ? getNormalizedExternalRewardsWallet(
        externalRewardsInfo?.provider ?? undefined
      )
      : undefined

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

  const { data: networks } = useGetVisibleNetworksQuery()
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const {
    data: tokenBalancesRegistry
  } = useBalancesFetcher({
    accounts,
    networks
  })

  const tokenPriceIds = React.useMemo(() =>
    userVisibleTokensInfo
      .filter((token) => !token.isErc721 && !token.isErc1155 && !token.isNft)
      .map(token => getPriceIdForToken(token)),
    [userVisibleTokensInfo]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length && defaultFiatCurrency
      ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

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
        .map((account: BraveWallet.AccountInfo) =>
          <AccountListItem
            key={account.accountId.uniqueKey}
            onClick={onSelectAccount}
            account={account}
            tokenBalancesRegistry={tokenBalancesRegistry}
            spotPriceRegistry={spotPriceRegistry}
          />
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
        .map((account: BraveWallet.AccountInfo) =>
          <AccountListItem
            key={account.accountId.uniqueKey}
            onClick={onSelectAccount}
            account={account}
            tokenBalancesRegistry={tokenBalancesRegistry}
            spotPriceRegistry={spotPriceRegistry}
          />
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
            tokenBalancesRegistry={tokenBalancesRegistry}
            spotPriceRegistry={spotPriceRegistry}
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
                tokenBalancesRegistry={tokenBalancesRegistry}
                spotPriceRegistry={spotPriceRegistry}
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

      {externalRewardsAccount &&
        <>
          <Row
            padding='8px'
            justifyContent='flex-start'
          >
            <SectionTitle>
              {getLocale('braveWalletConnectedAccounts')}
            </SectionTitle>
          </Row>
          <Column
            fullWidth={true}
            alignItems='flex-start'
            margin='0px 0px 24px 0px'
          >
            <AccountListItem
              key={externalRewardsAccount.accountId.uniqueKey}
              onClick={onSelectAccount}
              account={externalRewardsAccount}
              tokenBalancesRegistry={tokenBalancesRegistry}
              spotPriceRegistry={spotPriceRegistry}
            />
          </Column>
        </>
      }
    </WalletPageWrapper>
  )
}

export default Accounts
