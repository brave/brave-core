import * as React from 'react'
import {
  AccountAssetOptionType,
  OrderTypes,
  SlippagePresetObjectType,
  ExpirationPresetObjectType,
  SwapValidationErrorType,
  AmountPresetTypes
} from '../../../constants/types'
import { AmountPresetOptions } from '../../../options/amount-preset-options'
import { SlippagePresetOptions } from '../../../options/slippage-preset-options'
import { ExpirationPresetOptions } from '../../../options/expiration-preset-options'
import { formatWithCommasAndDecimals } from '../../../utils/format-prices'
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  RefreshButton,
  RefreshIcon,
  FromBalanceText,
  AssetIcon,
  AssetButton,
  AssetTicker,
  CaratDownIcon,
  PresetButton,
  PresetRow,
  SelectValueText,
  SelectText,
  Input,
  Row,
  PasteIcon,
  PasteButton,
  SlippageInput,
  WarningText
} from './style'

import { BubbleContainer } from '../shared-styles'

export type BuySendSwapInputType =
  | 'toAmount'
  | 'fromAmount'
  | 'toAddress'
  | 'buyAmount'
  | 'exchange'
  | 'selector'

export interface Props {
  componentType: BuySendSwapInputType
  selectedAssetBalance?: string
  selectedAsset?: AccountAssetOptionType
  selectedAssetInputAmount?: string
  toAddress?: string
  inputName?: string
  orderType?: OrderTypes
  slippageTolerance?: SlippagePresetObjectType
  orderExpiration?: ExpirationPresetObjectType
  validationError?: SwapValidationErrorType
  customSlippageTolerance?: string
  onCustomSlippageToleranceChange?: (value: string) => void
  onInputChange?: (value: string, name: string) => void
  onSelectPresetAmount?: (percent: number) => void
  onSelectSlippageTolerance?: (slippage: SlippagePresetObjectType) => void
  onSelectExpiration?: (expiration: ExpirationPresetObjectType) => void
  onToggleOrderType?: () => void
  onShowSelection?: () => void
  onRefresh?: () => void
  onPaste?: () => void
}

function SwapInputComponent (props: Props) {
  const {
    selectedAsset,
    selectedAssetBalance,
    componentType,
    selectedAssetInputAmount,
    inputName,
    toAddress,
    orderType,
    slippageTolerance,
    orderExpiration,
    validationError,
    customSlippageTolerance,
    onCustomSlippageToleranceChange,
    onInputChange,
    onPaste,
    onRefresh,
    onSelectPresetAmount,
    onSelectSlippageTolerance,
    onSelectExpiration,
    onShowSelection
  } = props
  const [spin, setSpin] = React.useState<number>(0)
  const [expandSelector, setExpandSelector] = React.useState<boolean>(false)
  const [showSlippageWarning, setShowSlippageWarning] = React.useState<boolean>(false)
  const [selectedPreset, setSelectedPreset] = React.useState<AmountPresetTypes | undefined>()

  const toggleExpandSelector = () => {
    setExpandSelector(!expandSelector)
  }

  const refresh = () => {
    if (onRefresh) {
      onRefresh()
    }
    setSpin(1)
  }

  const setPresetSlippageValue = (slippage: SlippagePresetObjectType) => () => {
    if (onSelectSlippageTolerance) {
      onSelectSlippageTolerance(slippage)
      setExpandSelector(false)
    }
  }

  const setExpirationValue = (expiration: ExpirationPresetObjectType) => () => {
    if (onSelectExpiration) {
      onSelectExpiration(expiration)
      setExpandSelector(false)
    }
  }

  const setPresetAmountValue = (percent: AmountPresetTypes) => () => {
    if (onSelectPresetAmount) {
      setSelectedPreset(percent)
      onSelectPresetAmount(percent)
    }
  }

  const getTitle = () => {
    switch (componentType) {
      case 'fromAmount':
        return getLocale('braveWalletSwapFrom')
      case 'toAmount':
        if (orderType === 'market') {
          return `${getLocale('braveWalletSwapTo')} (${getLocale('braveWalletSwapEstimate')})`
        } else {
          return getLocale('braveWalletSwapTo')
        }
      case 'buyAmount':
        return getLocale('braveWalletBuy')
      case 'exchange':
        if (orderType === 'market') {
          return `${getLocale('braveWalletSwapMarket')} ${getLocale('braveWalletSwapPriceIn')} ${selectedAsset?.asset.symbol}`
        } else {
          return `${getLocale('braveWalletSwapPriceIn')} ${selectedAsset?.asset.symbol}`
        }
      case 'selector':
        if (orderType === 'market') {
          return getLocale('braveWalletSlippageToleranceTitle')
        } else {
          return getLocale('braveWalletExpiresInTitle')
        }
      case 'toAddress':
        return getLocale('braveWalletSwapTo')
    }
  }

  const onInputChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (onInputChange) {
      onInputChange(event.target.value, event.target.name)
    }
  }

  React.useMemo(() => {
    // Show Warning if slippage is to high
    if (Number(customSlippageTolerance) >= 6) {
      setShowSlippageWarning(true)
      return
    }
    setShowSlippageWarning(false)
  }, [customSlippageTolerance])

  const handleCustomSlippageToleranceChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (onCustomSlippageToleranceChange) {
      // This will only formate to only allow Numbers and remove multiple . decimals
      const value = event.target.value.replace(/[^0-9.]/g, '').replace(/(\..*?)\..*/g, '$1')
      // Sets the value to not go higher than 100
      if (Number(value) > 100) {
        onCustomSlippageToleranceChange('100')
        return
      }
      // Prevents double 00 before decimal place and formats to 0. if value starts with .
      if (value === '00' || value === '.') {
        onCustomSlippageToleranceChange('0.')
        return
      }
      onCustomSlippageToleranceChange(value)
    }
  }

  const resetAnimation = () => {
    setSpin(0)
  }

  const fromAmountHasErrors = validationError && (
    validationError === 'insufficientBalance' ||
    validationError === 'insufficientEthBalance'
  )

  return (
    <BubbleContainer>
      {componentType !== 'selector' &&
        <>
          <Row>
            <FromBalanceText componentType={componentType}>{getTitle()}</FromBalanceText>

            {/* Limit orders on Swap are currently disabled.
              componentType === 'exchange' &&
                <MarketLimitButton onClick={onToggleOrderType}>{orderType === 'market' ? locale.swapLimit : locale.swapMarket}</MarketLimitButton>
            */}

            {componentType !== 'exchange' && componentType !== 'toAddress' && componentType !== 'buyAmount' &&
              <FromBalanceText>{getLocale('braveWalletBalance')}: {formatWithCommasAndDecimals(selectedAssetBalance?.toString() ?? '')}</FromBalanceText>
            }
            {componentType === 'toAddress' &&
              <PasteButton onClick={onPaste}>
                <PasteIcon />
              </PasteButton>
            }
          </Row>
          <Row componentType={componentType}>
            {componentType === 'buyAmount' &&
              <AssetTicker>$</AssetTicker>
            }
            <Input
              componentType={componentType}
              type={componentType === 'toAddress' ? 'text' : 'number'}
              placeholder={componentType === 'toAddress' ? '0x address or url' : '0'}
              value={componentType === 'toAddress' ? toAddress : selectedAssetInputAmount}
              name={inputName}
              onChange={onInputChanged}
              spellCheck={false}
              hasError={componentType === 'fromAmount' && fromAmountHasErrors}
              disabled={orderType === 'market' && componentType === 'exchange' || orderType === 'limit' && componentType === 'toAmount'}
            />
            {componentType === 'exchange' && orderType === 'market' &&
              <RefreshButton onClick={refresh}>
                <RefreshIcon
                  onAnimationEnd={resetAnimation}
                  spin={spin}
                />
              </RefreshButton>
            }
            {componentType !== 'exchange' && componentType !== 'toAddress' &&
              <AssetButton onClick={onShowSelection}>
                <AssetIcon icon={selectedAsset?.asset.logo} />
                <AssetTicker>{selectedAsset?.asset.symbol}</AssetTicker>
                <CaratDownIcon />
              </AssetButton>
            }
          </Row>
          {componentType === 'fromAmount' &&
            <PresetRow>
              {AmountPresetOptions().map((preset, idx) =>
                <PresetButton
                  isSelected={selectedPreset === preset.value}
                  key={idx}
                  onClick={setPresetAmountValue(preset.value)}
                >
                  {preset.name}
                </PresetButton>
              )}
            </PresetRow>
          }
        </>
      }
      {componentType === 'selector' &&
        <>
          <Row>
            <SelectText>{getTitle()}</SelectText>
            <AssetButton onClick={toggleExpandSelector}>
              <SelectValueText>{orderType === 'market' ? customSlippageTolerance ? `${customSlippageTolerance}%` : `${slippageTolerance?.slippage}%` : `${orderExpiration?.expiration} days`}</SelectValueText>
              <CaratDownIcon />
            </AssetButton>
          </Row>
          {expandSelector &&
            <PresetRow>
              {orderType === 'market' ? (
                <>
                  {SlippagePresetOptions.map((preset) =>
                    <PresetButton
                      key={preset.id}
                      isSlippage={true}
                      isSelected={customSlippageTolerance === '' ? slippageTolerance?.slippage === preset.slippage : false}
                      onClick={setPresetSlippageValue(preset)}
                    >
                      {preset.slippage}%
                    </PresetButton>
                  )}
                  <SlippageInput
                    value={customSlippageTolerance}
                    placeholder='%'
                    type='text'
                    isSelected={customSlippageTolerance !== ''}
                    onChange={handleCustomSlippageToleranceChange}
                    maxLength={4}
                  />
                </>
              ) : (
                <>
                  {ExpirationPresetOptions.map((preset) =>
                    <PresetButton
                      key={preset.id}
                      onClick={setExpirationValue(preset)}
                    >
                      {preset.name}
                    </PresetButton>
                  )}
                </>
              )}
            </PresetRow>
          }
        </>
      }
      {showSlippageWarning &&
        <WarningText>{getLocale('braveWalletSlippageToleranceWarning')}</WarningText>
      }
    </BubbleContainer >
  )
}

export default SwapInputComponent
