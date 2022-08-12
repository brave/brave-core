import * as React from 'react'
import { useSelector } from 'react-redux'

import { getLocale, splitStringForTag } from '../../../../common/locale'
import {
  AmountPresetTypes,
  BraveWallet,
  BuySendSwapViewTypes,
  ToOrFromType,
  WalletState
} from '../../../constants/types'
import SwapInputComponent from '../swap-input-component'
import { SwapTooltip } from '../../desktop'

// Styled Components
import {
  AlertIcon,
  ArrowButton,
  ArrowDownIcon,
  StyledWrapper,
  SwapButtonLoader,
  SwapButtonText,
  SwapDisclaimerButton,
  SwapDisclaimerRow,
  SwapDisclaimerText,
  SwapFeesNoticeRow,
  SwapFeesNoticeText,
  SwapNavButton
} from './style'
import { LoaderIcon } from 'brave-ui/components/icons'
import { ResetButton } from '../shared-styles'
import { useSwap } from '../../../common/hooks'
import { SwapProvider } from '../../../common/hooks/swap'

export interface Props {
  isFetchingSwapQuote: boolean
  onChangeSwapView: (view: BuySendSwapViewTypes, option?: ToOrFromType) => void
  onFilterAssetList: (asset?: BraveWallet.BlockchainToken) => void
  fromAsset?: BraveWallet.BlockchainToken
  toAsset?: BraveWallet.BlockchainToken
  customSlippageTolerance: ReturnType<typeof useSwap>['customSlippageTolerance']
  exchangeRate: ReturnType<typeof useSwap>['exchangeRate']
  flipSwapAssets: ReturnType<typeof useSwap>['flipSwapAssets']
  fromAmount: ReturnType<typeof useSwap>['fromAmount']
  fromAssetBalance: ReturnType<typeof useSwap>['fromAssetBalance']
  isSwapButtonDisabled: ReturnType<typeof useSwap>['isSwapButtonDisabled']
  onCustomSlippageToleranceChange: ReturnType<typeof useSwap>['onCustomSlippageToleranceChange']
  setOrderExpiration: ReturnType<typeof useSwap>['setOrderExpiration']
  onSelectPresetAmount: ReturnType<typeof useSwap>['onSelectPresetAmount']
  onSelectSlippageTolerance: ReturnType<typeof useSwap>['onSelectSlippageTolerance']
  onSubmitSwap: ReturnType<typeof useSwap>['onSubmitSwap']
  onSwapInputChange: ReturnType<typeof useSwap>['onSwapInputChange']
  onSwapQuoteRefresh: ReturnType<typeof useSwap>['onSwapQuoteRefresh']
  onToggleOrderType: ReturnType<typeof useSwap>['onToggleOrderType']
  orderExpiration: ReturnType<typeof useSwap>['orderExpiration']
  orderType: ReturnType<typeof useSwap>['orderType']
  selectedPreset: ReturnType<typeof useSwap>['selectedPreset']
  setSelectedPreset: ReturnType<typeof useSwap>['setSelectedPreset']
  slippageTolerance: ReturnType<typeof useSwap>['slippageTolerance']
  swapValidationError: ReturnType<typeof useSwap>['swapValidationError']
  toAmount: ReturnType<typeof useSwap>['toAmount']
  toAssetBalance: ReturnType<typeof useSwap>['toAssetBalance']
  swapProvider: SwapProvider
}

function Swap (props: Props) {
  const {
    customSlippageTolerance,
    exchangeRate,
    flipSwapAssets,
    fromAmount,
    fromAsset,
    fromAssetBalance,
    isFetchingSwapQuote,
    isSwapButtonDisabled,
    onChangeSwapView,
    onCustomSlippageToleranceChange,
    onFilterAssetList,
    setOrderExpiration,
    onSelectPresetAmount,
    onSelectSlippageTolerance,
    onSubmitSwap,
    onSwapInputChange: onInputChange,
    onSwapQuoteRefresh,
    onToggleOrderType,
    orderExpiration,
    orderType,
    selectedPreset,
    setSelectedPreset,
    slippageTolerance,
    swapValidationError: validationError,
    toAmount,
    toAsset,
    toAssetBalance,
    swapProvider
  } = props

  // redux
  const {
    selectedNetwork
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

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

    if (validationError === 'insufficientFundsForGas') {
      return getLocale('braveWalletSwapInsufficientFundsForGas')
    }

    if (validationError === 'insufficientAllowance' && fromAsset) {
      return getLocale('braveWalletSwapInsufficientAllowance')
        .replace('$1', fromAsset.symbol)
    }

    if (validationError === 'insufficientLiquidity') {
      return getLocale('braveWalletSwapInsufficientLiquidity')
    }

    if (validationError === 'unknownError') {
      return getLocale('braveWalletSwapUnknownError')
    }

    return getLocale('braveWalletSwap')
  }, [validationError, fromAsset])

  const disclaimerText = getLocale('braveWalletSwapDisclaimer')
    .replace('$3', swapProvider === SwapProvider.Jupiter
      ? 'Jupiter'
      : '0x')
  const { beforeTag, duringTag, afterTag } = splitStringForTag(disclaimerText)

  const onClickSwapProvider = () => {
    const url = swapProvider === SwapProvider.Jupiter
      ? 'https://jup.ag'
      : 'https://0x.org'
    chrome.tabs.create({ url }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const onReset = () => {
    onInputChange('', 'from')
    onInputChange('', 'to')
    onInputChange('', 'rate')
    setPresetAmountValue(0)
  }

  const setPresetAmountValue = (percent: number) => {
    setSelectedPreset(percent as AmountPresetTypes)
    onSelectPresetAmount(percent)
  }

  const handleOnInputChange = (value: string, name: 'to' | 'from' | 'rate') => {
    if (name === 'from' && selectedPreset) {
      // Clear preset
      setSelectedPreset(undefined)
    }
    onInputChange(value, name)
  }

  return (
    <StyledWrapper>
      <SwapInputComponent
        componentType='fromAmount'
        onSelectPresetAmount={setPresetAmountValue}
        onInputChange={handleOnInputChange}
        selectedAssetInputAmount={fromAmount}
        inputName='from'
        selectedAssetBalance={fromAssetBalance}
        selectedAsset={fromAsset}
        selectedNetwork={selectedNetwork}
        onShowSelection={onShowAssetFrom}
        validationError={validationError}
        autoFocus={true}
        selectedPreset={selectedPreset}
      />
      <ArrowButton onClick={flipSwapAssets}>
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
        selectedNetwork={selectedNetwork}
        onShowSelection={onShowAssetTo}
        validationError={validationError}
      />
      <SwapInputComponent
        componentType='exchange'
        orderType={orderType}
        onToggleOrderType={onToggleOrderType}
        onInputChange={onInputChange}
        selectedAssetInputAmount={exchangeRate}
        inputName='rate'
        selectedAsset={fromAsset}
        onRefresh={onSwapQuoteRefresh}
      />
      <SwapInputComponent
        componentType='selector'
        orderType={orderType}
        onSelectSlippageTolerance={onSelectSlippageTolerance}
        onSelectExpiration={setOrderExpiration}
        slippageTolerance={slippageTolerance}
        orderExpiration={orderExpiration}
        customSlippageTolerance={customSlippageTolerance}
        onCustomSlippageToleranceChange={onCustomSlippageToleranceChange}
      />
      <SwapNavButton
        disabled={isSwapButtonDisabled}
        buttonType='primary'
        onClick={onSubmitSwap}
      >
        {
          isFetchingSwapQuote
            ? <SwapButtonLoader><LoaderIcon /></SwapButtonLoader>
            : <SwapButtonText>{submitText}</SwapButtonText>
        }
      </SwapNavButton>
      <ResetButton
        onClick={onReset}
        >
          {getLocale('braveWalletReset')}
      </ResetButton>
      <SwapFeesNoticeRow>
        <SwapFeesNoticeText>
          {getLocale('braveWalletSwapFeesNotice')
            .replace('$1', swapProvider === SwapProvider.Jupiter
              ? '0.85%'
              : '0.875%')}
        </SwapFeesNoticeText>
      </SwapFeesNoticeRow>
      <SwapDisclaimerRow>
        <SwapDisclaimerText>
          {beforeTag}
          <SwapDisclaimerButton onClick={onClickSwapProvider}>{duringTag}</SwapDisclaimerButton>
          {afterTag}
        </SwapDisclaimerText>
        <SwapTooltip
          text={swapProvider === SwapProvider.Jupiter
            ? getLocale('braveWalletJupiterSwapDisclaimerDescription')
            : getLocale('braveWalletSwapDisclaimerDescription')}
        >
          <AlertIcon />
        </SwapTooltip>
      </SwapDisclaimerRow>
    </StyledWrapper>
  )
}

export default Swap
