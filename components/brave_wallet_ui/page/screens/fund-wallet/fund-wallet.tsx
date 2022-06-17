// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { useSelector } from 'react-redux'

// styles
// import {} from './fund-wallet.style'

// utils
import { getNetworkInfo } from '../../../utils/network-utils'
// import { getLocale } from '../../../../common/locale'

// routes, types, options
import { BraveWallet, SupportedOnRampNetworks, UserAssetInfoType, WalletRoutes, WalletState } from '../../../constants/types'
import { AllNetworksOption } from '../../../options/network-filter-options'

// action

// hooks
import { useAssets } from '../../../common/hooks'

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
  // routing
  const history = useHistory()

  // redux
  const {
    defaultCurrencies,
    networkList,
    transactionSpotPrices,
    selectedNetworkFilter
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // state
  const [filteredList, setFilteredList] = React.useState<UserAssetInfoType[]>([])
  const [buyAmount, setBuyAmount] = React.useState('')
  const [selectedAsset, setSelectedAsset] = React.useState<BraveWallet.BlockchainToken | undefined>()
  const [isShowingAllOptions, setIsShowingAllOptions] = React.useState(false)

  // custom hooks
  const { allBuyAssetOptions, getAllBuyOptionsAllChains } = useAssets()

  // memos
  const buyAssetNetworks = React.useMemo(() => {
    return networkList.filter(n =>
      SupportedOnRampNetworks.includes(n.chainId)
    )
  }, [networkList])

  const isNextStepEnabled = React.useMemo(() => {
    return !!selectedAsset
  }, [selectedAsset])

  const selectedAssetNetwork = React.useMemo(() => {
    return selectedAsset ? getNetworkInfo(selectedAsset.chainId, selectedAsset.coin, buyAssetNetworks) : undefined
  }, [selectedAsset, buyAssetNetworks])

  const assetsForFilteredNetwork = React.useMemo(() => {
    const assets = selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? allBuyAssetOptions
      : allBuyAssetOptions.filter(({ chainId }) => selectedNetworkFilter.chainId === chainId)

    return assets.map(asset => ({ asset, assetBalance: '1' }))
  }, [selectedNetworkFilter.chainId, allBuyAssetOptions])

  // methods
  const nextStep = React.useCallback(() => {
    if (isNextStepEnabled) {
      history.push(WalletRoutes.DepositFundsPage)
    }
  }, [isNextStepEnabled])

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

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default OnboardingDisclosures
