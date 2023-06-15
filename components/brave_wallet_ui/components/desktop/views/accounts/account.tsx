// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, useParams, useLocation } from 'react-router'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { create } from 'ethereum-blockies'

// Selectors
import { useSafeWalletSelector, useUnsafeWalletSelector } from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../common/selectors'

// Types
import {
  BraveWallet,
  CoinTypesMap,
  WalletRoutes,
  AccountButtonOptionsObjectType
} from '../../../../constants/types'

// utils
import { reduceAddress } from '../../../../utils/reduce-address'
import { getLocale } from '../../../../../common/locale'
import { sortTransactionByDate } from '../../../../utils/tx-utils'
import { getBalance } from '../../../../utils/balance-utils'
import {
  getFilecoinKeyringIdFromNetwork
} from '../../../../utils/network-utils'
import Amount from '../../../../utils/amount'
import {
  getTokenPriceAmountFromRegistry
} from '../../../../utils/pricing-utils'
import { getPriceIdForToken } from '../../../../utils/api-utils'
import {
  selectAllUserAssetsFromQueryResult
} from '../../../../common/slices/entities/blockchain-token.entity'

// Styled Components
import {
  StyledWrapper,
  SubDivider,
  WalletInfoRow,
  WalletAddress,
  WalletName,
  AccountCircle,
  WalletInfoLeftSide,
  SubviewSectionTitle,
  TransactionPlaceholderContainer,
  AccountButtonsRow
} from './style'
import { TransactionPlaceholderText, Spacer } from '../portfolio/style'
import { ScrollableColumn, Column } from '../../../shared/style'

// Components
import { PortfolioTransactionItem } from '../../portfolio-transaction-item/index'
import { PortfolioAssetItemLoadingSkeleton } from '../../portfolio-asset-item/portfolio-asset-item-loading-skeleton'
import { PortfolioAssetItem } from '../../portfolio-asset-item/index'
import { CopyTooltip } from '../../../shared/copy-tooltip/copy-tooltip'
import { AccountListItemOptionButton } from '../../account-list-item/account-list-item-option-button'
import { SellAssetModal } from '../../popup-modals/sell-asset-modal/sell-asset-modal'

// options
import { AccountButtonOptions } from '../../../../options/account-list-button-options'

// Hooks
import { useScrollIntoView } from '../../../../common/hooks/use-scroll-into-view'
import {
  useGetVisibleNetworksQuery,
  useGetUserTokensRegistryQuery,
  useGetTransactionsQuery,
  useGetTokenSpotPricesQuery
} from '../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../../common/slices/constants'
import { useMultiChainSellAssets } from '../../../../common/hooks/use-multi-chain-sell-assets'

// Actions
import { AccountsTabActions } from '../../../../page/reducers/accounts-tab-reducer'

export const Account = () => {
  // routing
  const { id: accountId } = useParams<{ id: string }>()
  const { hash: transactionID } = useLocation()

  // redux
  const dispatch = useDispatch()

  // unsafe selectors
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const selectedAccount = React.useMemo(() => {
    return accounts.find(({ address }) =>
      address.toLowerCase() === accountId?.toLowerCase()
    )
  }, [accounts, accountId])

  // queries
  const { data: networkList = [] } = useGetVisibleNetworksQuery()
  const { userVisibleTokensInfo } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: result => ({
      userVisibleTokensInfo: selectAllUserAssetsFromQueryResult(result)
    })
  })
  const { data: unsortedTransactionList = [] } = useGetTransactionsQuery(
    selectedAccount
      ? {
          address: selectedAccount.address,
          chainId: null,
          coinType: selectedAccount.accountId.coin
        }
      : skipToken,
    { skip: !selectedAccount }
  )

  // safe selectors
  const assetAutoDiscoveryCompleted = useSafeWalletSelector(WalletSelectors.assetAutoDiscoveryCompleted)

  // state
  const [showSellModal, setShowSellModal] = React.useState<boolean>(false)

  // custom hooks
  const scrollIntoView = useScrollIntoView()

  const {
    allSellAssetOptions,
    getAllSellAssetOptions,
    selectedSellAsset,
    setSelectedSellAsset,
    sellAmount,
    setSellAmount,
    selectedSellAssetNetwork,
    openSellAssetLink,
    checkIsAssetSellSupported
  } = useMultiChainSellAssets()

  // memos
  const orb = React.useMemo(() => {
    if (selectedAccount) {
      return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
    }
  }, [selectedAccount])

  const transactionList = React.useMemo(() => {
    return sortTransactionByDate(unsortedTransactionList, 'descending')
  }, [unsortedTransactionList])

  const accountsTokensList = React.useMemo(() => {
    if (!selectedAccount) {
      return []
    }
    // Since LOCALHOST's chainId is shared between coinType's
    // this check will make sure we are returning the correct
    // LOCALHOST asset for each account.
    const hasLocalHostNetwork = networkList.some(
      (network) =>
        network.chainId === BraveWallet.LOCALHOST_CHAIN_ID
        && network.coin === selectedAccount.accountId.coin
    )
    const coinName = CoinTypesMap[selectedAccount.accountId.coin]
    const localHostCoins = userVisibleTokensInfo.filter((token) => token.chainId === BraveWallet.LOCALHOST_CHAIN_ID)
    const accountsLocalHost = localHostCoins.find((token) => token.symbol.toUpperCase() === coinName)
    const chainList = networkList.filter((network) => network.coin === selectedAccount.accountId.coin &&
      (network.coin !== BraveWallet.CoinType.FIL || getFilecoinKeyringIdFromNetwork(network) === selectedAccount.accountId.keyringId)).map((network) => network.chainId) ?? []
    const list =
      userVisibleTokensInfo.filter((token) => chainList.includes(token?.chainId ?? '') &&
        token.chainId !== BraveWallet.LOCALHOST_CHAIN_ID) ?? []
    if (
      accountsLocalHost &&
      hasLocalHostNetwork &&
      (
        selectedAccount.accountId.keyringId !==
        BraveWallet.KeyringId.kFilecoin
      )
    ) {
      return [...list, accountsLocalHost]
    }
    return list
  }, [userVisibleTokensInfo, selectedAccount, networkList])

  const nonFungibleTokens = React.useMemo(
    () =>
      accountsTokensList.filter(({ isErc721, isNft }) => isErc721 || isNft)
        .filter((token) => getBalance(selectedAccount, token) !== '0'),
    [accountsTokensList, selectedAccount]
  )

  const fungibleTokens = React.useMemo(
    () => accountsTokensList.filter(({ isErc721, isErc1155, isNft }) =>
      !(isErc721 || isErc1155 || isNft)),
    [accountsTokensList]
  )

  const {
    data: spotPriceRegistry,
    isLoading: isLoadingSpotPrices,
    isFetching: isFetchingSpotPrices
  } = useGetTokenSpotPricesQuery(
    {
      ids: fungibleTokens
        .filter(token => new Amount(getBalance(selectedAccount, token)).gt(0))
        .map(token => getPriceIdForToken(token)),
    },
    querySubscriptionOptions60s
  )

  const buttonOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    const filteredButtonOptions = AccountButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'details')
    // We are not able to remove a Derviced account so we filter out this option.
    if (selectedAccount?.accountId.kind === BraveWallet.AccountKind.kDerived) {
      return filteredButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'remove')
    }
    // We are not able to fetch Private Keys for a Hardware account so we filter out this option.
    if (selectedAccount?.accountId.kind === BraveWallet.AccountKind.kHardware) {
      return filteredButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'privateKey')
    }
    return filteredButtonOptions
  }, [selectedAccount])

  // methods
  const onRemoveAccount = React.useCallback(() => {
    if (selectedAccount) {
      dispatch(
        AccountsTabActions.setAccountToRemove({
          accountId: selectedAccount.accountId,
          name: selectedAccount.name
        })
      )
    }
  }, [selectedAccount, dispatch])

  const onClickButtonOption = React.useCallback((option: AccountButtonOptionsObjectType) => () => {
    if (option.id === 'remove') {
      onRemoveAccount()
      return
    }
    dispatch(AccountsTabActions.setShowAccountModal(true))
    dispatch(AccountsTabActions.setAccountModalType(option.id))
    dispatch(AccountsTabActions.setSelectedAccount(selectedAccount))
  }, [onRemoveAccount, dispatch])

  const checkIsTransactionFocused = React.useCallback((id: string): boolean => {
    if (transactionID !== '') {
      return transactionID.replace('#', '') === id
    }
    return false
  }, [transactionID])

  const handleScrollIntoView = React.useCallback((id: string, ref: HTMLDivElement | null) => {
    if (checkIsTransactionFocused(id)) {
      scrollIntoView(ref)
    }
  }, [checkIsTransactionFocused, scrollIntoView])

  const onClickShowSellModal = React.useCallback((token: BraveWallet.BlockchainToken) => {
    setShowSellModal(true)
    setSelectedSellAsset(token)
  }, [setSelectedSellAsset, setShowSellModal])

  const onOpenSellAssetLink = React.useCallback(() => {
    if (selectedAccount?.address) {
      openSellAssetLink({ sellAddress: selectedAccount.address, sellAsset: selectedSellAsset })
    }
  }, [selectedSellAsset, selectedAccount?.address, openSellAssetLink])

  // Effects
  React.useEffect(() => {
    if (allSellAssetOptions.length === 0) {
      getAllSellAssetOptions()
    }
  }, [allSellAssetOptions.length, getAllSellAssetOptions])

  // redirect (asset not found)
  if (!selectedAccount) {
    return <Redirect to={WalletRoutes.Accounts} />
  }

  // render
  return (
    <StyledWrapper>
      <WalletInfoLeftSide>
        <AccountCircle orb={orb} />
        <WalletName>{selectedAccount.name}</WalletName>
        <CopyTooltip text={selectedAccount.address}>
          <WalletAddress>{reduceAddress(selectedAccount.address)}</WalletAddress>
        </CopyTooltip>
      </WalletInfoLeftSide>

      <WalletInfoRow>
        <AccountButtonsRow>
          {buttonOptions.map((option) =>
            <AccountListItemOptionButton
              key={option.id}
              option={option}
              onClick={onClickButtonOption(option)}
            />
          )}
        </AccountButtonsRow>
      </WalletInfoRow>

      <Spacer />
      <Spacer />

      <ScrollableColumn>
        <Column fullWidth={true} alignItems='flex-start'>
          <SubviewSectionTitle>
            {getLocale('braveWalletAccountsAssets')}
          </SubviewSectionTitle>
          <SubDivider />
          <Spacer />
        </Column>

        {fungibleTokens.map((item) =>
          <PortfolioAssetItem
            key={`${item.contractAddress}-${item.symbol}-${item.chainId}`}
            assetBalance={getBalance(selectedAccount, item)}
            token={item}
            isAccountDetails={true}
            showSellModal={() => onClickShowSellModal(item)}
            isSellSupported={checkIsAssetSellSupported(item)}
            spotPrice={
              spotPriceRegistry && !isLoadingSpotPrices && !isFetchingSpotPrices
                ? getTokenPriceAmountFromRegistry(spotPriceRegistry, item)
                : Amount.empty()
            }
          />
        )}

        {!assetAutoDiscoveryCompleted &&
          <PortfolioAssetItemLoadingSkeleton />
        }

        <Spacer />

        {nonFungibleTokens?.length !== 0 &&
          <>
            <Column fullWidth={true} alignItems='flex-start'>
              <Spacer />
              <SubviewSectionTitle>
                {getLocale('braveWalletTopNavNFTS')}
              </SubviewSectionTitle>
              <SubDivider />
            </Column>
            {nonFungibleTokens?.map((item) =>
              <PortfolioAssetItem
                key={
                  `${item.contractAddress}-
                   ${item.symbol}-
                   ${item.chainId}-
                   ${item.tokenId}
                  `
                }
                assetBalance={getBalance(selectedAccount, item)}
                token={item}
                spotPrice={Amount.empty()}
              />
            )}
            <Spacer />
          </>
        }

        <Column fullWidth={true} alignItems='flex-start'>
          <Spacer />
          <SubviewSectionTitle>
            {getLocale('braveWalletTransactions')}
          </SubviewSectionTitle>
          <SubDivider />
        </Column>

        {transactionList.length !== 0 ? (
          <>
            {transactionList.map((transaction) =>
              <PortfolioTransactionItem
                key={transaction?.id}
                transaction={transaction}
                displayAccountName={false}
                ref={(ref) => handleScrollIntoView(transaction.id, ref)}
                isFocused={checkIsTransactionFocused(transaction.id)}
              />
            )}
          </>
        ) : (
          <TransactionPlaceholderContainer>
            <TransactionPlaceholderText>
              {getLocale('braveWalletTransactionPlaceholder')}
            </TransactionPlaceholderText>
          </TransactionPlaceholderContainer>
        )}
      </ScrollableColumn>
      {showSellModal && selectedSellAsset &&
        <SellAssetModal
          selectedAsset={selectedSellAsset}
          selectedAssetsNetwork={selectedSellAssetNetwork}
          onClose={() => setShowSellModal(false)}
          sellAmount={sellAmount}
          setSellAmount={setSellAmount}
          openSellAssetLink={onOpenSellAssetLink}
          showSellModal={showSellModal}
          account={selectedAccount}
          sellAssetBalance={getBalance(selectedAccount, selectedSellAsset)}
        />
      }
    </StyledWrapper>
  )
}

export default Account
