// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import {
  BraveWallet,
  AddAccountNavTypes,
  WalletAccountType
} from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'
import Amount from '../../../../../../utils/amount'
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

import { SellAssetModal } from '../../../../popup-modals/sell-asset-modal/sell-asset-modal'

// Hooks
import { useUnsafeWalletSelector } from '../../../../../../common/hooks/use-safe-selector'
import { useMultiChainSellAssets } from '../../../../../../common/hooks/use-multi-chain-sell-assets'
import {
  useGetNetworkQuery,
  useGetSelectedChainQuery //
} from '../../../../../../common/slices/api.slice'

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
  Column,
  HorizontalSpace,
  Row,
  ToggleVisibilityButton
} from '../../../../../shared/style'

export interface Props {
  selectedAsset: BraveWallet.BlockchainToken | undefined
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
  onClickAddAccount
}: Props) => {
  // redux
  // unsafe selectors
  const transactionSpotPrices = useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: selectedAssetNetwork } = useGetNetworkQuery(selectedAsset, {
    skip: !selectedAsset
  })


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
  const [hideBalances, setHideBalances] = React.useState<boolean>(false)
  const [selectedSellAccount, setSelectedSellAccount] = React.useState<WalletAccountType>()
  const [showSellModal, setShowSellModal] = React.useState<boolean>(false)

  // computed
  const isNonFungibleToken = selectedAsset?.isErc721 || selectedAsset?.isNft

  // memos
  const filteredAccountsByCoinType = React.useMemo(() => {
    if (!selectedAsset) {
      return []
    }
    return accounts.filter((account) => account.coin === selectedAsset.coin)
  }, [accounts, selectedAsset])

  const accountsList = React.useMemo(() => {
    return filteredAccountsByCoinType.filter((account) => new Amount(getBalance(account, selectedAsset)).gt(0))
  }, [selectedAsset, filteredAccountsByCoinType])

  const nonRejectedTransactions = React.useMemo(() => {
    return selectedAssetTransactions
      .filter(t => t.status !== BraveWallet.TransactionStatus.Rejected)
  }, [selectedAssetTransactions])

  // Methods
  const onShowSellModal = React.useCallback((account: WalletAccountType) => {
    setSelectedSellAccount(account)
    setShowSellModal(true)
  }, [])

  const onOpenSellAssetLink = React.useCallback(() => {
    openSellAssetLink({ sellAddress: selectedSellAccount?.address ?? '', sellAsset: selectedAsset })
  }, [selectedAsset, selectedSellAccount?.address, openSellAssetLink])

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
          <Column fullWidth={true} alignItems='flex-start'>
            <DividerRow>
              <DividerText>
                {
                  isNonFungibleToken
                    ? getLocale('braveWalletOwner')
                    : getLocale('braveWalletAccounts')
                }
              </DividerText>
              <Row justifyContent='flex-end'>
                {!isNonFungibleToken &&
                  <WithHideBalancePlaceholder
                    size='small'
                    hideBalances={hideBalances}
                  >
                    <AssetBalanceDisplay>
                      {
                        fullAssetFiatBalance
                          .formatAsFiat(defaultCurrencies.fiat)
                      } {formattedFullAssetBalance}
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
          </Column>
          {accountsList.map((account) =>
            <PortfolioAccountItem
              spotPrices={transactionSpotPrices}
              defaultCurrencies={defaultCurrencies}
              key={account.address}
              assetContractAddress={selectedAsset.contractAddress}
              assetChainId={selectedAsset.chainId}
              assetTicker={selectedAsset.symbol}
              assetDecimals={selectedAsset.decimals}
              name={account.name}
              address={account.address}
              assetBalance={getBalance(account, selectedAsset)}
              selectedNetwork={selectedAssetNetwork || selectedNetwork}
              hideBalances={hideBalances}
              isNft={isNonFungibleToken}
              showSellModal={() => onShowSellModal(account)}
              isSellSupported={checkIsAssetSellSupported(selectedAsset)}
            />
          )}
          <ButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={onClickAddAccount('create')}
              text={getLocale('braveWalletAddAccount')}
            />
          </ButtonRow>

          <Column fullWidth={true} alignItems='flex-start'>
            <DividerText>{getLocale('braveWalletTransactions')}</DividerText>
            <SubDivider />
          </Column>

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
