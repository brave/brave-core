// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, useParams, useLocation } from 'react-router'
import { useDispatch } from 'react-redux'
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
import {
  parseTransactionWithPrices,
  sortTransactionByDate
} from '../../../../utils/tx-utils'
import { getBalance } from '../../../../utils/balance-utils'
import { getFilecoinKeyringIdFromNetwork, getNetworkFromTXDataUnion } from '../../../../utils/network-utils'
import { selectAllBlockchainTokensFromQueryResult, selectAllUserAssetsFromQueryResult } from '../../../../common/slices/entities/blockchain-token.entity'

// Styled Components
import {
  StyledWrapper,
  SubDivider,
  TopRow,
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
import { BackButton } from '../../../shared'
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
  useGetNetworksQuery,
  useGetTokensRegistryQuery,
  useGetUserTokensRegistryQuery
} from '../../../../common/slices/api.slice'
import { useMultiChainSellAssets } from '../../../../common/hooks/use-multi-chain-sell-assets'

// Actions
import { AccountsTabActions } from '../../../../page/reducers/accounts-tab-reducer'

export interface Props {
  goBack: () => void
}

export const Account = ({
  goBack
}: Props) => {
  // routing
  const { id: accountId } = useParams<{ id: string }>()
  const { hash: transactionID } = useLocation()

  // redux
  const dispatch = useDispatch()

  // unsafe selectors
  const userVisibleTokensInfo = useUnsafeWalletSelector(WalletSelectors.userVisibleTokensInfo)
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const transactions = useUnsafeWalletSelector(WalletSelectors.transactions)
  const solFeeEstimates = useUnsafeWalletSelector(WalletSelectors.solFeeEstimates)
  const spotPrices = useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)

  // queries
  const { data: networkList = [] } = useGetNetworksQuery()
  const { fullTokenList } = useGetTokensRegistryQuery(undefined, {
    selectFromResult: (result) => ({
      fullTokenList: selectAllBlockchainTokensFromQueryResult(result)
    })
  })
  const { userVisibleTokensList } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: result => ({
      userVisibleTokensList: selectAllUserAssetsFromQueryResult(result)
    })
  })

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
  const selectedAccount = React.useMemo(() => {
    return accounts.find(({ address }) =>
      address.toLowerCase() === accountId?.toLowerCase()
    )
  }, [accounts, accountId])

  const orb = React.useMemo(() => {
    if (selectedAccount) {
      return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
    }
  }, [selectedAccount])

  const transactionList = React.useMemo(() => {
    if (selectedAccount?.address && transactions[selectedAccount.address]) {
      return sortTransactionByDate(
        transactions[selectedAccount.address],
        'descending'
      ).map(tx => parseTransactionWithPrices({
        tx,
        accounts,
        fullTokenList,
        userVisibleTokensList,
        solFeeEstimates,
        transactionNetwork: getNetworkFromTXDataUnion(tx.txDataUnion, networkList),
        spotPrices
      }))
    } else {
      return []
    }
  }, [
    accounts,
    selectedAccount?.address,
    transactions,
    accounts,
    fullTokenList,
    userVisibleTokensList,
    solFeeEstimates,
    networkList,
    spotPrices
  ])

  const accountsTokensList = React.useMemo(() => {
    if (!selectedAccount) {
      return []
    }
    // Since LOCALHOST's chainId is shared between coinType's
    // this check will make sure we are returning the correct
    // LOCALHOST asset for each account.
    const coinName = CoinTypesMap[selectedAccount?.coin ?? 0]
    const localHostCoins = userVisibleTokensInfo.filter((token) => token.chainId === BraveWallet.LOCALHOST_CHAIN_ID)
    const accountsLocalHost = localHostCoins.find((token) => token.symbol.toUpperCase() === coinName)
    const chainList = networkList.filter((network) => network.coin === selectedAccount?.coin &&
      (network.coin !== BraveWallet.CoinType.FIL || getFilecoinKeyringIdFromNetwork(network) === selectedAccount?.keyringId)).map((network) => network.chainId) ?? []
    const list =
      userVisibleTokensInfo.filter((token) => chainList.includes(token?.chainId ?? '') &&
        token.chainId !== BraveWallet.LOCALHOST_CHAIN_ID) ?? []
    if (accountsLocalHost && (selectedAccount.keyringId !== BraveWallet.FILECOIN_KEYRING_ID)) {
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
    () => accountsTokensList.filter(({ isErc721, isNft }) => !(isErc721 || isNft)),
    [accountsTokensList]
  )

  const isHardwareWallet: boolean = React.useMemo(() => {
    return selectedAccount?.accountType === 'Trezor' || selectedAccount?.accountType === 'Ledger'
  }, [selectedAccount])

  const buttonOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    const filteredButtonOptions = AccountButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'details')
    // We are not able to remove a Primary account so we filter out this option.
    if (selectedAccount?.accountType === 'Primary') {
      return filteredButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'remove')
    }
    // We are not able to fetch Private Keys for a Hardware account so we filter out this option.
    if (isHardwareWallet) {
      return filteredButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'privateKey')
    }
    return filteredButtonOptions
  }, [selectedAccount, isHardwareWallet])

  // methods
  const onRemoveAccount = React.useCallback(() => {
    if (selectedAccount) {
      dispatch(AccountsTabActions.setAccountToRemove({ address: selectedAccount.address, hardware: isHardwareWallet, coin: selectedAccount.coin, name: selectedAccount.name }))
    }
  }, [selectedAccount, isHardwareWallet, dispatch])

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
      <TopRow>
        <BackButton onSubmit={goBack} />
      </TopRow>

      <WalletInfoRow>
        <WalletInfoLeftSide>
          <AccountCircle orb={orb} />
          <WalletName>{selectedAccount.name}</WalletName>
          <CopyTooltip text={selectedAccount.address}>
            <WalletAddress>{reduceAddress(selectedAccount.address)}</WalletAddress>
          </CopyTooltip>
        </WalletInfoLeftSide>
      </WalletInfoRow>

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
