// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useLocation } from 'react-router-dom'

// Constants
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../../../common/constants/local-storage-keys'
import {
  emptyRewardsInfo //
} from '../../../../../../common/async/base-query-cache'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo,
  SpotPriceRegistry,
  WalletRoutes
} from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'
import Amount from '../../../../../../utils/amount'
import { getBalance } from '../../../../../../utils/balance-utils'
import { computeFiatAmount } from '../../../../../../utils/pricing-utils'
import { getIsRewardsToken } from '../../../../../../utils/rewards_utils'

// Options
import { PortfolioAssetOptions } from '../../../../../../options/nav-options'

// Components
import {
  PortfolioTransactionItem //
} from '../../../../portfolio_transaction_item/portfolio_transaction_item'
import { PortfolioAccountItem } from '../../../../portfolio-account-item/index'
import {
  SegmentedControl //
} from '../../../../../shared/segmented_control/segmented_control'
import {
  SellAssetModal //
} from '../../../../popup-modals/sell-asset-modal/sell-asset-modal'
import { LoadingSkeleton } from '../../../../../shared/loading-skeleton/index'

// Hooks
import {
  useMultiChainSellAssets //
} from '../../../../../../common/hooks/use-multi-chain-sell-assets'
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetRewardsInfoQuery,
  useGetSelectedChainQuery
} from '../../../../../../common/slices/api.slice'
import {
  TokenBalancesRegistry //
} from '../../../../../../common/slices/entities/token-balance.entity'
import {
  useSyncedLocalStorage //
} from '../../../../../../common/hooks/use_local_storage'

// Styled Components
import {
  ToggleVisibilityButton,
  EmptyTransactionsIcon,
  EmptyAccountsIcon,
  EyeIcon
} from '../../style'
import {
  Column,
  Text,
  Row,
  VerticalDivider,
  VerticalSpacer,
  HorizontalSpace
} from '../../../../../shared/style'

interface Props {
  selectedAsset: BraveWallet.BlockchainToken | undefined
  fullAssetFiatBalance: Amount
  formattedFullAssetBalance: string
  selectedAssetTransactions: SerializableTransactionInfo[]
  accounts: BraveWallet.AccountInfo[]
  tokenBalancesRegistry: TokenBalancesRegistry | undefined | null
  isLoadingBalances: boolean
  spotPriceRegistry: SpotPriceRegistry | undefined
}

export const AccountsAndTransactionsList = ({
  selectedAsset,
  fullAssetFiatBalance,
  formattedFullAssetBalance,
  selectedAssetTransactions,
  accounts,
  tokenBalancesRegistry,
  isLoadingBalances,
  spotPriceRegistry
}: Props) => {
  // routing
  const { hash } = useLocation()

  // local-storage
  const [hidePortfolioBalances, setHidePortfolioBalances] =
    useSyncedLocalStorage(LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES, false)

  // queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: selectedAssetNetwork } = useGetNetworkQuery(
    selectedAsset ?? skipToken
  )
  const {
    data: { balance: rewardsBalance, rewardsAccount } = emptyRewardsInfo
  } = useGetRewardsInfoQuery()

  // hooks
  const {
    checkIsAssetSellSupported,
    sellAmount,
    setSellAmount,
    openSellAssetLink
  } = useMultiChainSellAssets()

  // state
  const [selectedSellAccount, setSelectedSellAccount] =
    React.useState<BraveWallet.AccountInfo>()
  const [showSellModal, setShowSellModal] = React.useState<boolean>(false)

  // Memos & Computed
  const isRewardsToken = getIsRewardsToken(selectedAsset)

  const externalRewardsAccount = isRewardsToken ? rewardsAccount : undefined

  const filteredAccountsByCoinType = React.useMemo(() => {
    if (!selectedAsset) {
      return []
    }
    return accounts.filter(
      (account) => account.accountId.coin === selectedAsset.coin
    )
  }, [accounts, selectedAsset])

  const accountsList = React.useMemo(() => {
    if (!selectedAsset) {
      return []
    }
    if (isRewardsToken) {
      return externalRewardsAccount ? [externalRewardsAccount] : []
    }
    return filteredAccountsByCoinType
      .filter((account) =>
        new Amount(
          getBalance(account.accountId, selectedAsset, tokenBalancesRegistry)
        ).gt(0)
      )
      .sort((a, b) => {
        const aBalance = computeFiatAmount({
          spotPriceRegistry,
          value: getBalance(a.accountId, selectedAsset, tokenBalancesRegistry),
          token: selectedAsset
        })

        const bBalance = computeFiatAmount({
          spotPriceRegistry,
          value: getBalance(b.accountId, selectedAsset, tokenBalancesRegistry),
          token: selectedAsset
        })

        return bBalance.minus(aBalance).toNumber()
      })
  }, [
    selectedAsset,
    isRewardsToken,
    filteredAccountsByCoinType,
    externalRewardsAccount,
    tokenBalancesRegistry,
    spotPriceRegistry
  ])

  const nonRejectedTransactions = React.useMemo(() => {
    return selectedAssetTransactions.filter(
      (t) => t.txStatus !== BraveWallet.TransactionStatus.Rejected
    )
  }, [selectedAssetTransactions])

  // Methods
  const onShowSellModal = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      setSelectedSellAccount(account)
      setShowSellModal(true)
    },
    []
  )

  const onOpenSellAssetLink = React.useCallback(() => {
    openSellAssetLink({
      sellAsset: selectedAsset
    })
  }, [selectedAsset, openSellAssetLink])

  const onToggleHideBalances = React.useCallback(() => {
    setHidePortfolioBalances((prev) => !prev)
  }, [setHidePortfolioBalances])

  if (
    hash !== WalletRoutes.TransactionsHash &&
    isLoadingBalances &&
    accountsList.length === 0
  ) {
    return (
      <>
        {!isRewardsToken && (
          <Row padding='24px 0px'>
            <SegmentedControl
              navOptions={PortfolioAssetOptions}
              width={'384px'}
            />
          </Row>
        )}
        <Row
          width='100%'
          justifyContent='space-between'
          alignItems='center'
          marginBottom={18}
          padding='0px 8px'
        >
          <Text
            isBold={true}
            textColor='text01'
            textSize='16px'
          >
            {getLocale('braveWalletAccounts')}
          </Text>
          <div>
            <LoadingSkeleton
              width={60}
              height={22}
            />
          </div>
        </Row>
        <VerticalDivider />
        <VerticalSpacer space={8} />
        <Row
          padding='8px'
          justifyContent='space-between'
        >
          <Row
            width='unset'
            justifyContent='flex-start'
          >
            <LoadingSkeleton
              width={44}
              height={44}
              borderRadius={8}
            />
            <Column
              padding='0px 0px 0px 12px'
              alignItems='flex-start'
            >
              <LoadingSkeleton
                width={80}
                height={18}
                borderRadius={8}
              />
              <VerticalSpacer space={4} />
              <LoadingSkeleton
                width={80}
                height={16}
                borderRadius={8}
              />
            </Column>
          </Row>
          <Column alignItems='flex-end'>
            <LoadingSkeleton
              width={80}
              height={18}
              borderRadius={8}
            />
            <VerticalSpacer space={4} />
            <LoadingSkeleton
              width={80}
              height={16}
              borderRadius={8}
            />
          </Column>
        </Row>
      </>
    )
  }

  return (
    <>
      {selectedAsset && (
        <>
          {!isRewardsToken && (
            <Row padding='24px 0px'>
              <SegmentedControl
                navOptions={PortfolioAssetOptions}
                width={'384px'}
              />
            </Row>
          )}
          {hash !== WalletRoutes.TransactionsHash && (
            <>
              {accountsList.length !== 0 ? (
                <>
                  <Row
                    width='100%'
                    justifyContent='space-between'
                    alignItems='center'
                    marginBottom={18}
                    padding='0px 8px'
                  >
                    <Text
                      isBold={true}
                      textColor='text01'
                      textSize='16px'
                    >
                      {getLocale('braveWalletAccounts')}
                    </Text>
                    {!isRewardsToken && (
                      <Row
                        width='unset'
                        justifyContent='flex-end'
                      >
                        {!hidePortfolioBalances ? (
                          <>
                            <Text
                              isBold={true}
                              textColor='text01'
                              textSize='14px'
                            >
                              {formattedFullAssetBalance}
                            </Text>
                            <HorizontalSpace space='4px' />
                            <Text
                              isBold={false}
                              textColor='text03'
                              textSize='14px'
                            >
                              {'(' +
                                fullAssetFiatBalance.formatAsFiat(
                                  defaultFiatCurrency
                                ) +
                                ')'}
                            </Text>
                          </>
                        ) : (
                          <Text
                            isBold={true}
                            textColor='text01'
                            textSize='14px'
                          >
                            ******
                          </Text>
                        )}
                        <HorizontalSpace space='16px' />
                        <ToggleVisibilityButton onClick={onToggleHideBalances}>
                          <EyeIcon
                            name={hidePortfolioBalances ? 'eye-off' : 'eye-on'}
                          />
                        </ToggleVisibilityButton>
                      </Row>
                    )}
                  </Row>
                  <VerticalDivider />
                  <VerticalSpacer space={8} />
                  {accountsList.map((account) => (
                    <PortfolioAccountItem
                      key={account.accountId.uniqueKey}
                      asset={selectedAsset}
                      account={account}
                      assetBalance={
                        isRewardsToken && rewardsBalance
                          ? new Amount(rewardsBalance)
                              .multiplyByDecimals(selectedAsset.decimals)
                              .format()
                          : getBalance(
                              account.accountId,
                              selectedAsset,
                              tokenBalancesRegistry
                            )
                      }
                      selectedNetwork={selectedAssetNetwork || selectedNetwork}
                      showSellModal={() => onShowSellModal(account)}
                      isSellSupported={checkIsAssetSellSupported(selectedAsset)}
                      hideBalances={hidePortfolioBalances}
                    />
                  ))}
                </>
              ) : (
                <Column
                  margin='20px 0px 40px 0px'
                  alignItems='center'
                  justifyContent='center'
                >
                  <EmptyAccountsIcon />
                  <Text
                    textColor='text01'
                    textSize='16px'
                    isBold={true}
                  >
                    {getLocale('braveWalletNoAccountsWithABalance')}
                  </Text>
                  <VerticalSpacer space={10} />
                  <Text
                    textSize='14px'
                    textColor='text03'
                    isBold={false}
                  >
                    {getLocale('braveWalletNoAccountsWithABalanceDescription')}
                  </Text>
                </Column>
              )}
            </>
          )}

          {hash === WalletRoutes.TransactionsHash && (
            <>
              {nonRejectedTransactions.length !== 0 ? (
                <>
                  {nonRejectedTransactions.map((transaction) => (
                    <PortfolioTransactionItem
                      key={transaction.id}
                      transaction={transaction}
                    />
                  ))}
                </>
              ) : (
                <Column
                  margin='20px 0px 40px 0px'
                  alignItems='center'
                  justifyContent='center'
                >
                  <EmptyTransactionsIcon />
                  <Text
                    textColor='text01'
                    textSize='16px'
                    isBold={true}
                  >
                    {getLocale('braveWalletNoTransactionsYet')}
                  </Text>
                  <VerticalSpacer space={10} />
                  <Text
                    textSize='14px'
                    textColor='text03'
                    isBold={false}
                  >
                    {getLocale('braveWalletNoTransactionsYetDescription')}
                  </Text>
                </Column>
              )}
            </>
          )}
        </>
      )}
      {showSellModal && selectedAsset && (
        <SellAssetModal
          selectedAsset={selectedAsset}
          onClose={() => setShowSellModal(false)}
          sellAmount={sellAmount}
          setSellAmount={setSellAmount}
          openSellAssetLink={onOpenSellAssetLink}
          showSellModal={showSellModal}
          account={selectedSellAccount}
          sellAssetBalance={getBalance(
            selectedSellAccount?.accountId,
            selectedAsset,
            tokenBalancesRegistry
          )}
        />
      )}
    </>
  )
}

export default AccountsAndTransactionsList
