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
import { getTokensNetwork } from '../../../utils/network-utils'

// types
import {
  BraveWallet,
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
import { Flex, LinkText, Row } from '../../../components/shared/style'
import { Description, MainWrapper, NextButtonRow, StyledWrapper, Title } from '../onboarding/onboarding.style'
import {
  ScrollContainer,
  SearchWrapper,
  SelectAssetWrapper
} from './fund-wallet.style'

// components
// import SelectNetworkButton from '../../../components/shared/select-network-button'
import SearchBar from '../../../components/shared/search-bar'
import SelectAccountItem from '../../../components/shared/select-account-item'
import SelectAccount from '../../../components/shared/select-account'
import WalletPageLayout from '../../../components/desktop/wallet-page-layout/index'
import { NavButton } from '../../../components/extension/buttons/nav-button/index'
import { TokenLists } from '../../../components/desktop/views/portfolio/components/token-lists/token-list'
import CreateAccountTab from '../../../components/buy-send-swap/create-account'
import SwapInputComponent from '../../../components/buy-send-swap/swap-input-component'
import SelectHeader from '../../../components/buy-send-swap/select-header'
import { StepsNavigation } from '../../../components/desktop/steps-navigation/steps-navigation'
import { BuyAssetOptionItem } from '../../../components/buy-option/buy-asset-option'

export const FundWalletScreen = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const {
    accounts,
    defaultCurrencies,
    selectedNetworkFilter,
    selectedAccount
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // state
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

  // default to showing the first 5 assets
  const filteredAssetsListForFilteredNetwork = React.useMemo(() => isShowingAllOptions
      ? assetsForFilteredNetwork
      : assetsForFilteredNetwork.slice(0, 5),
    [isShowingAllOptions, assetsForFilteredNetwork]
  )

  // methods
  const openAccountSearch = React.useCallback(() => setShowAccountSearch(true), [])
  const closeAccountSearch = React.useCallback(() => setShowAccountSearch(false), [])
  const showAllBuyOptions = React.useCallback(() => setIsShowingAllOptions(true), [])
  const onSearchTextChanged = React.useCallback((e: React.ChangeEvent<HTMLInputElement>) => setAccountSearchText(e.target.value), [])

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
    if (!selectedAsset || !selectedAssetNetwork || !selectedAccount) {
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
    // filter to show only top results on chain switch
    // also unselect asset on chain switch
    if (selectedNetworkFilter) {
      setIsShowingAllOptions(false)
      setSelectedAsset(undefined)
    }
  }, [selectedNetworkFilter])

  // sync default selected account with selected asset
  React.useEffect(() => {
    if (
      selectedAsset &&
      selectedAssetNetwork &&
      accountsForSelectedAssetNetwork.length && // asset is selected & account is available
      selectedAccount.coin !== selectedAsset.coin // needs to change accounts to one with correct network
    ) {
      dispatch(WalletActions.selectAccount(accountsForSelectedAssetNetwork[0]))
    }
  }, [selectedAsset, selectedAssetNetwork, accountsForSelectedAssetNetwork, selectedAccount])

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
              <SelectAssetWrapper>
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
                  userAssetList={filteredAssetsListForFilteredNetwork}
                  networks={networksFilterOptions}
                  hideAddButton
                  renderToken={({ asset }) => {
                    return <BuyAssetOptionItem
                      isSelected={asset === selectedAsset}
                      key={asset.isErc721
                        ? `${asset.contractAddress}-${asset.symbol}-${asset.chainId}`
                        : `${asset.contractAddress}-${asset.tokenId}-${asset.chainId}`}
                      token={asset}
                      tokenNetwork={getTokensNetwork(networksFilterOptions, asset)}
                      onClick={setSelectedAsset}
                    />
                  }}
                />

                {assetsForFilteredNetwork.length > 5 && !isShowingAllOptions &&
                  <LinkText onClick={showAllBuyOptions}>
                    {getLocale('braveWalletButtonMore')}
                  </LinkText>
                }

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
                      />
                    </Flex>
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
