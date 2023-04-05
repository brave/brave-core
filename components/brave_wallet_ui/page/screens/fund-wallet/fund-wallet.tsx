// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import {
  useSelector
} from 'react-redux'

// utils
import { getLocale } from '../../../../common/locale'
import { getAssetIdKey } from '../../../utils/asset-utils'

// types
import {
  BraveWallet,
  UserAssetInfoType,
  WalletAccountType,
  WalletState
} from '../../../constants/types'
import { RenderTokenFunc } from '../../../components/desktop/views/portfolio/components/token-lists/virtualized-tokens-list'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'
import { SelectBuyOption } from '../../../components/buy-send-swap/select-buy-option/select-buy-option'

// hooks
import { usePrevNetwork } from '../../../common/hooks/previous-network'
import { useMultiChainBuyAssets } from '../../../common/hooks/use-multi-chain-buy-assets'
import { useScrollIntoView } from '../../../common/hooks/use-scroll-into-view'
import { useGetNetworkQuery } from '../../../common/slices/api.slice'

// style
import { Column, Flex, LoadingIcon, Row, VerticalSpace } from '../../../components/shared/style'
import {
  Description,
  NextButtonRow,
  Title
} from '../onboarding/onboarding.style'
import {
  ScrollContainer,
  SearchWrapper,
  SelectAssetWrapper
} from './fund-wallet.style'

// components
import SearchBar from '../../../components/shared/search-bar'
import SelectAccountItem from '../../../components/shared/select-account-item'
import SelectAccount from '../../../components/shared/select-account'
import { BuyAssetOptionItem } from '../../../components/shared/buy-option/buy-asset-option'
import { StepsNavigation } from '../../../components/desktop/steps-navigation/steps-navigation'
import { TokenLists } from '../../../components/desktop/views/portfolio/components/token-lists/token-list'
import { NavButton } from '../../../components/extension/buttons/nav-button/index'
import CreateAccountTab from '../../../components/buy-send-swap/create-account'
import SwapInputComponent from '../../../components/buy-send-swap/swap-input-component'
import SelectHeader from '../../../components/buy-send-swap/select-header'
import { SelectCurrency } from '../../../components/buy-send-swap/select-currency/select-currency'

export const FundWalletScreen = () => {
  // routing
  const history = useHistory()

  // redux
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)
  const defaultCurrencies = useSelector(({ wallet }: { wallet: WalletState }) => wallet.defaultCurrencies)
  const selectedNetworkFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetworkFilter)

  // queries
  const { data: selectedNetworkFromFilter = AllNetworksOption } =
    useGetNetworkQuery(selectedNetworkFilter, {
      skip:
        !selectedNetworkFilter ||
        selectedNetworkFilter.chainId === AllNetworksOption.chainId
    })

  // custom hooks
  const { prevNetwork } = usePrevNetwork()
  const {
    allAssetOptions: allBuyAssetOptions,
    buyAmount,
    buyAssetNetworks,
    getAllBuyOptionsAllChains,
    openBuyAssetLink,
    selectedAsset,
    selectedAssetBuyOptions,
    selectedAssetNetwork,
    setBuyAmount,
    setSelectedAsset
  } = useMultiChainBuyAssets()
  const scrollIntoView = useScrollIntoView()

  // state
  const [showBuyOptions, setShowBuyOptions] = React.useState<boolean>(false)
  const [showFiatSelection, setShowFiatSelection] = React.useState<boolean>(false)
  const [showAccountSearch, setShowAccountSearch] = React.useState<boolean>(false)
  const [accountSearchText, setAccountSearchText] = React.useState<string>('')
  const [selectedCurrency, setSelectedCurrency] = React.useState<string>(defaultCurrencies.fiat || 'usd')
  const [selectedAccount, setSelectedAccount] = React.useState<WalletAccountType | undefined>()

  // memos & computed
  const isNextStepEnabled = !!selectedAsset

  const selectedNetwork = selectedAssetNetwork || selectedNetworkFromFilter

  const assetsForFilteredNetwork: UserAssetInfoType[] = React.useMemo(() => {
    const assets = selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? allBuyAssetOptions
      : allBuyAssetOptions.filter(({ chainId }) =>
        selectedNetworkFilter.chainId === chainId
      )

    return assets.map(asset => ({ asset, assetBalance: '1' }))
  }, [selectedNetworkFilter.chainId, allBuyAssetOptions])

  const accountsForSelectedAssetNetwork: WalletAccountType[] = React.useMemo(() => {
    return selectedAssetNetwork
      ? accounts.filter(a => a.coin === selectedAssetNetwork.coin)
      : []
  }, [selectedAssetNetwork, accounts])

  const needsAccount: boolean =
    !!selectedAsset && accountsForSelectedAssetNetwork.length < 1

  const accountListSearchResults: WalletAccountType[] = React.useMemo(() => {
    if (accountSearchText === '') {
      return accountsForSelectedAssetNetwork
    }

    return accountsForSelectedAssetNetwork.filter((item) => {
      return item.name.toLowerCase().startsWith(accountSearchText.toLowerCase())
    })
  }, [accountSearchText, accountsForSelectedAssetNetwork])

  const networksFilterOptions: BraveWallet.NetworkInfo[] = React.useMemo(() => {
    return [AllNetworksOption, ...buyAssetNetworks]
  }, [buyAssetNetworks])

  // methods
  const openAccountSearch = React.useCallback(() => setShowAccountSearch(true), [])
  const closeAccountSearch = React.useCallback(() => setShowAccountSearch(false), [])
  const onSearchTextChanged = React.useCallback((e: React.ChangeEvent<HTMLInputElement>) => setAccountSearchText(e.target.value), [])

  const onSelectAccountFromSearch = React.useCallback((account: WalletAccountType) => () => {
    closeAccountSearch()
    setSelectedAccount(account)
  }, [closeAccountSearch])

  const onBack = React.useCallback(() => {
    if (!showBuyOptions && history.length) {
      return history.goBack()
    }

    if (showBuyOptions) {
      // go back to asset selection
      setShowBuyOptions(false)
      return closeAccountSearch()
    }
  }, [showBuyOptions, closeAccountSearch, history])

  const nextStep = React.useCallback(() => {
    if (!isNextStepEnabled) {
      return
    }
    setShowBuyOptions(true)
  }, [isNextStepEnabled])

  const onSubmitBuy = React.useCallback((buyOption: BraveWallet.OnRampProvider) => {
    if (!selectedAsset || !selectedAssetNetwork || !selectedAccount) {
      return
    }
    openBuyAssetLink({
      buyOption,
      depositAddress: selectedAccount.address
    })
  }, [selectedAsset, selectedAssetNetwork, selectedAccount, selectedCurrency])

  const goBackToSelectAssets = React.useCallback(() => {
    setShowBuyOptions(false)
    setSelectedAsset(undefined)
  }, [])

  const checkIsBuyAssetSelected = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    if (selectedAsset) {
      return (
        selectedAsset.contractAddress.toLowerCase() === asset.contractAddress.toLowerCase() &&
        selectedAsset.symbol.toLowerCase() === asset.symbol.toLowerCase() &&
        selectedAsset.chainId === asset.chainId
      )
    }
    return false
  }, [selectedAsset])

  const handleScrollIntoView = React.useCallback((asset: BraveWallet.BlockchainToken, ref: HTMLButtonElement | null) => {
    if (checkIsBuyAssetSelected(asset)) {
      scrollIntoView(ref)
    }
  }, [checkIsBuyAssetSelected, scrollIntoView])

  const renderToken: RenderTokenFunc = React.useCallback(({ item: { asset } }) =>
    <BuyAssetOptionItem
      selectedCurrency={selectedCurrency}
      isSelected={checkIsBuyAssetSelected(asset)}
      key={getAssetIdKey(asset)}
      token={asset}
      onClick={setSelectedAsset}
      ref={(ref) => handleScrollIntoView(asset, ref)}
    />,
    [
      selectedCurrency,
      checkIsBuyAssetSelected,
      buyAssetNetworks,
      handleScrollIntoView
    ]
  )

  // effects
  React.useEffect(() => {
    if (assetsForFilteredNetwork.length === 0) {
      getAllBuyOptionsAllChains()
    }
  }, [assetsForFilteredNetwork.length])

  React.useEffect(() => {
    // unselect asset if  AllNetworksOption is not selected
    if (selectedNetworkFilter.chainId !== AllNetworksOption.chainId) {
      setSelectedAsset(undefined)
    }
  }, [selectedNetworkFilter.chainId])

  // sync selected currency with default
  React.useEffect(() => {
    if (defaultCurrencies.fiat) {
      setSelectedCurrency(defaultCurrencies.fiat)
    }
  }, [defaultCurrencies.fiat])

  // sync default selected account with selected asset
  React.useEffect(() => {
    if (
      selectedAsset &&
      selectedAssetNetwork &&
      accountsForSelectedAssetNetwork.length && // asset is selected & account is available
      selectedAccount?.coin !== selectedAsset.coin // needs to change accounts to one with correct network
    ) {
      setSelectedAccount(accountsForSelectedAssetNetwork[0])
    }
  }, [
    selectedAsset,
    selectedAssetNetwork,
    accountsForSelectedAssetNetwork,
    selectedAccount
  ])

  // render
  return (
    <>
      {/* Hide nav when creating or searching accounts */}
      {!showAccountSearch && !showFiatSelection && !(
        needsAccount && showBuyOptions
      ) &&
        <StepsNavigation
          goBack={onBack}
          steps={[]}
          currentStep=''
          preventGoBack={!showBuyOptions}
        />
      }

      {/* Fiat Selection */}
      {showFiatSelection &&
        <SelectCurrency
          onSelectCurrency={
            // this internally sets the currency via redux,
            // so just hide the UI when selected
            (currency) => {
              setSelectedCurrency(currency.currencyCode)
              setShowFiatSelection(false)
            }
          }
          onBack={() => setShowFiatSelection(false)}
        />
      }

      {/* Asset Selection */}
      {!showBuyOptions && !showFiatSelection &&
        <>
          <SelectAssetWrapper>
            <SwapInputComponent
              defaultCurrencies={defaultCurrencies}
              componentType='buyAmount'
              onInputChange={setBuyAmount}
              selectedAssetInputAmount={buyAmount}
              inputName='buy'
              selectedAsset={selectedAsset}
              selectedNetwork={selectedNetwork}
              autoFocus={true}
              onShowCurrencySelection={() => setShowFiatSelection(true)}
            />

            {assetsForFilteredNetwork.length
              ? <TokenLists
                enableScroll
                maxListHeight='38vh'
                userAssetList={assetsForFilteredNetwork}
                networks={networksFilterOptions}
                hideAddButton
                hideAssetFilter
                hideAccountFilter
                hideAutoDiscovery
                estimatedItemSize={100}
                renderToken={renderToken}
              />
              : <Column>
                <LoadingIcon
                  opacity={1}
                  size='100px'
                  color='interactive05'
                />
              </Column>
            }

            <VerticalSpace space='12px' />

          </SelectAssetWrapper>

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={
                selectedAsset
                  ? getLocale('braveWalletBuyContinueButton')
                  : getLocale('braveWalletBuySelectAsset')
              }
              onSubmit={nextStep}
              disabled={!isNextStepEnabled}
            />
          </NextButtonRow>
        </>
      }

      {/* Creates wallet Account if needed */}
      {needsAccount && showBuyOptions &&
        <CreateAccountTab
          network={selectedAssetNetwork}
          prevNetwork={prevNetwork}
          onCancel={goBackToSelectAssets}
        />
      }

      {/* Buy Option selection & Account selection/search */}
      {!needsAccount && showBuyOptions &&
        <>
          {!showAccountSearch &&
            <>

              <Title>
                {
                  getLocale('braveWalletFundWalletTitle')
                    .replace('$1', selectedAsset?.symbol ?? '')
                }
              </Title>

              <Description>
                {getLocale('braveWalletFundWalletDescription')}
              </Description>

              <Row
                justifyContent='space-around'
                alignItems='center'
              >
                <Flex>
                  <SelectAccountItem
                    selectedNetwork={selectedAssetNetwork}
                    account={selectedAccount}
                    onSelectAccount={openAccountSearch}
                    showTooltips
                    fullAddress
                    showSwitchAccountsIcon
                  />
                </Flex>
              </Row>

              <SelectBuyOption
                layoutType='loose'
                buyOptions={selectedAssetBuyOptions}
                onSelect={onSubmitBuy}
                selectedOption={undefined}
              />
            </>
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

export default FundWalletScreen
