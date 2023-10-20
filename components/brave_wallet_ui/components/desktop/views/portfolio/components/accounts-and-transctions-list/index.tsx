// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useLocation } from 'react-router-dom'

// Constants
import {
  LOCAL_STORAGE_KEYS
} from '../../../../../../common/constants/local-storage-keys'

// Actions
import {
  WalletActions
} from '../../../../../../common/actions'

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
import { WalletSelectors } from '../../../../../../common/selectors'
import { getBalance } from '../../../../../../utils/balance-utils'
import { computeFiatAmount } from '../../../../../../utils/pricing-utils'
import {
  getIsRewardsToken, getNormalizedExternalRewardsWallet
} from '../../../../../../utils/rewards_utils'
import {
  externalWalletProviderFromString
} from '../../../../../../../brave_rewards/resources/shared/lib/external_wallet'

// Options
import {
  PortfolioAssetOptions
} from '../../../../../../options/nav-options'

// Components
import {
  PortfolioTransactionItem //
} from '../../../../portfolio_transaction_item/portfolio_transaction_item'
import {
  PortfolioAccountItem
} from '../../../../portfolio-account-item/index'
import {
  SegmentedControl
} from '../../../../../shared/segmented-control/segmented-control'
import {
  SellAssetModal
} from '../../../../popup-modals/sell-asset-modal/sell-asset-modal'

// Hooks
import {
  useUnsafeWalletSelector,
  useSafeWalletSelector
} from '../../../../../../common/hooks/use-safe-selector'
import {
  useMultiChainSellAssets
} from '../../../../../../common/hooks/use-multi-chain-sell-assets'
import {
  useGetNetworkQuery,
  useGetRewardsBalanceQuery,
  useGetSelectedChainQuery
} from '../../../../../../common/slices/api.slice'
import {
  TokenBalancesRegistry
} from '../../../../../../common/slices/entities/token-balance.entity'

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
  tokenBalancesRegistry: TokenBalancesRegistry | undefined
  spotPriceRegistry: SpotPriceRegistry | undefined
}

export const AccountsAndTransactionsList = ({
  selectedAsset,
  fullAssetFiatBalance,
  formattedFullAssetBalance,
  selectedAssetTransactions,
  accounts,
  tokenBalancesRegistry,
  spotPriceRegistry
}: Props) => {
  // routing
  const { hash } = useLocation()

  // redux
  const dispatch = useDispatch()

  // unsafe selectors
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)
  const hidePortfolioBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioBalances)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: selectedAssetNetwork } = useGetNetworkQuery(
    selectedAsset ?? skipToken
  )
  const { data: rewardsBalance } = useGetRewardsBalanceQuery()

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

  const externalRewardsAccount =
    isRewardsToken
      ? getNormalizedExternalRewardsWallet(
        externalWalletProviderFromString(selectedAsset?.chainId ?? '')
      )
      : undefined

  const filteredAccountsByCoinType = React.useMemo(() => {
    if (!selectedAsset) {
      return []
    }
    return accounts.filter((account) => account.accountId.coin === selectedAsset.coin)
  }, [accounts, selectedAsset])

  const accountsList = React.useMemo(() => {
    if (!selectedAsset) {
      return []
    }
    if (isRewardsToken) {
      return externalRewardsAccount
        ? [externalRewardsAccount]
        : []
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
    filteredAccountsByCoinType,
    spotPriceRegistry,
    tokenBalancesRegistry
  ])

  const nonRejectedTransactions = React.useMemo(() => {
    return selectedAssetTransactions
      .filter(t => t.txStatus !== BraveWallet.TransactionStatus.Rejected)
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
    openSellAssetLink({ sellAddress: selectedSellAccount?.address ?? '', sellAsset: selectedAsset })
  }, [selectedAsset, selectedSellAccount?.address, openSellAssetLink])

  const onToggleHideBalances = React.useCallback(() => {
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
      hidePortfolioBalances
        ? 'false'
        : 'true'
    )
    dispatch(
      WalletActions
        .setHidePortfolioBalances(
          !hidePortfolioBalances
        ))
  }, [hidePortfolioBalances])

  return (
    <>
      {selectedAsset &&
        <>
          {!isRewardsToken &&
            <Row padding='24px 0px'>
              <SegmentedControl
                navOptions={PortfolioAssetOptions}
                width={384}
              />
            </Row>
          }
          {hash !== WalletRoutes.TransactionsHash &&
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
                    {!isRewardsToken &&
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
                              {
                                '(' + fullAssetFiatBalance
                                  .formatAsFiat(defaultCurrencies.fiat) + ')'
                              }
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
                        <ToggleVisibilityButton
                          onClick={onToggleHideBalances}
                        >
                          <EyeIcon
                            name={
                              hidePortfolioBalances
                                ? 'eye-off'
                                : 'eye-on'
                            }
                          />
                        </ToggleVisibilityButton>
                      </Row>
                    }
                  </Row>
                  <VerticalDivider />
                  <VerticalSpacer space={8} />
                  {accountsList.map(account =>
                    <PortfolioAccountItem
                      key={account.accountId.uniqueKey}
                      asset={selectedAsset}
                      defaultCurrencies={defaultCurrencies}
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
                  )}
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
                    {
                      getLocale('braveWalletNoAccountsWithABalanceDescription')
                    }
                  </Text>
                </Column>
              )}
            </>
          }

          {hash === WalletRoutes.TransactionsHash &&
            <>
              {nonRejectedTransactions.length !== 0 ? (
                <>
                  {nonRejectedTransactions.map((transaction) =>
                    <PortfolioTransactionItem
                      key={transaction.id}
                      transaction={transaction}
                    />
                  )}
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
          }
        </>
      }
      {showSellModal && selectedAsset &&
        <SellAssetModal
          selectedAsset={selectedAsset}
          selectedAssetsNetwork={selectedAssetNetwork || selectedNetwork}
          onClose={() => setShowSellModal(false)}
          sellAmount={sellAmount}
          setSellAmount={setSellAmount}
          openSellAssetLink={onOpenSellAssetLink}
          showSellModal={showSellModal}
          account={selectedSellAccount}
          sellAssetBalance={
            getBalance(
              selectedSellAccount?.accountId,
              selectedAsset,
              tokenBalancesRegistry
            )
          }
        />
      }
    </>
  )
}

export default AccountsAndTransactionsList
