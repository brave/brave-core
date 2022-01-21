import * as React from 'react'

import { getLocale, splitStringForTag } from '../../../../common/locale'
import {
  BraveWallet,
  OrderTypes,
  BuySendSwapViewTypes,
  SlippagePresetObjectType,
  ExpirationPresetObjectType,
  ToOrFromType,
  SwapValidationErrorType,
  AmountPresetTypes
} from '../../../constants/types'
import SwapInputComponent from '../swap-input-component'
import { SwapTooltip } from '../../desktop'

// Styled Components
import {
  StyledWrapper,
  ArrowDownIcon,
  ArrowButton,
  SwapNavButton,
  SwapButtonText,
  SwapButtonLoader,
  SwapDisclaimerText,
  SwapDisclaimerButton,
  SwapDisclaimerRow,
  AlertIcon,
  SwapFeesNoticeRow,
  SwapFeesNoticeText,
  ResetRow
} from './style'
import { LoaderIcon } from 'brave-ui/components/icons'
import { ResetButton } from '../shared-styles'

export interface Props {
  toAsset: BraveWallet.BlockchainToken
  fromAsset: BraveWallet.BlockchainToken
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
  onFilterAssetList: (asset: BraveWallet.BlockchainToken) => void
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
  const [selectedPreset, setSelectedPreset] = React.useState<AmountPresetTypes | undefined>()

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
        .replace('$1', fromAsset.symbol)
    }

    if (validationError === 'insufficientLiquidity') {
      return getLocale('braveWalletSwapInsufficientLiquidity')
    }

    if (validationError === 'unknownError') {
      return getLocale('braveWalletSwapUnknownError')
    }

    return getLocale('braveWalletSwap')
  }, [validationError])

  const disclaimerText = getLocale('braveWalletSwapDisclaimer')
  const { beforeTag, duringTag, afterTag } = splitStringForTag(disclaimerText)

  const onClick0x = () => {
    chrome.tabs.create({ url: 'https://0x.org' }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const onReset = () => {
    onInputChange('', 'from')
    onInputChange('', 'to')
    setPresetAmountValue(0)
  }

  const setPresetAmountValue = (percent: number) => {
    setSelectedPreset(percent as AmountPresetTypes)
    onSelectPresetAmount(percent)
  }

  return (
    <StyledWrapper>
      <SwapInputComponent
        componentType='fromAmount'
        onSelectPresetAmount={setPresetAmountValue}
        onInputChange={onInputChange}
        selectedAssetInputAmount={fromAmount}
        inputName='from'
        selectedAssetBalance={fromAssetBalance}
        selectedAsset={fromAsset}
        onShowSelection={onShowAssetFrom}
        validationError={validationError}
        autoFocus={true}
        selectedPreset={selectedPreset}
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

      <ResetRow>
        <ResetButton
          onClick={onReset}
          >
            {getLocale('braveWalletReset')}
        </ResetButton>
      </ResetRow>

      <SwapFeesNoticeRow>
        <SwapFeesNoticeText>
          {getLocale('braveWalletSwapFeesNotice')}
        </SwapFeesNoticeText>
      </SwapFeesNoticeRow>

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
      <SwapDisclaimerRow>
        <SwapDisclaimerText>
          {beforeTag}
          <SwapDisclaimerButton onClick={onClick0x}>{duringTag}</SwapDisclaimerButton>
          {afterTag}
        </SwapDisclaimerText>
        <SwapTooltip
          text={getLocale('braveWalletSwapDisclaimerDescription')}
        >
          <AlertIcon />
        </SwapTooltip>
      </SwapDisclaimerRow>
    </StyledWrapper>
  )
}

export default Swap
