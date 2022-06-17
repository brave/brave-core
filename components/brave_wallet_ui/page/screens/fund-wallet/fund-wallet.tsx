// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// styles
// import {} from './fund-wallet.style'

// utils
import { getRampAssetSymbol } from '../../../utils/asset-utils'
// import { getLocale } from '../../../../common/locale'

// routes, types
import { BraveWallet, UserAssetInfoType, WalletState } from '../../../constants/types'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'
import { SelectBuyOption } from '../../../components/buy-send-swap/select-buy-option/select-buy-option'

// hooks
import { useLib } from '../../../common/hooks'
import { useMultiChainBuyAssets } from '../../../common/hooks/use-multi-chain-buy-assets'

// style
import { LinkText } from '../../../components/shared/style'
import { MainWrapper, NextButtonRow, StyledWrapper } from '../onboarding/onboarding.style'

// components
import WalletPageLayout from '../../../components/desktop/wallet-page-layout/index'
import { NavButton } from '../../../components/extension/buttons/nav-button/index'
import OnboardingDisclosures from '../onboarding/disclosures/disclosures'
import SwapInputComponent from '../../../components/buy-send-swap/swap-input-component'
import TokenLists from '../../../components/desktop/views/portfolio/components/token-lists'

export const FundWalletScreen = () => {
  // redux
  const {
    defaultCurrencies,
    transactionSpotPrices,
    selectedNetworkFilter
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // state
  const [filteredList, setFilteredList] = React.useState<UserAssetInfoType[]>([])
  const [buyAmount, setBuyAmount] = React.useState('')
  const [isShowingAllOptions, setIsShowingAllOptions] = React.useState(false)
  const [showBuyOptions, setShowBuyOptions] = React.useState<boolean>(false)

  // custom hooks
  const {
    allAssetOptions: allBuyAssetOptions,
    getAllBuyOptionsAllChains,
    selectedAsset,
    selectedAssetNetwork,
    setSelectedAsset,
    selectedAssetBuyOptions,
    buyAssetNetworks
  } = useMultiChainBuyAssets()
  const { getBuyAssetUrl } = useLib()

  // memos
  const isNextStepEnabled = React.useMemo(() => {
    return !!selectedAsset && !!buyAmount
  }, [selectedAsset, buyAmount])

  const assetsForFilteredNetwork = React.useMemo(() => {
    const assets = selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? allBuyAssetOptions
      : allBuyAssetOptions.filter(({ chainId }) => selectedNetworkFilter.chainId === chainId)

    return assets.map(asset => ({ asset, assetBalance: '1' }))
  }, [selectedNetworkFilter.chainId, allBuyAssetOptions])

  // methods
  const nextStep = React.useCallback(() => {
    if (isNextStepEnabled) {
      setShowBuyOptions(true)
    }
  }, [isNextStepEnabled])

  const onSubmitBuy = React.useCallback((buyOption: BraveWallet.OnRampProvider) => {
    if (!selectedAsset || !selectedAssetNetwork) {
      return
    }

    const asset = buyOption === BraveWallet.OnRampProvider.kRamp
      ? { ...selectedAsset, symbol: getRampAssetSymbol(selectedAsset) }
      : selectedAsset

    getBuyAssetUrl({
      asset,
      onRampProvider: buyOption,
      chainId: selectedAssetNetwork.chainId,
      address: '', // TODO: selectedAccount.address,
      amount: buyAmount
    })
      .then(url => {
        chrome.tabs.create({ url }, () => {
          if (chrome.runtime.lastError) {
            console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
          }
        })
      })
      .catch(e => console.error(e))
  }, [selectedAsset, selectedAssetNetwork, getBuyAssetUrl, buyAmount])

  const onBack = React.useCallback(() => {
    setShowBuyOptions(false)
  }, [])

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

          {/* TODO: Nav Here */}

          {/* Content */}
          {!showBuyOptions &&
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
                  // onShowSelection={onShowAssets}
                  autoFocus={true}
                />

                <TokenLists
                  defaultCurrencies={defaultCurrencies}
                  userAssetList={assetsForFilteredNetwork}
                  filteredAssetList={filteredList}
                  tokenPrices={transactionSpotPrices}
                  networks={[
                    AllNetworksOption,
                    ...buyAssetNetworks
                  ]}
                  onSetFilteredAssetList={setFilteredList}
                  onSelectAsset={(asset) => () => setSelectedAsset(asset)}
                  hideBalances={true}
                />

                {assetsForFilteredNetwork.length > 5 && !isShowingAllOptions &&
                  <LinkText onClick={() => setIsShowingAllOptions(true)}>
                    More
                    {/* // getLocale TODO */}
                  </LinkText>
                }

              </div>

              <NextButtonRow>
                <NavButton
                  buttonType='primary'
                  text={
                    selectedAsset
                      ? 'Choose purchase method...'
                      : 'Choose an asset'
                    // getLocale('braveWalletChoosePurchaseMethod')
                  }
                  onSubmit={nextStep}
                  disabled={!isNextStepEnabled}
                />
              </NextButtonRow>
            </>
          }

          {showBuyOptions &&
            <SelectBuyOption
              buyOptions={selectedAssetBuyOptions}
              onSelect={onSubmitBuy}
              onBack={onBack}
            />
          }

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default OnboardingDisclosures
