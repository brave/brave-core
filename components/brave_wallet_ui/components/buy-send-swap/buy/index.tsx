// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// types
import {
  BraveWallet,
  BuyOption,
  BuySendSwapViewTypes,
  ToOrFromType,
  SupportedTestNetworks,
  WalletState
} from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'
import { getRampAssetSymbol, isSelectedAssetInAssetOptions } from '../../../utils/asset-utils'

// options
import { BuyOptions } from '../../../options/buy-with-options'
import { SelectBuyOption } from '../select-buy-option/select-buy-option'

// hooks
import { useAssets } from '../../../common/hooks/assets'
import { useLib } from '../../../common/hooks/useLib'

// components
import { NavButton } from '../../extension'
import SwapInputComponent from '../swap-input-component'

// styles
import {
  StyledWrapper,
  Spacer,
  NetworkNotSupported
} from './style'

export interface Props {
  selectedAsset: BraveWallet.BlockchainToken
  onChangeBuyView: (view: BuySendSwapViewTypes, option?: ToOrFromType) => void
}

export const Buy = ({
  selectedAsset,
  onChangeBuyView
}: Props) => {
  // state
  const [buyAmount, setBuyAmount] = React.useState('')
  const [showBuyOptions, setShowBuyOptions] = React.useState<boolean>(false)
  const [selectedOnRampProvider, setSelectedOnRampProvider] = React.useState<BraveWallet.OnRampProvider>()

  // Redux
  const {
    selectedNetwork,
    selectedAccount,
    defaultCurrencies,
    selectedCurrency
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // Custom Hooks
  const { wyreAssetOptions, rampAssetOptions, sardineAssetOptions } = useAssets()
  const { getBuyAssetUrl } = useLib()

  // memos
  const supportedBuyOptions: BuyOption[] = React.useMemo(() => {
    return BuyOptions.filter(buyOption => {
      switch (buyOption.id) {
        case BraveWallet.OnRampProvider.kWyre: return isSelectedAssetInAssetOptions(selectedAsset, wyreAssetOptions)
        case BraveWallet.OnRampProvider.kRamp: return isSelectedAssetInAssetOptions(selectedAsset, rampAssetOptions)
        case BraveWallet.OnRampProvider.kSardine: return isSelectedAssetInAssetOptions(selectedAsset, sardineAssetOptions)
        default: return false
      }
    })
  }, [selectedAsset, wyreAssetOptions, rampAssetOptions])

  const isSelectedNetworkSupported = React.useMemo(() => {
    // Test networks are not supported in buy tab
    if (SupportedTestNetworks.includes(selectedNetwork.chainId.toLowerCase())) {
      return false
    }

    return [...rampAssetOptions, ...wyreAssetOptions, ...sardineAssetOptions]
      .map(asset => asset.chainId.toLowerCase())
      .includes(selectedNetwork.chainId.toLowerCase())
  }, [selectedNetwork, rampAssetOptions, wyreAssetOptions])

  // methods
  const onSubmitBuy = React.useCallback(async (buyOption: BraveWallet.OnRampProvider) => {
    const asset = buyOption === BraveWallet.OnRampProvider.kRamp
      ? { ...selectedAsset, symbol: getRampAssetSymbol(selectedAsset) }
      : selectedAsset
    setSelectedOnRampProvider(buyOption)

    try {
      const url = await getBuyAssetUrl({
        asset,
        onRampProvider: buyOption,
        chainId: selectedNetwork.chainId,
        address: selectedAccount.address,
        amount: buyAmount,
        currencyCode: selectedCurrency ? selectedCurrency.currencyCode : 'USD'
      })

      chrome.tabs.create({ url }, () => {
        if (chrome.runtime.lastError) {
          console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
        }
      })
    } catch (e) {
      console.error(e)
    }
    setSelectedOnRampProvider(undefined)
  }, [
    getBuyAssetUrl,
    selectedNetwork,
    selectedAccount,
    buyAmount,
    selectedAsset,
    selectedCurrency
  ])
  const onShowAssets = React.useCallback(() => {
    onChangeBuyView('assets', 'from')
  }, [onChangeBuyView])

  const onContinue = React.useCallback(() => {
    setShowBuyOptions(true)
  }, [])

  const onBack = React.useCallback(() => {
    setShowBuyOptions(false)
  }, [])

  const onShowCurrencySelection = React.useCallback(() => {
    onChangeBuyView('currencies', 'from')
  }, [onChangeBuyView])

  // render
  return (
    <StyledWrapper>
      {showBuyOptions
        ? <SelectBuyOption
          buyOptions={supportedBuyOptions}
          selectedOption={selectedOnRampProvider}
          onSelect={onSubmitBuy}
          onBack={onBack}
        />
        : <>
          {isSelectedNetworkSupported
            ? <>
              <SwapInputComponent
                defaultCurrencies={defaultCurrencies}
                componentType='buyAmount'
                onInputChange={setBuyAmount}
                selectedAssetInputAmount={buyAmount}
                inputName='buy'
                selectedAsset={selectedAsset}
                selectedNetwork={selectedNetwork}
                onShowSelection={onShowAssets}
                onShowCurrencySelection={onShowCurrencySelection}
                autoFocus={true}
              />
              <Spacer />
              <NavButton
                disabled={false}
                buttonType='primary'
                text={getLocale('braveWalletBuyContinueButton')}
                onSubmit={onContinue}
              />
            </>
            : <NetworkNotSupported>{getLocale('braveWalletBuyTapBuyNotSupportedMessage')}</NetworkNotSupported>
          }
        </>
      }
    </StyledWrapper>
  )
}

export default Buy
