// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { CurrencySymbols } from '../../../utils/currency-symbols'
import Amount from '../../../utils/amount'

// Components
import {
  withPlaceholderIcon //
} from '../../shared/create-placeholder-icon/index'

// Styled Components
import {
  AssetIcon,
  AssetButton,
  AssetTicker,
  CaratDownIcon,
  Input,
  Row,
  ButtonLeftSide,
  Spacer
} from './buy_amount_input.style'

import { BubbleContainer } from '../shared-styles'

export interface Props {
  autoFocus?: boolean
  selectedAsset?: BraveWallet.BlockchainToken | undefined
  selectedNetwork?: BraveWallet.NetworkInfo
  buyAmount: string
  onAmountChange: (value: string) => void
  onShowCurrencySelection: () => void
  selectedFiatCurrencyCode: string
}

const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, {
  size: 'small',
  marginLeft: 4,
  marginRight: 8
})

/**
 * Ramp assets have the format: ChainNativeTokenSymbol_AssetSymbol e.g
 * MATIC_BSC
 * @returns just the AssetSymbol
 */
const getAssetSymbol = (symbol?: string) => {
  return symbol && symbol.includes('_') ? symbol.split('_')[1] : symbol
}

export function BuyAmountInput({
  autoFocus,
  selectedAsset,
  selectedNetwork,
  buyAmount,
  onAmountChange,
  onShowCurrencySelection,
  selectedFiatCurrencyCode
}: Props) {
  // methods
  const onInputChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onAmountChange(event.target.value)
    },
    [onAmountChange]
  )

  // render
  return (
    <BubbleContainer isV2>
      <Row>
        <AssetButton onClick={onShowCurrencySelection}>
          <AssetTicker role='currency'>
            {CurrencySymbols[selectedFiatCurrencyCode]}
          </AssetTicker>
          <CaratDownIcon />
          <Spacer />
        </AssetButton>
        {!(selectedAsset?.isErc721 || selectedAsset?.isNft) && (
          <Input
            type={'number'}
            placeholder={'0'}
            value={buyAmount}
            name={'buy'}
            onChange={onInputChanged}
            spellCheck={false}
            autoFocus={autoFocus}
          />
        )}
        <AssetButton isERC721={selectedAsset?.isErc721 || selectedAsset?.isNft}>
          <ButtonLeftSide>
            <AssetIconWithPlaceholder
              asset={selectedAsset}
              network={selectedNetwork}
            />
            <AssetTicker role='symbol'>
              {getAssetSymbol(selectedAsset?.symbol)}{' '}
              {selectedAsset?.isErc721 && selectedAsset?.tokenId
                ? '#' + new Amount(selectedAsset.tokenId).toNumber()
                : ''}
            </AssetTicker>
          </ButtonLeftSide>
        </AssetButton>
      </Row>
    </BubbleContainer>
  )
}

export default BuyAmountInput
