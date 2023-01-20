// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import {
  BraveWallet,
  AddAccountNavTypes
} from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'
import Amount from '../../../../../../utils/amount'
import { getTokensNetwork } from '../../../../../../utils/network-utils'
import { WalletSelectors } from '../../../../../../common/selectors'
import { getBalance } from '../../../../../../utils/balance-utils'
import { ParsedTransaction } from '../../../../../../utils/tx-utils'

// Components
import {
  PortfolioTransactionItem,
  PortfolioAccountItem,
  AddButton,
  WithHideBalancePlaceholder
} from '../../../../'

// Hooks
import { useUnsafeWalletSelector } from '../../../../../../common/hooks/use-safe-selector'

// Styled Components
import {
  ButtonRow,
  DividerText,
  SubDivider,
  EmptyTransactionContainer,
  TransactionPlaceholderText,
  AssetBalanceDisplay,
  DividerRow
} from '../../style'
import {
  HorizontalSpace,
  Row,
  ToggleVisibilityButton
} from '../../../../../shared/style'

export interface Props {
  selectedAsset: BraveWallet.BlockchainToken | undefined
  networkList: BraveWallet.NetworkInfo[]
  fullAssetFiatBalance: Amount
  formattedFullAssetBalance: string
  selectedAssetTransactions: ParsedTransaction[]
  onClickAddAccount: (tabId: AddAccountNavTypes) => () => void
}

export const AccountsAndTransactionsList = ({
  selectedAsset,
  fullAssetFiatBalance,
  formattedFullAssetBalance,
  selectedAssetTransactions,
  networkList,
  onClickAddAccount
}: Props) => {
  // redux
  // unsafe selectors
  const transactionSpotPrices = useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)
  const selectedNetwork = useUnsafeWalletSelector(WalletSelectors.selectedNetwork)

  // state
  const [hideBalances, setHideBalances] = React.useState<boolean>(false)

  const isNonFungibleToken = React.useMemo(() => {
    return selectedAsset?.isErc721 || selectedAsset?.isNft
  }, [selectedAsset])

  const selectedAssetsNetwork = React.useMemo(() => {
    if (!selectedAsset) {
      return selectedNetwork
    }
    return getTokensNetwork(networkList, selectedAsset)
  }, [selectedNetwork, selectedAsset, networkList])

  const filteredAccountsByCoinType = React.useMemo(() => {
    if (!selectedAsset) {
      return []
    }
    return accounts.filter((account) => account.coin === selectedAsset.coin)
  }, [accounts, selectedAsset])

  const accountsList = React.useMemo(() => {
    if (isNonFungibleToken && selectedAssetsNetwork) {
      return filteredAccountsByCoinType.filter((account) => Number(account.nativeBalanceRegistry[selectedAssetsNetwork.chainId] ?? 0) !== 0)
    }
    return filteredAccountsByCoinType
  }, [selectedAsset, filteredAccountsByCoinType])

  const nonRejectedTransactions = React.useMemo(() => {
    return selectedAssetTransactions
      .filter(t => t.status !== BraveWallet.TransactionStatus.Rejected)
  }, [selectedAssetTransactions])

  return (
    <>
      {selectedAsset &&
        <>
          <DividerRow>
            <DividerText>{isNonFungibleToken ? getLocale('braveWalletOwner') : getLocale('braveWalletAccounts')}</DividerText>
            <Row justifyContent='flex-end'>
              {!isNonFungibleToken &&
                <WithHideBalancePlaceholder
                  size='small'
                  hideBalances={hideBalances}
                >
                  <AssetBalanceDisplay>
                    {fullAssetFiatBalance.formatAsFiat(defaultCurrencies.fiat)} {formattedFullAssetBalance}
                  </AssetBalanceDisplay>
                </WithHideBalancePlaceholder>
              }
              <HorizontalSpace space='16px' />
              <ToggleVisibilityButton
                isVisible={!hideBalances}
                onClick={() => setHideBalances(prev => !prev)}
              />
            </Row>
          </DividerRow>
          <SubDivider />
          {accountsList.map((account) =>
            <PortfolioAccountItem
              spotPrices={transactionSpotPrices}
              defaultCurrencies={defaultCurrencies}
              key={account.address}
              assetTicker={selectedAsset.symbol}
              assetDecimals={selectedAsset.decimals}
              name={account.name}
              address={account.address}
              assetBalance={getBalance(account, selectedAsset)}
              selectedNetwork={selectedAssetsNetwork}
              hideBalances={hideBalances}
              isNft={isNonFungibleToken}
            />
          )}
          <ButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={onClickAddAccount('create')}
              text={getLocale('braveWalletAddAccount')}
            />
          </ButtonRow>
          <DividerText>{getLocale('braveWalletTransactions')}</DividerText>
          <SubDivider />
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
            <EmptyTransactionContainer>
              <TransactionPlaceholderText>{getLocale('braveWalletTransactionPlaceholder')}</TransactionPlaceholderText>
            </EmptyTransactionContainer>
          )}
        </>
      }
    </>
  )
}

export default AccountsAndTransactionsList
