// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import {
  BraveWallet,
  BuySendSwapViewTypes,
  ToOrFromType,
  BuyOption
} from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'
import { WalletSelectors } from '../../../common/selectors'

// options
import { SelectBuyOption } from '../select-buy-option/select-buy-option'

// hooks
import { useUnsafeWalletSelector } from '../../../common/hooks/use-safe-selector'

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
  isSelectedNetworkSupported: boolean
  buyAmount: string
  buyOptions: BuyOption[]
  onChangeBuyAmount: (buyAmount: string) => void
  onChangeBuyView: (view: BuySendSwapViewTypes, option?: ToOrFromType) => void
  onShowCurrencySelection: () => void
  openBuyAssetLink: ({ buyOption, depositAddress }: { buyOption: number, depositAddress: string }) => void
}

export const Buy = ({
  selectedAsset,
  buyAmount,
  buyOptions,
  isSelectedNetworkSupported,
  onChangeBuyAmount,
  onChangeBuyView,
  onShowCurrencySelection,
  openBuyAssetLink
}: Props) => {
  // state
  const [showBuyOptions, setShowBuyOptions] = React.useState<boolean>(false)
  const [selectedOnRampProvider, setSelectedOnRampProvider] = React.useState<BraveWallet.OnRampProvider>()

  // Redux
  const selectedNetwork = useUnsafeWalletSelector(WalletSelectors.selectedNetwork)
  const selectedAccount = useUnsafeWalletSelector(WalletSelectors.selectedAccount)
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)

  // methods
  const onSubmitBuy = React.useCallback(async (buyOption: BraveWallet.OnRampProvider) => {
    if (!selectedAsset || !selectedNetwork || !selectedAccount) {
      return
    }
    openBuyAssetLink({
      buyOption,
      depositAddress: selectedAccount.address
    })
    setSelectedOnRampProvider(undefined)
  }, [selectedAsset, selectedNetwork, buyAmount, selectedAccount])

  const onShowAssets = React.useCallback(() => {
    onChangeBuyView('assets', 'from')
  }, [onChangeBuyView])

  const onContinue = React.useCallback(() => {
    setShowBuyOptions(true)
  }, [])

  const onBack = React.useCallback(() => {
    setShowBuyOptions(false)
  }, [])

  // render
  return (
    <StyledWrapper>
      {showBuyOptions
        ? <SelectBuyOption
          buyOptions={buyOptions}
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
                onInputChange={onChangeBuyAmount}
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
