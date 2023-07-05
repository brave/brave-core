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
  WalletAccountType,
  SerializableTransactionInfo,
  WalletRoutes
} from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'
import Amount from '../../../../../../utils/amount'
import { WalletSelectors } from '../../../../../../common/selectors'
import { getBalance } from '../../../../../../utils/balance-utils'
import { computeFiatAmount } from '../../../../../../utils/pricing-utils'
import { getPriceIdForToken } from '../../../../../../utils/api-utils'

// Options
import {
  PortfolioAssetOptions
} from '../../../../../../options/nav-options'

// Components
import {
  PortfolioTransactionItem,
  PortfolioAccountItem
} from '../../../../'
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
  useGetSelectedChainQuery,
  useGetTokenSpotPricesQuery
} from '../../../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../../../../common/slices/constants'

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

export interface Props {
  selectedAsset: BraveWallet.BlockchainToken | undefined
  fullAssetFiatBalance: Amount
  formattedFullAssetBalance: string
  selectedAssetTransactions: SerializableTransactionInfo[]
}

export const AccountsAndTransactionsList = ({
  selectedAsset,
  fullAssetFiatBalance,
  formattedFullAssetBalance,
  selectedAssetTransactions
}: Props) => {
  // routing
  const { hash } = useLocation()

  // redux
  const dispatch = useDispatch()

  // unsafe selectors
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)
  const hidePortfolioBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioBalances)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: selectedAssetNetwork } = useGetNetworkQuery(
    selectedAsset ?? skipToken
  )

  const tokenPriceIds = React.useMemo(() =>
    selectedAsset
      ? [getPriceIdForToken(selectedAsset)]
      : [],
    [selectedAsset]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length ? { ids: tokenPriceIds } : skipToken,
    querySubscriptionOptions60s
  )

  // hooks
  const {
    allSellAssetOptions,
    getAllSellAssetOptions,
    checkIsAssetSellSupported,
    sellAmount,
    setSellAmount,
    openSellAssetLink
  } = useMultiChainSellAssets()

  // state
  const [selectedSellAccount, setSelectedSellAccount] = React.useState<WalletAccountType>()
  const [showSellModal, setShowSellModal] = React.useState<boolean>(false)

  // memos
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
    return filteredAccountsByCoinType
      .filter(
        (account) =>
          new Amount(getBalance(account, selectedAsset)).gt(0)
      )
      .sort(
        (a, b) => {
          const aBalance = computeFiatAmount({
            spotPriceRegistry,
            value: getBalance(a, selectedAsset),
            token: selectedAsset
          })

          const bBalance = computeFiatAmount({
            spotPriceRegistry,
            value: getBalance(b, selectedAsset),
            token: selectedAsset
          })

          return bBalance.minus(aBalance).toNumber()
        })
  }, [
    selectedAsset,
    filteredAccountsByCoinType,
    spotPriceRegistry
  ])

  const nonRejectedTransactions = React.useMemo(() => {
    return selectedAssetTransactions
      .filter(t => t.txStatus !== BraveWallet.TransactionStatus.Rejected)
  }, [selectedAssetTransactions])

  // Methods
  const onShowSellModal = React.useCallback((account: WalletAccountType) => {
    setSelectedSellAccount(account)
    setShowSellModal(true)
  }, [])

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

  // Effects
  React.useEffect(() => {
    if (allSellAssetOptions.length === 0) {
      getAllSellAssetOptions()
    }
  }, [allSellAssetOptions.length, getAllSellAssetOptions])

  return (
    <>
      {selectedAsset &&
        <>
          <Row padding='24px 0px'>
            <SegmentedControl
              navOptions={PortfolioAssetOptions}
              width={384}
            />
          </Row>
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
                  </Row>
                  <VerticalDivider />
                  <VerticalSpacer space={8} />
                  {accountsList.map((account) =>
                    <PortfolioAccountItem
                      asset={selectedAsset}
                      defaultCurrencies={defaultCurrencies}
                      key={account.accountId.uniqueKey}
                      name={account.name}
                      address={account.address}
                      accountKind={account.accountId.kind}
                      assetBalance={getBalance(account, selectedAsset)}
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
                      displayAccountName={true}
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
          sellAssetBalance={getBalance(selectedSellAccount, selectedAsset)}
        />
      }
    </>
  )
}

export default AccountsAndTransactionsList
