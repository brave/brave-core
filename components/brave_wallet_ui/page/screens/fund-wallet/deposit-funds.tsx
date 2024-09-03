// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { Redirect, Route, Switch, useHistory, useParams } from 'react-router'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// utils
import { getLocale } from '../../../../common/locale'
import { makeNetworkAsset } from '../../../options/asset-options'
import {
  getAssetIdKey,
  sortNativeAndAndBatAssetsToTop,
  tokenNameToNftCollectionName
} from '../../../utils/asset-utils'
import {
  makeDepositFundsAccountRoute,
  makeDepositFundsRoute
} from '../../../utils/routes-utils'
import { networkSupportsAccount } from '../../../utils/network-utils'

// types
import {
  BraveWallet,
  NetworkFilterType,
  SupportedTestNetworks,
  WalletRoutes
} from '../../../constants/types'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'

// hooks
import { useCopyToClipboard } from '../../../common/hooks/use-copy-to-clipboard'
import {
  useGetNetworkQuery,
  useGetQrCodeImageQuery,
  useGetVisibleNetworksQuery
} from '../../../common/slices/api.slice'
import {
  useAccountsQuery,
  useGetCombinedTokensListQuery,
  useReceiveAddressQuery
} from '../../../common/slices/api.slice.extra'
import { useDebouncedCallback } from '../swap/hooks/useDebouncedCallback'

// style
import {
  Column,
  CopyButton,
  HorizontalSpace,
  LoadingIcon,
  Row,
  LeoSquaredButton
} from '../../../components/shared/style'
import { Description, Title } from '../onboarding/onboarding.style'
import {
  AddressText,
  AddressTextLabel,
  TokenListWrapper,
  QRCodeContainer,
  QRCodeImage,
  ScrollContainer,
  SearchWrapper,
  SelectAssetWrapper,
  SearchAndDropdownWrapper
} from './fund-wallet.style'
import {
  LoadingRing //
} from '../../../components/extension/add-suggested-token-panel/style'
import {
  FilterTokenRow //
} from '../../../components/desktop/views/portfolio/style'

// components
import {
  RenderTokenFunc,
  VirtualizedTokensList
} from '../../../components/desktop/views/portfolio/components/token-lists/virtualized-tokens-list'
import SearchBar from '../../../components/shared/search-bar/index'
import SelectAccountItem from '../../../components/shared/select-account-item/index'
import SelectAccount from '../../../components/shared/select-account/index'
import { BuyAssetOptionItem } from '../../../components/shared/buy-option/buy-asset-option'
import { CopiedToClipboardConfirmation } from '../../../components/desktop/copied-to-clipboard-confirmation/copied-to-clipboard-confirmation'
import CreateAccountTab from '../../../components/buy-send-swap/create-account/index'
import SelectHeader from '../../../components/buy-send-swap/select-header/index'
import {
  NetworkFilterSelector //
} from '../../../components/desktop/network-filter-selector'
import {
  WalletPageWrapper //
} from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import {
  PageTitleHeader //
} from '../../../components/desktop/card-headers/page-title-header'
import { Skeleton } from '../../../components/shared/loading-skeleton/styles'

interface Props {
  isAndroid?: boolean
}

interface Params {
  assetId: string
}

export const DepositFundsScreen = ({ isAndroid }: Props) => {
  // routing
  const history = useHistory()

  // render
  return (
    <Switch>
      <Route
        path={WalletRoutes.DepositFundsAccountPage}
        exact
      >
        <WalletPageWrapper
          hideNav={isAndroid}
          hideHeader={isAndroid}
          wrapContentInBox={true}
          cardHeader={
            <PageTitleHeader
              title={getLocale('braveWalletDepositCryptoButton')}
              onBack={history.goBack}
            />
          }
        >
          <DepositAccount />
        </WalletPageWrapper>
      </Route>

      <Route path={WalletRoutes.DepositFundsPage}>
        <WalletPageWrapper
          hideNav={isAndroid}
          hideHeader={isAndroid}
          wrapContentInBox={true}
          useFullHeight={true}
          cardHeader={
            <PageTitleHeader
              title={getLocale('braveWalletDepositCryptoButton')}
              expandRoute={WalletRoutes.DepositFundsPage}
            />
          }
        >
          <AssetSelection />
        </WalletPageWrapper>
      </Route>

      <Redirect to={WalletRoutes.DepositFundsPage} />
    </Switch>
  )
}

function AssetSelection() {
  // routing
  const history = useHistory()
  const { assetId: selectedDepositAssetId } = useParams<Params>()
  const params = new URLSearchParams(history.location.search)
  const searchParam = params.get('search')
  const chainIdParam = params.get('chainId')
  const coinTypeParam = params.get('coinType')

  // redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // state
  const [searchValue, setSearchValue] = React.useState<string>(
    searchParam ?? ''
  )
  const [selectedNetworkFilter, setSelectedNetworkFilter] =
    React.useState<NetworkFilterType>(
      chainIdParam && coinTypeParam !== null
        ? {
            chainId: chainIdParam,
            coin: Number(coinTypeParam)
          }
        : AllNetworksOption
    )

  // queries
  const { data: selectedNetworkFromFilter = AllNetworksOption } =
    useGetNetworkQuery(
      !selectedNetworkFilter ||
        selectedNetworkFilter.chainId === AllNetworksOption.chainId
        ? skipToken
        : selectedNetworkFilter
    )
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()
  const selectedAsset = combinedTokensList.find(
    (token) => getAssetIdKey(token) === selectedDepositAssetId
  )

  const { data: visibleNetworks = [] } = useGetVisibleNetworksQuery()

  // computed
  const isNextStepEnabled = !!selectedAsset

  // memos
  const {
    mainnetNetworkAssetsList,
    testnetAssetsList,
    mainnetNetworkAssetsListIds,
    testnetAssetsListIds
  } = React.useMemo(() => {
    const mainnets = []
    const testnets = []

    // categorize networks
    for (const net of visibleNetworks) {
      if (SupportedTestNetworks.includes(net.chainId)) {
        testnets.push(net)
      } else {
        mainnets.push(net)
      }
    }

    // make assets
    const mainnetNetworkAssetsList = mainnets.map(makeNetworkAsset)
    const testnetAssetsList = testnets.map(makeNetworkAsset)

    // get asset ids
    const mainnetNetworkAssetsListIds =
      mainnetNetworkAssetsList.map(getAssetIdKey)
    const testnetAssetsListIds = testnetAssetsList.map(getAssetIdKey)

    return {
      mainnetNetworkAssetsList,
      testnetAssetsList,
      mainnetNetworkAssetsListIds,
      testnetAssetsListIds
    }
  }, [visibleNetworks])

  // Combine all NFTs from each collection
  // into a single "asset" for depositing purposes.
  const { nftCollectionAssets, nftCollectionAssetsIds } = React.useMemo(() => {
    const nftCollectionAssets: BraveWallet.BlockchainToken[] = []
    const nftCollectionAssetsIds: string[] = []
    for (const token of combinedTokensList) {
      if (
        token.isNft &&
        !nftCollectionAssets.find(
          (t) =>
            t.contractAddress === token.contractAddress &&
            t.symbol === token.symbol
        )
      ) {
        const collectionToken = {
          ...token,
          tokenId: '',
          // Remove the token id from the token name
          name: tokenNameToNftCollectionName(token)
        }
        nftCollectionAssets.push(collectionToken)
        nftCollectionAssetsIds.push(getAssetIdKey(collectionToken))
      }
    }
    return { nftCollectionAssets, nftCollectionAssetsIds }
  }, [combinedTokensList])

  // removes pre-categorized assets from combined list
  const tokensList = React.useMemo(() => {
    return combinedTokensList.filter((t) => {
      const id = getAssetIdKey(t)
      return (
        !mainnetNetworkAssetsListIds.includes(id) &&
        !testnetAssetsListIds.includes(id) &&
        !nftCollectionAssetsIds.includes(id)
      )
    })
  }, [
    combinedTokensList,
    mainnetNetworkAssetsListIds,
    testnetAssetsListIds,
    nftCollectionAssetsIds
  ])

  const fullAssetsList: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    // separate BAT from other tokens in the list so they can be placed higher
    // in the list
    const sortedFungibleAssets = sortNativeAndAndBatAssetsToTop(
      tokensList
    ).filter((token) => token.contractAddress && !token.tokenId)
    return mainnetNetworkAssetsList.concat(
      sortedFungibleAssets,
      testnetAssetsList,
      nftCollectionAssets
    )
  }, [
    mainnetNetworkAssetsList,
    tokensList,
    nftCollectionAssets,
    testnetAssetsList
  ])

  const assetsForFilteredNetwork = React.useMemo(() => {
    const assets =
      selectedNetworkFilter.chainId === AllNetworksOption.chainId
        ? fullAssetsList
        : fullAssetsList.filter(
            ({ chainId }) => selectedNetworkFilter.chainId === chainId
          )

    return assets
  }, [selectedNetworkFilter.chainId, fullAssetsList])

  const assetListSearchResults = React.useMemo(() => {
    if (searchValue === '') {
      return assetsForFilteredNetwork
    }
    return assetsForFilteredNetwork.filter((asset) => {
      const searchValueLower = searchValue.toLowerCase()
      return (
        asset.name.toLowerCase().includes(searchValueLower) ||
        asset.symbol.toLowerCase().includes(searchValueLower)
      )
    })
  }, [searchValue, assetsForFilteredNetwork])

  // methods
  // This filters a list of assets when the user types in search bar
  const onSearchValueChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchValue(event.target.value)
    },
    []
  )

  const nextStep = React.useCallback(() => {
    if (!selectedDepositAssetId) {
      return
    }

    const searchValueLower = searchValue.toLowerCase()

    // save latest form values in router history
    history.replace(
      makeDepositFundsRoute(selectedDepositAssetId, {
        // save latest search-box value (if it matches selection name or symbol)
        searchText:
          searchValue &&
          (selectedAsset?.name.toLowerCase().startsWith(searchValueLower) ||
            selectedAsset?.symbol.toLowerCase().startsWith(searchValueLower))
            ? searchValue
            : undefined,
        // saving network filter (if it matches selection)
        chainId:
          selectedAsset?.chainId === selectedNetworkFilter.chainId
            ? selectedNetworkFilter.chainId || AllNetworksOption.chainId
            : AllNetworksOption.chainId,
        coinType:
          selectedAsset?.coin === selectedNetworkFilter.coin
            ? selectedNetworkFilter.coin.toString() ||
              AllNetworksOption.coin.toString()
            : AllNetworksOption.coin.toString()
      })
    )

    history.push(makeDepositFundsAccountRoute(selectedDepositAssetId))
  }, [
    selectedDepositAssetId,
    searchValue,
    history,
    selectedAsset,
    selectedNetworkFilter
  ])

  const renderToken = React.useCallback<
    RenderTokenFunc<BraveWallet.BlockchainToken>
  >(
    ({ item: asset, ref }) => {
      const assetId = getAssetIdKey(asset)
      return (
        <BuyAssetOptionItem
          key={assetId}
          token={asset}
          onClick={() => history.push(makeDepositFundsRoute(assetId))}
          ref={ref}
        />
      )
    },
    [history]
  )

  // render
  return (
    <>
      <SelectAssetWrapper
        fullWidth={true}
        fullHeight={true}
        justifyContent='flex-start'
      >
        <FilterTokenRow
          horizontalPadding={12}
          isV2={false}
        >
          <SearchAndDropdownWrapper alignItems='flex-start'>
            <SearchBar
              placeholder={getLocale('braveWalletSearchText')}
              action={onSearchValueChange}
              value={searchValue}
              isV2={false}
            />
          </SearchAndDropdownWrapper>
          <NetworkFilterSelector
            isV2={false}
            selectedNetwork={selectedNetworkFromFilter}
            onSelectNetwork={setSelectedNetworkFilter}
            dropdownPosition='right'
          />
        </FilterTokenRow>

        {fullAssetsList.length ? (
          <TokenListWrapper
            fullWidth={true}
            justifyContent='flex-start'
          >
            <VirtualizedTokensList
              userAssetList={assetListSearchResults}
              selectedAssetId={selectedDepositAssetId}
              renderToken={renderToken}
            />
          </TokenListWrapper>
        ) : (
          <Column>
            <LoadingIcon
              opacity={1}
              size={'100px'}
              color={'interactive05'}
            />
          </Column>
        )}
      </SelectAssetWrapper>

      <Row
        width='unset'
        padding='20px 0px 0px 0px'
      >
        <LeoSquaredButton
          onClick={nextStep}
          isDisabled={!isNextStepEnabled}
          size={isPanel ? 'medium' : 'large'}
        >
          {selectedAsset
            ? getLocale('braveWalletButtonContinue')
            : getLocale('braveWalletBuySelectAsset')}
        </LeoSquaredButton>
      </Row>
    </>
  )
}

function DepositAccount() {
  // routing
  const history = useHistory()
  const { assetId: selectedDepositAssetId } = useParams<Params>()

  // queries
  const { accounts } = useAccountsQuery()
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()
  const selectedAsset = combinedTokensList.find(
    (token) => getAssetIdKey(token) === selectedDepositAssetId
  )
  const { data: selectedAssetNetwork } = useGetNetworkQuery(
    selectedAsset ?? skipToken
  )
  const accountsForSelectedAssetCoinType = React.useMemo(() => {
    return selectedAssetNetwork
      ? accounts.filter((a) =>
          networkSupportsAccount(selectedAssetNetwork, a.accountId)
        )
      : []
  }, [selectedAssetNetwork, accounts])

  // search
  const [showAccountSearch, setShowAccountSearch] =
    React.useState<boolean>(false)
  const [accountSearchText, setAccountSearchText] = React.useState<string>('')

  // selected account
  const [selectedAccount, setSelectedAccount] = React.useState<
    BraveWallet.AccountInfo | undefined
  >(accountsForSelectedAssetCoinType[0])
  const { receiveAddress, isFetchingAddress } = useReceiveAddressQuery(
    selectedAccount?.accountId
  )
  const { data: qrCode, isFetching: isLoadingQrCode } = useGetQrCodeImageQuery(
    receiveAddress || skipToken
  )

  // custom hooks
  const { copyToClipboard, isCopied, resetCopyState } = useCopyToClipboard()

  // memos & computed
  const needsAccount =
    !!selectedAsset && accountsForSelectedAssetCoinType.length < 1

  const accountListSearchResults = React.useMemo(() => {
    if (accountSearchText === '') {
      return accountsForSelectedAssetCoinType
    }

    return accountsForSelectedAssetCoinType.filter((item) => {
      return item.name.toLowerCase().includes(accountSearchText.toLowerCase())
    })
  }, [accountSearchText, accountsForSelectedAssetCoinType])

  const depositTitleText: string = React.useMemo(() => {
    const isNativeAsset =
      selectedAsset?.coin === BraveWallet.CoinType.ETH &&
      !selectedAsset?.isErc20 &&
      !selectedAsset?.isErc721
    const isFil = selectedAsset?.coin === BraveWallet.CoinType.FIL
    const isSolOrSpl = selectedAsset?.coin === BraveWallet.CoinType.SOL
    const isErc = selectedAsset?.isErc20 || selectedAsset?.isErc721

    // EVM native network (gas) assets & Filecoin
    if (isNativeAsset || isFil) {
      return getLocale('braveWalletDepositX').replace(
        '$1',
        selectedAsset.symbol
      )
    }

    // ERC-based tokens
    if (isErc) {
      return getLocale('braveWalletDepositErc')
    }

    // Solana assets
    if (isSolOrSpl) {
      return getLocale('braveWalletDepositSolSplTokens')
    }

    return ''
  }, [selectedAsset])

  // methods
  const openAccountSearch = React.useCallback(
    () => setShowAccountSearch(true),
    []
  )
  const closeAccountSearch = React.useCallback(
    () => setShowAccountSearch(false),
    []
  )
  const _onSearchTextChanged = React.useCallback(
    (e: React.ChangeEvent<HTMLInputElement>) =>
      setAccountSearchText(e.target.value),
    []
  )
  const onSearchTextChanged = useDebouncedCallback(_onSearchTextChanged, 250)

  const onSelectAccountFromSearch = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      closeAccountSearch()
      setSelectedAccount(account)
      resetCopyState()
    },
    [closeAccountSearch, resetCopyState]
  )

  const copyAddressToClipboard = React.useCallback(() => {
    copyToClipboard(receiveAddress || '')
  }, [copyToClipboard, receiveAddress])

  const onCopyKeyPress = React.useCallback(
    ({ key }: React.KeyboardEvent) => {
      // Invoke for space or enter, just like a regular input or button
      if ([' ', 'Enter'].includes(key)) {
        copyAddressToClipboard()
      }
    },
    [copyAddressToClipboard]
  )

  // effects
  React.useEffect(() => {
    // force selected account option state
    setSelectedAccount(accountsForSelectedAssetCoinType[0])
  }, [accountsForSelectedAssetCoinType])

  // render
  if (!selectedDepositAssetId) {
    return <Redirect to={WalletRoutes.DepositFundsPageStart} />
  }

  /* Creates wallet Account if needed for deposit */
  if (needsAccount && selectedAssetNetwork) {
    return (
      <CreateAccountTab
        network={selectedAssetNetwork}
        onCreated={setSelectedAccount}
        onCancel={() => {
          resetCopyState()
          history.push(WalletRoutes.DepositFundsPage)
        }}
      />
    )
  }

  if (!selectedAccount || showAccountSearch) {
    return (
      <Column
        padding='0 12px'
        fullWidth
      >
        <SearchWrapper>
          <SelectHeader
            title={getLocale('braveWalletSelectAccount')}
            onBack={closeAccountSearch}
            hasAddButton={false}
          />
          <SearchBar
            placeholder={getLocale('braveWalletSearchAccount')}
            action={onSearchTextChanged}
          />
          <ScrollContainer>
            <SelectAccount
              accounts={accountListSearchResults}
              selectedAccount={selectedAccount}
              onSelectAccount={onSelectAccountFromSearch}
            />
          </ScrollContainer>
        </SearchWrapper>
      </Column>
    )
  }

  return (
    <Column
      gap={'16px'}
      padding='0 12px'
    >
      <Column alignItems='flex-start'>
        <Title>{depositTitleText}</Title>

        {selectedAssetNetwork && (
          <Description>
            {getLocale('braveWalletDepositOnlySendOnXNetwork').replace(
              '$1',
              selectedAssetNetwork.chainName
            )}
          </Description>
        )}
      </Column>

      <Row>
        <HorizontalSpace space={'63%'} />
        <SelectAccountItem
          selectedNetwork={selectedAssetNetwork}
          account={selectedAccount}
          onSelectAccount={openAccountSearch}
          showTooltips
          hideAddress
          showSwitchAccountsIcon
        />
        <HorizontalSpace space={'45%'} />
      </Row>

      <Column gap={'4px'}>
        <AddressTextLabel>
          {getLocale('braveWalletAddress')}
          {':'}
        </AddressTextLabel>

        {receiveAddress && !isFetchingAddress ? (
          <>
            <Row gap={'12px'}>
              <AddressText>{receiveAddress}</AddressText>
              <CopyButton
                iconColor={'interactive05'}
                onKeyPress={onCopyKeyPress}
                onClick={copyAddressToClipboard}
              />
            </Row>

            {isCopied && <CopiedToClipboardConfirmation />}
          </>
        ) : (
          <Skeleton
            height={'20px'}
            width={'300px'}
          />
        )}

        <Row>
          <QRCodeContainer>
            {isLoadingQrCode || !receiveAddress || isFetchingAddress ? (
              <LoadingRing />
            ) : (
              <QRCodeImage src={qrCode} />
            )}
          </QRCodeContainer>
        </Row>
      </Column>
    </Column>
  )
}

export default DepositFundsScreen
