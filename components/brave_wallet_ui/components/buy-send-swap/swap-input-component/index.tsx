import * as React from 'react'
import { AccountAssetOptionType, OrderTypes, SlippagePresetObjectType, ExpirationPresetObjectType } from '../../../constants/types'
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
  Row,
  PasteIcon,
  PasteButton
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
    onInputChange,
    onPaste,
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
          return `${locale.swapMarket} ${locale.swapPriceIn} ${selectedAsset?.asset.symbol}`
        } else {
          return `${locale.swapPriceIn} ${selectedAsset?.asset.symbol}`
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
            <FromBalanceText componentType={componentType}>{getTitle()}</FromBalanceText>
            {componentType === 'exchange' &&
              <MarketLimitButton onClick={onToggleOrderType}>{orderType === 'market' ? locale.swapLimit : locale.swapMarket}</MarketLimitButton>
            }
            {componentType !== 'exchange' && componentType !== 'toAddress' && componentType !== 'buyAmount' &&
              <FromBalanceText>{locale.balance}: {selectedAssetBalance}</FromBalanceText>
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
            {componentType !== 'exchange' && componentType !== 'toAddress' &&
              <AssetButton onClick={onShowSelection}>
                <AssetIcon icon={selectedAsset?.asset.icon} />
                <AssetTicker>{selectedAsset?.asset.symbol}</AssetTicker>
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
