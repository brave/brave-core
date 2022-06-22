// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import {
  useDispatch,
  useSelector
} from 'react-redux'

// utils
import { getLocale } from '../../../../common/locale'

// types
import {
  BraveWallet,
  UserAssetInfoType,
  WalletAccountType,
  WalletRoutes,
  WalletState
} from '../../../constants/types'

// actions
import { WalletActions } from '../../../common/actions'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'
import { SelectBuyOption } from '../../../components/buy-send-swap/select-buy-option/select-buy-option'

// hooks
import { useHasAccount } from '../../../common/hooks'
import { useMultiChainBuyAssets } from '../../../common/hooks/use-multi-chain-buy-assets'

// style
import { ScrollContainer, SearchWrapper } from './fund-wallet.style'
import { Column, Flex, LinkText, Row } from '../../../components/shared/style'
import { Description, MainWrapper, NextButtonRow, StyledWrapper, Title } from '../onboarding/onboarding.style'

// components
import SelectNetworkButton from '../../../components/shared/select-network-button'
import SearchBar from '../../../components/shared/search-bar'
import SelectAccountItem from '../../../components/shared/select-account-item'
import SelectAccount from '../../../components/shared/select-account'
import WalletPageLayout from '../../../components/desktop/wallet-page-layout/index'
import { NavButton } from '../../../components/extension/buttons/nav-button/index'
import TokenLists from '../../../components/desktop/views/portfolio/components/token-lists'
import CreateAccountTab from '../../../components/buy-send-swap/create-account'
import SwapInputComponent from '../../../components/buy-send-swap/swap-input-component'
import SelectHeader from '../../../components/buy-send-swap/select-header'
import { StepsNavigation } from '../../../components/desktop/steps-navigation/steps-navigation'

export const FundWalletScreen = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const {
    accounts,
    defaultCurrencies,
    transactionSpotPrices,
    selectedNetworkFilter,
    selectedAccount
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // state
  const [filteredList, setFilteredList] = React.useState<UserAssetInfoType[]>([])
  const [isShowingAllOptions, setIsShowingAllOptions] = React.useState(false)
  const [showBuyOptions, setShowBuyOptions] = React.useState<boolean>(false)
  const [showAccountSearch, setShowAccountSearch] = React.useState<boolean>(false)
  const [accountSearchText, setAccountSearchText] = React.useState<string>('')

  // custom hooks
  const { needsAccount } = useHasAccount()
  const {
    allAssetOptions: allBuyAssetOptions,
    getAllBuyOptionsAllChains,
    selectedAsset,
    selectedAssetNetwork,
    setSelectedAsset,
    selectedAssetBuyOptions,
    buyAssetNetworks,
    buyAmount,
    setBuyAmount,
    openBuyAssetLink
  } = useMultiChainBuyAssets()

  // memos
  const isNextStepEnabled = React.useMemo(() => {
    return !!selectedAsset
  }, [selectedAsset, buyAmount])

  const assetsForFilteredNetwork = React.useMemo(() => {
    const assets = selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? allBuyAssetOptions
      : allBuyAssetOptions.filter(({ chainId }) => selectedNetworkFilter.chainId === chainId)

    return assets.map(asset => ({ asset, assetBalance: '1' }))
  }, [selectedNetworkFilter.chainId, allBuyAssetOptions])

  const accountsForSelectedAssetNetwork = React.useMemo(() => {
    return selectedAssetNetwork
      ? accounts.filter(a => a.coin === selectedAssetNetwork.coin)
      : []
  }, [selectedAssetNetwork, accounts])

  const accountListSearchResults = React.useMemo(() => {
    if (accountSearchText === '') {
      return accountsForSelectedAssetNetwork
    }

    return accountsForSelectedAssetNetwork.filter((item) => {
      return item.name.toLowerCase().startsWith(accountSearchText.toLowerCase())
    })
  }, [accountSearchText, accountsForSelectedAssetNetwork])

  const networksFilterOptions = React.useMemo(() => {
    return [AllNetworksOption, ...buyAssetNetworks]
  }, [buyAssetNetworks])

  // methods
  const openAccountSearch = React.useCallback(() => setShowAccountSearch(true), [])
  const closeAccountSearch = React.useCallback(() => setShowAccountSearch(false), [])
  const showAllBuyOptions = React.useCallback(() => setIsShowingAllOptions(true), [])
  const onSearchTextChanged = React.useCallback((e: React.ChangeEvent<HTMLInputElement>) => setAccountSearchText(e.target.value), [])

  const onSelectAssetFromTokenList = React.useCallback((asset: BraveWallet.BlockchainToken | undefined) => {
    return () => setSelectedAsset(asset)
  }, [])

  const goToPortfolio = React.useCallback(() => {
    history.push(WalletRoutes.Portfolio)
  }, [history])

  const onSelectAccountFromSearch = React.useCallback((account: WalletAccountType) => () => {
    closeAccountSearch()
    dispatch(WalletActions.selectAccount(account))
  }, [closeAccountSearch])

  const onBack = React.useCallback(() => {
    setShowBuyOptions(false)
    closeAccountSearch()
  }, [closeAccountSearch])

  const nextStep = React.useCallback(() => {
    if (!isNextStepEnabled || !selectedAssetNetwork) {
      return
    }
    dispatch(WalletActions.selectNetwork(selectedAssetNetwork))
    setShowBuyOptions(true)
  }, [isNextStepEnabled])

  const onSubmitBuy = React.useCallback((buyOption: BraveWallet.OnRampProvider) => {
    if (!selectedAsset || !selectedAssetNetwork) {
      return
    }
    openBuyAssetLink({
      buyOption,
      depositAddress: selectedAccount.address
    })
  }, [selectedAsset, selectedAssetNetwork, buyAmount, selectedAccount])

  // effects
  React.useEffect(() => {
    if (assetsForFilteredNetwork.length === 0) {
      getAllBuyOptionsAllChains()
    }
  }, [assetsForFilteredNetwork.length])

  React.useEffect(() => {
    // default to showing the first 5 assets
    if (!isShowingAllOptions) {
      setFilteredList(assetsForFilteredNetwork.slice(0, 5))
    }
    if (isShowingAllOptions) {
      setFilteredList(assetsForFilteredNetwork)
    }
  }, [isShowingAllOptions, assetsForFilteredNetwork])

  React.useEffect(() => {
    // filter to show only top results on chain switch
    // also unselect asset on chain switch
    if (selectedNetworkFilter) {
      setIsShowingAllOptions(false)
      setSelectedAsset(undefined)
    }
  }, [selectedNetworkFilter])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          {!showAccountSearch &&
            <StepsNavigation
              goBack={onBack}
              onSkip={goToPortfolio}
              skipButtonText={getLocale('braveWalletButtonClose')}
              steps={[]}
              currentStep=''
            />
          }

          {/* Creates wallet Account if needed */}
          {needsAccount && <CreateAccountTab /> }

          {/* Asset Selection */}
          {!needsAccount && !showBuyOptions &&
            <>
              <div>
                <SwapInputComponent
                  defaultCurrencies={defaultCurrencies}
                  componentType='buyAmount'
                  onInputChange={setBuyAmount}
                  selectedAssetInputAmount={buyAmount}
                  inputName='buy'
                  selectedAsset={selectedAsset}
                  selectedNetwork={selectedAssetNetwork || selectedNetworkFilter}
                  autoFocus={true}
                />

                <TokenLists
                  defaultCurrencies={defaultCurrencies}
                  userAssetList={assetsForFilteredNetwork}
                  filteredAssetList={filteredList}
                  tokenPrices={transactionSpotPrices}
                  networks={networksFilterOptions}
                  onSetFilteredAssetList={setFilteredList}
                  onSelectAsset={onSelectAssetFromTokenList}
                  hideBalances={true}
                />

                {assetsForFilteredNetwork.length > 5 && !isShowingAllOptions &&
                  <LinkText onClick={showAllBuyOptions}>
                    {getLocale('braveWalletButtonMore')}
                  </LinkText>
                }

              </div>

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

                  <Row justifyContent={'space-around'}>
                    <Flex>
                      <SelectAccountItem
                        account={selectedAccount}
                        onSelectAccount={openAccountSearch}
                        showTooltips
                        fullAddress
                      />
                    </Flex>

                    {selectedAssetNetwork &&
                      <Column>
                        <SelectNetworkButton selectedNetwork={selectedAssetNetwork} />
                      </Column>
                    }
                  </Row>

                  <SelectBuyOption
                    layoutType='loose'
                    buyOptions={selectedAssetBuyOptions}
                    onSelect={onSubmitBuy}
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

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default FundWalletScreen
