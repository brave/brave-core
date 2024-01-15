// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { Redirect, Route, Switch, useHistory } from 'react-router'

// utils
import { getLocale } from '../../../../common/locale'
import { makeNetworkAsset } from '../../../options/asset-options'
import { getBatTokensFromList, getAssetIdKey } from '../../../utils/asset-utils'
import { WalletActions } from '../../../common/slices/wallet.slice'
import { WalletSelectors } from '../../../common/selectors'
import { makeDepositFundsRoute } from '../../../utils/routes-utils'
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
import { useSafeWalletSelector } from '../../../common/hooks/use-safe-selector'
import { useScrollIntoView } from '../../../common/hooks/use-scroll-into-view'
import { useDebouncedCallback } from '../swap/hooks/useDebouncedCallback'

// style
import {
  Column,
  CopyButton,
  HorizontalSpace,
  LoadingIcon,
  Row,
  VerticalSpace
} from '../../../components/shared/style'
import {
  Description,
  NextButtonRow,
  Title
} from '../onboarding/onboarding.style'
import {
  AddressText,
  AddressTextLabel,
  QRCodeContainer,
  QRCodeImage,
  ScrollContainer,
  SearchWrapper,
  SelectAssetWrapper
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
import { NavButton } from '../../../components/extension/buttons/nav-button/index'
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

const itemSize = 82

function getItemSize(index: number): number {
  return itemSize
}

const getItemKey = (i: number, data: BraveWallet.BlockchainToken[]) =>
  getAssetIdKey(data[i])

interface Props {
  isAndroid?: boolean
}

export const DepositFundsScreen = ({ isAndroid }: Props) => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // clear selected asset on page mount
  React.useEffect(() => {
    dispatch(WalletActions.selectOnRampAssetId(undefined))
  }, [])

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
          cardWidth={456}
          cardHeader={
            <PageTitleHeader
              title={getLocale('braveWalletDepositCryptoButton')}
              showBackButton
              onBack={() => {
                dispatch(WalletActions.selectOnRampAssetId(undefined))
                history.goBack()
              }}
            />
          }
        >
          <DepositAccount />
        </WalletPageWrapper>
      </Route>

      <Route>
        <WalletPageWrapper
          hideNav={isAndroid}
          hideHeader={isAndroid}
          wrapContentInBox={true}
          cardWidth={456}
          cardHeader={
            <PageTitleHeader
              title={getLocale('braveWalletDepositCryptoButton')}
            />
          }
        >
          <AssetSelection />
        </WalletPageWrapper>
      </Route>
    </Switch>
  )
}

function AssetSelection() {
  // routing
  const history = useHistory()
  const params = new URLSearchParams(history.location.search)
  const searchParam = params.get('search')
  const chainIdParam = params.get('chainId')
  const coinTypeParam = params.get('coinType')

  // custom hooks
  const scrollIntoView = useScrollIntoView()

  // redux
  const dispatch = useDispatch()
  const selectedDepositAssetId = useSafeWalletSelector(
    WalletSelectors.selectedOnRampAssetId
  )

  // refs
  const listItemRefs = React.useRef<Map<string, HTMLButtonElement> | null>(null)

  const getRefsMap = React.useCallback(
    function () {
      if (!listItemRefs.current) {
        // Initialize the Map on first usage.
        listItemRefs.current = new Map()
      }
      return listItemRefs.current
    },
    [listItemRefs]
  )

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
  const { mainnetNetworkAssetsList, testnetAssetsList } = React.useMemo(() => {
    const mainnetNetworkAssetsList = []
    const testnetAssetsList = []
    for (const net of visibleNetworks) {
      if (SupportedTestNetworks.includes(net.chainId)) {
        testnetAssetsList.push(net)
      } else {
        mainnetNetworkAssetsList.push(net)
      }
    }
    return {
      mainnetNetworkAssetsList: mainnetNetworkAssetsList.map(makeNetworkAsset),
      testnetAssetsList: testnetAssetsList.map(makeNetworkAsset)
    }
  }, [visibleNetworks])

  // Combine all NFTs from each collection
  // into a single "asset" for depositing purposes.
  const nftCollectionAssets: BraveWallet.BlockchainToken[] =
    React.useMemo(() => {
      const nftContractTokens: BraveWallet.BlockchainToken[] = []
      for (const token of combinedTokensList) {
        if (
          token.isNft &&
          !nftContractTokens.find(
            (t) =>
              t.contractAddress === token.contractAddress &&
              t.name === token.name &&
              t.symbol === token.symbol
          )
        ) {
          nftContractTokens.push({
            ...token,
            tokenId: '' // remove token ID
          })
        }
      }
      return nftContractTokens
    }, [combinedTokensList])

  // removes pre-categorized assets from combined list
  const tokensList = React.useMemo(() => {
    const mainnetNetworkAssetsListIds = mainnetNetworkAssetsList.map((t) =>
      getAssetIdKey(t)
    )
    const testnetAssetsListIds = testnetAssetsList.map((t) => getAssetIdKey(t))
    const nftCollectionAssetsIds = nftCollectionAssets.map((t) =>
      getAssetIdKey(t)
    )

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
    nftCollectionAssets,
    mainnetNetworkAssetsList,
    testnetAssetsList
  ])

  const fullAssetsList: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    // separate BAT from other tokens in the list so they can be placed higher
    // in the list
    const { bat, nonBat } = getBatTokensFromList(tokensList)
    return [
      ...mainnetNetworkAssetsList,
      ...bat,
      ...nonBat.filter((token) => token.contractAddress && !token.tokenId),
      ...testnetAssetsList,
      ...nftCollectionAssets
    ]
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
    const searchValueLower = searchValue.toLowerCase()

    // save latest form values in router history
    history.replace(
      makeDepositFundsRoute(
        // save latest search-box value (if it matches selection name or symbol)
        searchValue &&
          (selectedAsset?.name.toLowerCase().startsWith(searchValueLower) ||
            selectedAsset?.symbol.toLowerCase().startsWith(searchValueLower))
          ? searchValue
          : undefined,
        // saving network filter (if it matches selection)
        selectedAsset?.chainId === selectedNetworkFilter.chainId
          ? selectedNetworkFilter.chainId || AllNetworksOption.chainId
          : AllNetworksOption.chainId,
        selectedAsset?.coin === selectedNetworkFilter.coin
          ? selectedNetworkFilter.coin.toString() ||
              AllNetworksOption.coin.toString()
          : AllNetworksOption.coin.toString()
      )
    )

    history.push(WalletRoutes.DepositFundsAccountPage)
  }, [history, selectedNetworkFilter, searchValue, selectedAsset])

  const renderToken = React.useCallback<
    RenderTokenFunc<BraveWallet.BlockchainToken>
  >(
    ({ item: asset }) => {
      const assetId = getAssetIdKey(asset)
      return (
        <BuyAssetOptionItem
          ref={(node) => {
            const refs = getRefsMap()
            if (node) {
              refs.set(assetId, node)
            } else {
              refs.delete(assetId)
            }
          }}
          key={assetId}
          token={asset}
          onClick={() => dispatch(WalletActions.selectOnRampAssetId(assetId))}
        />
      )
    },
    [dispatch, getRefsMap]
  )

  // effects
  React.useEffect(() => {
    // scroll selected item into view
    if (selectedDepositAssetId) {
      const ref = getRefsMap().get(selectedDepositAssetId)
      if (ref) {
        scrollIntoView(ref, true)
      }
    }
  }, [selectedDepositAssetId, getRefsMap, scrollIntoView])

  // render
  return (
    <>
      <SelectAssetWrapper>
        <FilterTokenRow
          horizontalPadding={0}
          isV2={false}
        >
          <Column
            flex={1}
            style={{ minWidth: '25%' }}
            alignItems='flex-start'
          >
            <SearchBar
              placeholder={getLocale('braveWalletSearchText')}
              action={onSearchValueChange}
              value={searchValue}
              isV2={false}
            />
          </Column>
          <NetworkFilterSelector
            isV2={false}
            selectedNetwork={selectedNetworkFromFilter}
            onSelectNetwork={setSelectedNetworkFilter}
          />
        </FilterTokenRow>

        {fullAssetsList.length ? (
          <VirtualizedTokensList
            getItemKey={getItemKey}
            getItemSize={getItemSize}
            userAssetList={assetListSearchResults}
            estimatedItemSize={itemSize}
            renderToken={renderToken}
          />
        ) : (
          <Column>
            <LoadingIcon
              opacity={1}
              size={'100px'}
              color={'interactive05'}
            />
          </Column>
        )}

        <VerticalSpace space={'12px'} />
      </SelectAssetWrapper>

      <NextButtonRow>
        <NavButton
          buttonType={'primary'}
          text={
            selectedAsset
              ? getLocale('braveWalletButtonContinue')
              : getLocale('braveWalletBuySelectAsset')
          }
          onSubmit={nextStep}
          disabled={!isNextStepEnabled}
        />
      </NextButtonRow>
    </>
  )
}

function DepositAccount() {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const selectedDepositAssetId = useSafeWalletSelector(
    WalletSelectors.selectedOnRampAssetId
  )

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
  const receiveAddress = useReceiveAddressQuery(selectedAccount?.accountId)
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
  }, [accountsForSelectedAssetCoinType[0]])

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
          dispatch(WalletActions.selectOnRampAssetId(undefined))
          history.push(WalletRoutes.DepositFundsPage)
        }}
      />
    )
  }

  if (!selectedAccount || showAccountSearch) {
    return (
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
    )
  }

  return (
    <Column gap={'16px'}>
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

      <Row>
        <QRCodeContainer>
          {isLoadingQrCode || !receiveAddress ? (
            <LoadingRing />
          ) : (
            <QRCodeImage src={qrCode} />
          )}
        </QRCodeContainer>
      </Row>

      <Column gap={'4px'}>
        <AddressTextLabel>Address:</AddressTextLabel>

        {receiveAddress ? (
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
      </Column>
    </Column>
  )
}

export default DepositFundsScreen
