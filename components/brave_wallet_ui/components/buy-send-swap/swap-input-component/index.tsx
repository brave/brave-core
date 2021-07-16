import * as React from 'react'
import { AssetOptionType, OrderTypes, SlippagePresetObjectType, ExpirationPresetObjectType } from '../../../constants/types'
import { AmountPresetOptions } from '../../../options/amount-preset-options'
import { SlippagePresetOptions } from '../../../options/slippage-preset-options'
import { ExpirationPresetOptions } from '../../../options/expiration-preset-options'
import locale from '../../../constants/locale'

// Styled Components
import {
  RefreshButton,
  RefreshIcon,
  MarketLimitButton,
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
  Row
} from './style'

import { BubbleContainer } from '../shared-styles'

export interface Props {
  componentType: 'toAmount' | 'fromAmount' | 'toAddress' | 'buyAmount' | 'exchange' | 'selector'
  selectedAssetBalance?: string
  selectedAsset?: AssetOptionType
  selectedAssetInputAmount?: string
  inputName?: string
  orderType?: OrderTypes
  slippageTolerance?: SlippagePresetObjectType
  orderExpiration?: ExpirationPresetObjectType
  onInputChange?: (value: string, name: string) => void
  onSelectPresetAmount?: (percent: number) => void
  onSelectSlippageTolerance?: (slippage: SlippagePresetObjectType) => void
  onSelectExpiration?: (expiration: ExpirationPresetObjectType) => void
  onToggleOrderType?: () => void
  onShowSelection?: () => void
  onRefresh?: () => void
}

function SwapInputComponent (props: Props) {
  const {
    selectedAsset,
    selectedAssetBalance,
    componentType,
    selectedAssetInputAmount,
    inputName,
    orderType,
    slippageTolerance,
    orderExpiration,
    onInputChange,
    onRefresh,
    onSelectPresetAmount,
    onSelectSlippageTolerance,
    onSelectExpiration,
    onToggleOrderType,
    onShowSelection
  } = props
  const [spin, setSpin] = React.useState<number>(0)
  const [expandSelector, setExpandSelector] = React.useState<boolean>(false)

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

  const setPresetAmmountValue = (percent: number) => () => {
    if (onSelectPresetAmount) {
      onSelectPresetAmount(percent)
    }
  }

  const getTitle = () => {
    switch (componentType) {
      case 'fromAmount':
        return locale.swapFrom
      case 'toAmount':
        if (orderType === 'market') {
          return `${locale.swapTo} (${locale.swapEstimate})`
        } else {
          return locale.swapTo
        }
      case 'buyAmount':
        return locale.buy
      case 'exchange':
        if (orderType === 'market') {
          return `${locale.swapMarket} ${locale.swapPriceIn} ${selectedAsset?.symbol}`
        } else {
          return `${locale.swapPriceIn} ${selectedAsset?.symbol}`
        }
      case 'selector':
        if (orderType === 'market') {
          return 'Slippage tolerance'
        } else {
          return 'Expires in'
        }
      case 'toAddress':
        return locale.swapTo
    }
  }

  const onInputChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (onInputChange) {
      onInputChange(event.target.value, event.target.name)
    }
  }

  const resetAnimation = () => {
    setSpin(0)
  }

  return (
    <BubbleContainer>
      {componentType !== 'selector' &&
        <>
          <Row>
            <FromBalanceText isExchange={componentType === 'exchange'}>{getTitle()}</FromBalanceText>
            {componentType === 'exchange' ? (
              <MarketLimitButton onClick={onToggleOrderType}>{orderType === 'market' ? locale.swapLimit : locale.swapMarket}</MarketLimitButton>
            ) : (
              <FromBalanceText>{locale.balance}: {selectedAssetBalance}</FromBalanceText>
            )
            }
          </Row>
          <Row isExchange={componentType === 'exchange'}>
            <Input
              isExchange={componentType === 'exchange'}
              type='number'
              placeholder='0.0'
              value={selectedAssetInputAmount}
              name={inputName}
              onChange={onInputChanged}
              hasError={selectedAssetBalance && componentType === 'fromAmount' ? Number(selectedAssetInputAmount) > Number(selectedAssetBalance) : false}
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
            {componentType !== 'exchange' &&
              <AssetButton onClick={onShowSelection}>
                <AssetIcon icon={selectedAsset?.icon} />
                <AssetTicker>{selectedAsset?.symbol}</AssetTicker>
                <CaratDownIcon />
              </AssetButton>
            }
          </Row>
          {componentType === 'fromAmount' &&
            <PresetRow>
              {AmountPresetOptions.map((preset) =>
                <PresetButton
                  key={preset.id}
                  onClick={setPresetAmmountValue(preset.id)}
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
              <SelectValueText>{orderType === 'market' ? `${slippageTolerance?.slippage}%` : `${orderExpiration?.expiration} days`}</SelectValueText>
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
                      onClick={setPresetSlippageValue(preset)}
                    >
                      {preset.slippage}%
                    </PresetButton>
                  )}
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
    </BubbleContainer >
  )
}

export default SwapInputComponent
