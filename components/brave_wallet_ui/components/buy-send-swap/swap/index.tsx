import * as React from 'react'

import { getLocale } from '../../../../common/locale'
import {
  AccountAssetOptionType,
  OrderTypes,
  BuySendSwapViewTypes,
  SlippagePresetObjectType,
  ExpirationPresetObjectType,
  ToOrFromType,
  SwapValidationErrorType
} from '../../../constants/types'
import SwapInputComponent from '../swap-input-component'

// Styled Components
import {
  StyledWrapper,
  ArrowDownIcon,
  ArrowButton,
  SwapNavButton,
  SwapButtonText,
  SwapButtonLoader
} from './style'
import { LoaderIcon } from 'brave-ui/components/icons'

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
  isFetchingQuote: boolean
  isSubmitDisabled: boolean
  validationError?: SwapValidationErrorType
  customSlippageTolerance: string
  onCustomSlippageToleranceChange: (value: string) => void
  onToggleOrderType: () => void
  onFlipAssets: () => void
  onSubmitSwap: () => void
  onInputChange: (value: string, name: string) => void
  onChangeSwapView: (view: BuySendSwapViewTypes, option?: ToOrFromType) => void
  onSelectPresetAmount: (percent: number) => void
  onSelectExpiration: (expiration: ExpirationPresetObjectType) => void
  onSelectSlippageTolerance: (slippage: SlippagePresetObjectType) => void
  onFilterAssetList: (asset: AccountAssetOptionType) => void
  onQuoteRefresh: () => void
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
    isFetchingQuote,
    isSubmitDisabled,
    validationError,
    customSlippageTolerance,
    onCustomSlippageToleranceChange,
    onToggleOrderType,
    onInputChange,
    onSelectPresetAmount,
    onSelectExpiration,
    onSelectSlippageTolerance,
    onFlipAssets,
    onSubmitSwap,
    onChangeSwapView,
    onFilterAssetList,
    onQuoteRefresh
  } = props

  const onShowAssetTo = () => {
    onChangeSwapView('assets', 'to')
    onFilterAssetList(fromAsset)
  }

  const onShowAssetFrom = () => {
    onChangeSwapView('assets', 'from')
    onFilterAssetList(toAsset)
  }

  const submitText = React.useMemo(() => {
    if (validationError === 'insufficientBalance') {
      return getLocale('braveWalletSwapInsufficientBalance')
    }

    if (validationError === 'insufficientEthBalance') {
      return getLocale('braveWalletSwapInsufficientEthBalance')
    }

    if (validationError === 'insufficientAllowance') {
      return getLocale('braveWalletSwapInsufficientAllowance')
        .replace('$1', fromAsset.asset.symbol)
    }

    if (validationError === 'insufficientLiquidity') {
      return getLocale('braveWalletSwapInsufficientLiquidity')
    }

    if (validationError === 'unknownError') {
      return getLocale('braveWalletSwapUnknownError')
    }

    return getLocale('braveWalletSwap')
  }, [validationError])

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
        validationError={validationError}
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
        onRefresh={onQuoteRefresh}
      />
      <SwapInputComponent
        componentType='selector'
        orderType={orderType}
        onSelectSlippageTolerance={onSelectSlippageTolerance}
        onSelectExpiration={onSelectExpiration}
        slippageTolerance={slippageTolerance}
        orderExpiration={orderExpiration}
        customSlippageTolerance={customSlippageTolerance}
        onCustomSlippageToleranceChange={onCustomSlippageToleranceChange}
      />
      <SwapNavButton
        disabled={isSubmitDisabled}
        buttonType='primary'
        onClick={onSubmitSwap}
      >
        {
          isFetchingQuote
          ? <SwapButtonLoader><LoaderIcon /></SwapButtonLoader>
          : <SwapButtonText>{submitText}</SwapButtonText>
        }
      </SwapNavButton>
    </StyledWrapper>
  )
}

export default Swap
