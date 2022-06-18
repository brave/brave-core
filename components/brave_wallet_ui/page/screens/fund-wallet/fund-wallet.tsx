// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  useDispatch,
  useSelector
} from 'react-redux'

// styles
// import {} from './fund-wallet.style'

// utils
// import { getLocale } from '../../../../common/locale'

// types
import { BraveWallet, UserAssetInfoType, WalletState } from '../../../constants/types'

// actions
import { WalletActions } from '../../../common/actions'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'
import { SelectBuyOption } from '../../../components/buy-send-swap/select-buy-option/select-buy-option'

// hooks
import { useHasAccount, useLib } from '../../../common/hooks'
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
import { CreateAccountTab } from '../../../components/buy-send-swap'

export const FundWalletScreen = () => {
  // redux
  const dispatch = useDispatch()
  const {
    defaultCurrencies,
    transactionSpotPrices,
    selectedNetworkFilter,
    selectedAccount
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // state
  const [filteredList, setFilteredList] = React.useState<UserAssetInfoType[]>([])
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
    buyAssetNetworks,
    buyAmount,
    setBuyAmount,
    openBuyAssetLink
  } = useMultiChainBuyAssets()
  const { getBuyAssetUrl } = useLib()
  const { needsAccount } = useHasAccount()

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
      // TODO: allow address input?
      depositAddress: selectedAccount.address
    })
  }, [selectedAsset, selectedAssetNetwork, getBuyAssetUrl, buyAmount, selectedAccount])

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
          {needsAccount && <CreateAccountTab /> }

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

          {!needsAccount && showBuyOptions &&
            <>
              <p>
                Address: {selectedAccount.address}
              </p>
              <SelectBuyOption
                buyOptions={selectedAssetBuyOptions}
                onSelect={onSubmitBuy}
                onBack={onBack}
              />
            </>
          }

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default OnboardingDisclosures
