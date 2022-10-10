// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Utils
import {
  UserAccountType,
  BuySendSwapViewTypes,
  BraveWallet,
  DefaultCurrencies
} from '../../../constants/types'

// Hooks

// Components
import {
  AccountsAssetsNetworks,
  Header,
  Buy
} from '..'
import { useMultiChainBuyAssets } from '../../../common/hooks/use-multi-chain-buy-assets'

export interface Props {
  showHeader?: boolean
  defaultCurrencies: DefaultCurrencies
  onSelectAccount: (account: UserAccountType) => void
}

function BuyTab (props: Props) {
  const {
    showHeader,
    onSelectAccount
  } = props

  // Custom Hooks
  const {
    allAssetOptions: buyAssetOptions,
    selectedAsset,
    selectedAssetBuyOptions,
    setSelectedAsset,
    getAllBuyOptionsAllChains,
    buyAmount,
    setBuyAmount,
    isSelectedNetworkSupported,
    assetsForFilteredNetwork,
    openBuyAssetLink
  } = useMultiChainBuyAssets()
  const [buyView, setBuyView] = React.useState<BuySendSwapViewTypes>('buy')

  const onChangeBuyView = React.useCallback((view: BuySendSwapViewTypes) => {
    setBuyView(view)
  }, [])

  const onClickSelectAccount = React.useCallback((account: UserAccountType) => () => {
    onSelectAccount(account)
    setBuyView('buy')
  }, [onSelectAccount])

  const onSelectedAsset = React.useCallback((asset: BraveWallet.BlockchainToken) => () => {
    setSelectedAsset(asset)
    setBuyView('buy')
  }, [])

  const onSelectCurrency = React.useCallback(() => {
    // hide currency selection view
    setBuyView('buy')
  }, [setBuyView])

  const goBack = React.useCallback(() => {
    setBuyView('buy')
  }, [])

  const onShowCurrencySelection = React.useCallback(() => {
    onChangeBuyView('currencies')
  }, [onChangeBuyView])

  React.useEffect(() => {
    if (buyAssetOptions.length === 0) {
      getAllBuyOptionsAllChains()
    }
  }, [buyAssetOptions.length])

  React.useEffect(() => {
    if (assetsForFilteredNetwork.length > 0) {
      setSelectedAsset(assetsForFilteredNetwork[0])
    }
  }, [assetsForFilteredNetwork])

  return (
    <>
      {buyView === 'buy' &&
        <>
          {showHeader &&
            <Header
              onChangeSwapView={onChangeBuyView}
            />
          }
          <Buy
            isSelectedNetworkSupported={isSelectedNetworkSupported}
            buyAmount={buyAmount}
            buyOptions={selectedAssetBuyOptions}
            onChangeBuyAmount={setBuyAmount}
            selectedAsset={selectedAsset || assetsForFilteredNetwork[0]}
            onChangeBuyView={onChangeBuyView}
            onShowCurrencySelection={onShowCurrencySelection}
           openBuyAssetLink={openBuyAssetLink}
          />
        </>
      }
      {buyView !== 'buy' &&
        <AccountsAssetsNetworks
          goBack={goBack}
          assetOptions={assetsForFilteredNetwork}
          onClickSelectAccount={onClickSelectAccount}
          onSelectedAsset={onSelectedAsset}
          onSelectCurrency={onSelectCurrency}
          selectedView={buyView}
        />
      }
    </>
  )
}

export default BuyTab
