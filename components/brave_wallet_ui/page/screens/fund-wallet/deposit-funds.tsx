// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useParams } from 'react-router'

// utils
import { getLocale } from '../../../../common/locale'
import { generateQRCode } from '../../../utils/qr-code-utils'
import { makeNetworkAsset } from '../../../options/asset-options'
import {
  getBatTokensFromList,
  getAssetIdKey
} from '../../../utils/asset-utils'

// types
import {
  BraveWallet,
  CoinType,
  UserAssetInfoType,
  WalletState
} from '../../../constants/types'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'

// hooks
import { useCopyToClipboard } from '../../../common/hooks/use-copy-to-clipboard'
import { useScrollIntoView } from '../../../common/hooks/use-scroll-into-view'
import {
  useGetMainnetsQuery,
  useGetNetworkQuery
} from '../../../common/slices/api.slice'
import {
  useGetCombinedTokensListQuery
} from '../../../common/slices/api.slice.extra'

// style
import { Column, CopyButton, HorizontalSpace, LoadingIcon, Row, VerticalSpace } from '../../../components/shared/style'
import {
  Description,
  NextButtonRow,
  Title
} from '../onboarding/onboarding.style'
import {
  AddressText,
  AddressTextLabel,
  QRCodeImage,
  ScrollContainer,
  SearchWrapper,
  SelectAssetWrapper
} from './fund-wallet.style'

// components
import {
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
  FilterTokenRow //
} from '../../../components/desktop/views/portfolio/style'
import {
  NetworkFilterSelector //
} from '../../../components/desktop/network-filter-selector'

const itemSize = 82

function getItemSize (index: number): number {
  return itemSize
}

interface Props {
  showDepositAddress: boolean
  onShowDepositAddress: (showDepositAddress: boolean) => void
}

export const DepositFundsScreen = (props: Props) => {
  const { showDepositAddress, onShowDepositAddress } = props

  // routing
  const { tokenId } = useParams<{ tokenId?: string }>()

  // redux
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)
  const selectedNetworkFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetworkFilter)

  // custom hooks
  const { copyToClipboard, isCopied, resetCopyState } = useCopyToClipboard()
  const scrollIntoView = useScrollIntoView()

  // state
  const [showAccountSearch, setShowAccountSearch] = React.useState<boolean>(false)
  const [accountSearchText, setAccountSearchText] = React.useState<string>('')
  const [qrCode, setQRCode] = React.useState<string>('')
  const [selectedAsset, setSelectedAsset] = React.useState<
    BraveWallet.BlockchainToken | undefined
  >(undefined)
  const [selectedAccount, setSelectedAccount] =
    React.useState<BraveWallet.AccountInfo | undefined>()
  const [searchValue, setSearchValue] = React.useState<string>(tokenId ?? '')

  // queries
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()
  const { data: mainnetsList = [] } = useGetMainnetsQuery()
  const { data: selectedAssetNetwork } = useGetNetworkQuery(
    selectedAsset ?? skipToken
  )

  // memos
  const isNextStepEnabled = React.useMemo(() => !!selectedAsset, [selectedAsset])

  const mainnetNetworkAssetsList = React.useMemo(() => {
    return (mainnetsList || []).map(makeNetworkAsset)
  }, [mainnetsList])

  const fullAssetsList: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    // separate BAT from other tokens in the list so they can be placed higher in the list
    const { bat, nonBat } = getBatTokensFromList(combinedTokensList)
    return [
      ...mainnetNetworkAssetsList,
      ...bat,
      ...nonBat.filter((token) => token.contractAddress)
    ]
  }, [mainnetNetworkAssetsList, combinedTokensList])

  const assetsForFilteredNetwork: UserAssetInfoType[] = React.useMemo(() => {
    const assets = selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? fullAssetsList
      : fullAssetsList.filter(({ chainId }) => selectedNetworkFilter.chainId === chainId)

    return assets.map(asset => ({ asset, assetBalance: '1' }))
  }, [selectedNetworkFilter.chainId, fullAssetsList])

  const accountsForSelectedAssetNetwork = React.useMemo(() => {
    return selectedAssetNetwork
      ? accounts.filter(a => a.accountId.coin === selectedAssetNetwork.coin)
      : []
  }, [selectedAssetNetwork, accounts])

  const needsAccount: boolean = React.useMemo(() => {
    return !!selectedAsset && accountsForSelectedAssetNetwork.length < 1
  }, [selectedAsset, accountsForSelectedAssetNetwork.length])

  const accountListSearchResults = React.useMemo(() => {
    if (accountSearchText === '') {
      return accountsForSelectedAssetNetwork
    }

    return accountsForSelectedAssetNetwork.filter((item) => {
      return item.name.toLowerCase().startsWith(accountSearchText.toLowerCase())
    })
  }, [accountSearchText, accountsForSelectedAssetNetwork])

  const assetListSearchResults = React.useMemo(() => {
    if (searchValue === '') {
      return assetsForFilteredNetwork
    }
    return assetsForFilteredNetwork.filter((item) => {
      const searchValueLower = searchValue.toLowerCase()
      return (
        item.asset.name.toLowerCase().startsWith(searchValueLower) ||
        item.asset.symbol.toLowerCase().startsWith(searchValueLower)
      )
    })
  }, [
    searchValue,
    assetsForFilteredNetwork
  ])

  const depositTitleText: string = React.useMemo(() => {
    const isNativeAsset = (
      selectedAsset?.coin === CoinType.ETH &&
      !selectedAsset?.isErc20 &&
      !selectedAsset?.isErc721
    )
    const isFil = selectedAsset?.coin === CoinType.FIL
    const isSolOrSpl = selectedAsset?.coin === CoinType.SOL
    const isErc = selectedAsset?.isErc20 || selectedAsset?.isErc721

    // EVM native network (gas) assets & Filecoin
    if (isNativeAsset || isFil) {
      return getLocale('braveWalletDepositX').replace('$1', selectedAsset.symbol)
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
  const openAccountSearch = React.useCallback(() => setShowAccountSearch(true), [])
  const closeAccountSearch = React.useCallback(() => setShowAccountSearch(false), [])
  const onSearchTextChanged = React.useCallback((e: React.ChangeEvent<HTMLInputElement>) => setAccountSearchText(e.target.value), [])

  // This filters a list of assets when the user types in search bar
  const onSearchValueChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchValue(event.target.value)
    },
    []
  )

  const onSelectAccountFromSearch = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      closeAccountSearch()
      setSelectedAccount(account)
      resetCopyState()
    },
    [closeAccountSearch, resetCopyState]
  )

  const nextStep = React.useCallback(() => {
    if (!isNextStepEnabled || !selectedAssetNetwork) {
      return
    }
    onShowDepositAddress(true)
  }, [isNextStepEnabled, selectedAssetNetwork, onShowDepositAddress])

  const goBackToSelectAssets = React.useCallback(() => {
    onShowDepositAddress(false)
    resetCopyState()
    setSelectedAsset(undefined)
  }, [onShowDepositAddress, resetCopyState])

  const copyAddressToClipboard = React.useCallback(() => {
    copyToClipboard(selectedAccount?.address || '')
  }, [copyToClipboard, selectedAccount?.address])

  const onCopyKeyPress = React.useCallback(({ key }: React.KeyboardEvent) => {
    // Invoke for space or enter, just like a regular input or button
    if ([' ', 'Enter'].includes(key)) {
      copyAddressToClipboard()
    }
  }, [copyAddressToClipboard])

  const checkIsDepositAssetSelected = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    if (selectedAsset) {
      return selectedAsset.contractAddress.toLowerCase() === asset.contractAddress.toLowerCase() &&
        selectedAsset.symbol.toLowerCase() === asset.symbol.toLowerCase() &&
        selectedAsset.chainId === asset.chainId
    }
    return false
  }, [selectedAsset])

  const handleScrollIntoView = React.useCallback((asset: BraveWallet.BlockchainToken, ref: HTMLButtonElement | null) => {
    if (checkIsDepositAssetSelected(asset)) {
      scrollIntoView(ref)
    }
  }, [checkIsDepositAssetSelected, scrollIntoView])

  // effects
  React.useEffect(() => {
    let subscribed = true

    // fetch selected Account QR Code
    selectedAccount?.address && generateQRCode(selectedAccount.address).then(qr => {
      if (subscribed) {
        setQRCode(qr)
      }
    })

    // cleanup
    return () => {
      subscribed = false
    }
  }, [selectedAccount?.address])


  // sync default selected account with selected asset
  React.useEffect(() => {
    if (
      selectedAsset &&
      selectedAssetNetwork &&
      // asset is selected & account is available
      accountsForSelectedAssetNetwork.length &&
      // needs to change accounts to one with correct network
      selectedAccount?.accountId?.coin !== selectedAsset.coin
    ) {
      setSelectedAccount(accountsForSelectedAssetNetwork[0])
    }
  }, [
    selectedAsset,
    selectedAssetNetwork,
    accountsForSelectedAssetNetwork,
    selectedAccount?.accountId?.coin
  ])

  React.useEffect(() => {
    if (!showDepositAddress) {
      if (isCopied) {
        resetCopyState()
      }

      if (showAccountSearch) {
        closeAccountSearch()
      }
    }
  }, [showDepositAddress, showAccountSearch, isCopied, closeAccountSearch, resetCopyState])

  React.useEffect(() => {
    // reset search field on list update
    if (fullAssetsList && !tokenId) {
      setSearchValue('')
    }
  }, [fullAssetsList, tokenId])

  // render
  return (
    <>
      {/* Asset Selection */}
      {!showDepositAddress && (
        <>
          <SelectAssetWrapper>

            <FilterTokenRow horizontalPadding={0} isV2={false}>
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
              <NetworkFilterSelector isV2={false} />
            </FilterTokenRow>

            {fullAssetsList.length ? (
              <VirtualizedTokensList
                getItemSize={getItemSize}
                userAssetList={assetListSearchResults}
                estimatedItemSize={itemSize}
                renderToken={({ item: { asset } }) => (
                  <BuyAssetOptionItem
                    isSelected={checkIsDepositAssetSelected(asset)}
                    key={getAssetIdKey(asset)}
                    token={asset}
                    onClick={setSelectedAsset}
                    ref={(ref) => handleScrollIntoView(asset, ref)}
                  />
                )}
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
      )}

      {/* Creates wallet Account if needed for deposit */}
      {needsAccount && showDepositAddress && selectedAssetNetwork &&
        <CreateAccountTab
          network={selectedAssetNetwork}
          onCancel={goBackToSelectAssets}
        />
      }

      {/* Address display & Account selection/search */}
      {!needsAccount && showDepositAddress &&
        <>
          {!showAccountSearch &&
            <Column gap={'16px'}>

              <Column alignItems='flex-start'>
                <Title>{depositTitleText}</Title>

                {selectedAssetNetwork &&
                  <Description>
                    {
                      getLocale('braveWalletDepositOnlySendOnXNetwork')
                        .replace('$1', selectedAssetNetwork.chainName)
                    }
                  </Description>
                }
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
                <QRCodeImage src={qrCode} />
              </Row>

              <Column gap={'4px'}>

                <AddressTextLabel>Address:</AddressTextLabel>

                <Row gap={'12px'}>
                  <AddressText>{selectedAccount?.address}</AddressText>
                  <CopyButton
                    iconColor={'interactive05'}
                    onKeyPress={onCopyKeyPress}
                    onClick={copyAddressToClipboard}
                  />
                </Row>

                {isCopied && <CopiedToClipboardConfirmation />}
              </Column>

            </Column>
          }

          {showAccountSearch &&
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
          }
        </>
      }
    </>
  )
}

export default DepositFundsScreen
