import * as React from 'react'
import {
  AccountAssetOptionType,
  OrderTypes,
  BuySendSwapViewTypes,
  SlippagePresetObjectType,
  ExpirationPresetObjectType,
  ToOrFromType
} from '../../../constants/types'
import { NavButton } from '../../extension'
import SwapInputComponent from '../swap-input-component'
// Styled Components
import {
  StyledWrapper,
  ArrowDownIcon,
  ArrowButton
} from './style'

export interface Props {
  toAsset: AccountAssetOptionType
  fromAsset: AccountAssetOptionType
  fromAmount: string
  toAmount: string
  exchangeRate: string
  slippageTolerance: SlippagePresetObjectType
  orderExpiration: ExpirationPresetObjectType
  orderType: OrderTypes
  fromAssetBalance: string
  toAssetBalance: string
  onToggleOrderType: () => void
  onFlipAssets: () => void
  onSubmitSwap: () => void
  onInputChange: (value: string, name: string) => void
  onChangeSwapView: (view: BuySendSwapViewTypes, option?: ToOrFromType) => void
  onSelectPresetAmount: (percent: number) => void
  onSelectExpiration: (expiration: ExpirationPresetObjectType) => void
  onSelectSlippageTolerance: (slippage: SlippagePresetObjectType) => void
  onFilterAssetList: (asset: AccountAssetOptionType) => void
}

function Swap (props: Props) {
  const {
    toAsset,
    fromAsset,
    fromAmount,
    toAmount,
    orderType,
    exchangeRate,
    slippageTolerance,
    orderExpiration,
    fromAssetBalance,
    toAssetBalance,
    onToggleOrderType,
    onInputChange,
    onSelectPresetAmount,
    onSelectExpiration,
    onSelectSlippageTolerance,
    onFlipAssets,
    onSubmitSwap,
    onChangeSwapView,
    onFilterAssetList
  } = props

  const onShowAssetTo = () => {
    onChangeSwapView('assets', 'to')
    onFilterAssetList(fromAsset)
  }

  const onShowAssetFrom = () => {
    onChangeSwapView('assets', 'from')
    onFilterAssetList(toAsset)
  }

  return (
    <StyledWrapper>
      <SwapInputComponent
        componentType='fromAmount'
        onSelectPresetAmount={onSelectPresetAmount}
        onInputChange={onInputChange}
        selectedAssetInputAmount={fromAmount}
        inputName='from'
        selectedAssetBalance={fromAssetBalance}
        selectedAsset={fromAsset}
        onShowSelection={onShowAssetFrom}
      />
      <ArrowButton onClick={onFlipAssets}>
        <ArrowDownIcon />
      </ArrowButton>
      <SwapInputComponent
        componentType='toAmount'
        orderType={orderType}
        onInputChange={onInputChange}
        selectedAssetInputAmount={toAmount}
        inputName='to'
        selectedAssetBalance={toAssetBalance}
        selectedAsset={toAsset}
        onShowSelection={onShowAssetTo}
      />
      <SwapInputComponent
        componentType='exchange'
        orderType={orderType}
        onToggleOrderType={onToggleOrderType}
        onInputChange={onInputChange}
        selectedAssetInputAmount={exchangeRate}
        inputName='rate'
        selectedAsset={fromAsset}
      />
      <SwapInputComponent
        componentType='selector'
        orderType={orderType}
        onSelectSlippageTolerance={onSelectSlippageTolerance}
        onSelectExpiration={onSelectExpiration}
        slippageTolerance={slippageTolerance}
        orderExpiration={orderExpiration}
      />
      <NavButton
        disabled={false}
        buttonType='primary'
        text='Swap'
        onSubmit={onSubmitSwap}
      />
    </StyledWrapper>
  )
}

export default Swap
