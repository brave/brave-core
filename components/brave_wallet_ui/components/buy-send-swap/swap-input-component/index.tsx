// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useSelector } from 'react-redux'
import {
  BraveWallet,
  OrderTypes,
  SlippagePresetObjectType,
  ExpirationPresetObjectType,
  SwapValidationErrorType,
  AmountPresetTypes,
  DefaultCurrencies,
  WalletState
} from '../../../constants/types'
import { AmountPresetOptions } from '../../../options/amount-preset-options'
import { SlippagePresetOptions } from '../../../options/slippage-preset-options'
import { ExpirationPresetOptions } from '../../../options/expiration-preset-options'
import { getLocale } from '../../../../common/locale'
import { withPlaceholderIcon, Tooltip } from '../../shared'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { CurrencySymbols } from '../../../utils/currency-symbols'
import Amount from '../../../utils/amount'

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
  WarningText,
  AddressConfirmationText,
  ButtonLeftSide,
  LearnMoreButton,
  WarningRow,
  Spacer
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
  autoFocus?: boolean
  componentType: BuySendSwapInputType
  selectedAssetBalance?: string
  selectedAsset?: BraveWallet.BlockchainToken | undefined
  selectedNetwork?: BraveWallet.NetworkInfo
  selectedAssetInputAmount?: string
  addressError?: string
  addressWarning?: string
  toAddressOrUrl?: string
  toAddress?: string
  inputName?: string
  orderType?: OrderTypes
  slippageTolerance?: SlippagePresetObjectType
  orderExpiration?: ExpirationPresetObjectType
  validationError?: SwapValidationErrorType | undefined
  customSlippageTolerance?: string
  defaultCurrencies?: DefaultCurrencies
  selectedPreset?: AmountPresetTypes | undefined
  onCustomSlippageToleranceChange?: (value: string) => void
  onInputChange?: (value: string, name: string) => void
  onSelectPresetAmount?: (percent: number) => void
  onSelectSlippageTolerance?: (slippage: SlippagePresetObjectType) => void
  onSelectExpiration?: (expiration: ExpirationPresetObjectType) => void
  onToggleOrderType?: () => void
  onShowSelection?: () => void
  onRefresh?: () => void
  onPaste?: () => void
  onShowCurrencySelection?: () => void
}

function SwapInputComponent (props: Props) {
  const {
    autoFocus,
    selectedAsset,
    selectedNetwork,
    selectedAssetBalance,
    componentType,
    selectedAssetInputAmount,
    inputName,
    addressError,
    addressWarning,
    toAddressOrUrl,
    toAddress,
    orderType,
    slippageTolerance,
    orderExpiration,
    validationError,
    customSlippageTolerance,
    selectedPreset,
    onCustomSlippageToleranceChange,
    onInputChange,
    onPaste,
    onRefresh,
    onSelectPresetAmount,
    onSelectSlippageTolerance,
    onSelectExpiration,
    onShowSelection,
    onShowCurrencySelection
  } = props
  const [spin, setSpin] = React.useState<number>(0)
  const [expandSelector, setExpandSelector] = React.useState<boolean>(false)
  const [showSlippageWarning, setShowSlippageWarning] = React.useState<boolean>(false)

  // redux
  const {
    selectedCurrency: reduxSelectedCurrency,
    onRampCurrencies: currencies
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

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
          return `${getLocale('braveWalletSwapMarket')} ${getLocale('braveWalletSwapPriceIn')} ${selectedAsset?.symbol}`
        } else {
          return `${getLocale('braveWalletSwapPriceIn')} ${selectedAsset?.symbol}`
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

  const selectedCurrency = React.useMemo(() => {
    return reduxSelectedCurrency || currencies[0]
  }, [reduxSelectedCurrency, currencies])

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

  const getAssetSymbol = (symbol?: string) => {
    // Ramp assets have the format: ChainNativeTokenSymbol_AssetSymbol e.g MATIC_BSC
    // This returns just the AssetSymbol
    return symbol && symbol.includes('_')
      ? symbol.split('_')[1]
      : symbol
  }

  const fromAmountHasErrors = validationError && [
    'insufficientBalance',
    'insufficientFundsForGas',
    'fromAmountDecimalsOverflow'
  ].includes(validationError)

  const toAmountHasErrors = validationError && [
    'toAmountDecimalsOverflow'
  ].includes(validationError)

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'small', marginLeft: 4, marginRight: 8 })
  }, [])

  const formattedAssetBalance = selectedAssetBalance
    ? getLocale('braveWalletBalance') + ': ' + new Amount(selectedAssetBalance)
      .divideByDecimals(selectedAsset?.decimals ?? 18)
      .format(6, true)
    : ''

  const onClickLearnMore = () => {
    chrome.tabs.create({ url: 'https://support.brave.com/hc/en-us/articles/4441999049101' }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const placeholderText = React.useMemo((): string => {
    return componentType === 'toAddress'
      ? selectedNetwork?.coin === BraveWallet.CoinType.ETH
        ? getLocale('braveWalletSendPlaceholder')
        : getLocale('braveWalletSendNoURLPlaceholder')
      : '0'
  }, [selectedNetwork, componentType])

  return (
    <BubbleContainer>
      {componentType !== 'selector' &&
        <>
          {!(selectedAsset?.isErc721 || selectedAsset?.isNft) &&
            <Row>
              <FromBalanceText componentType={componentType}>{getTitle()}</FromBalanceText>

              {/* Limit orders on Swap are currently disabled.
              componentType === 'exchange' &&
                <MarketLimitButton onClick={onToggleOrderType}>{orderType === 'market' ? locale.swapLimit : locale.swapMarket}</MarketLimitButton>
            */}

              {componentType !== 'exchange' && componentType !== 'toAddress' && componentType !== 'buyAmount' &&
                <FromBalanceText>{formattedAssetBalance}</FromBalanceText>
              }
              {componentType === 'toAddress' &&
                <PasteButton onClick={onPaste}>
                  <PasteIcon />
                </PasteButton>
              }
            </Row>
          }
          <Row componentType={componentType}>
            {componentType === 'buyAmount' &&
              <AssetButton onClick={onShowCurrencySelection}>
                <AssetTicker>{CurrencySymbols[selectedCurrency?.currencyCode]}</AssetTicker>
                <CaratDownIcon />
                <Spacer />
              </AssetButton>
            }
            {!(selectedAsset?.isErc721 || selectedAsset?.isNft) &&
              <Input
                componentType={componentType}
                type={componentType === 'toAddress' ? 'text' : 'number'}
                placeholder={placeholderText}
                value={componentType === 'toAddress' ? toAddressOrUrl : selectedAssetInputAmount}
                name={inputName}
                onChange={onInputChanged}
                spellCheck={false}
                hasError={
                  (componentType === 'fromAmount' && !!fromAmountHasErrors) ||
                  (componentType === 'toAmount' && !!toAmountHasErrors)
                }
                disabled={
                  (orderType === 'market' && componentType === 'exchange') ||
                  (orderType === 'limit' && componentType === 'toAmount') ||
                  (selectedNetwork?.chainId === BraveWallet.SOLANA_MAINNET && componentType === 'toAmount')
                }
                autoFocus={autoFocus}
              />
            }
            {componentType === 'exchange' && orderType === 'market' &&
              <RefreshButton onClick={refresh}>
                <RefreshIcon
                  onAnimationEnd={resetAnimation}
                  spin={spin}
                />
              </RefreshButton>
            }
            {componentType !== 'exchange' && componentType !== 'toAddress' &&
              <AssetButton isERC721={(selectedAsset?.isErc721 || selectedAsset?.isNft)} onClick={onShowSelection}>
                <ButtonLeftSide>
                  <AssetIconWithPlaceholder asset={selectedAsset} network={selectedNetwork} />
                  <AssetTicker>
                    {getAssetSymbol(selectedAsset?.symbol)} {
                      selectedAsset?.isErc721 && selectedAsset?.tokenId
                        ? '#' + new Amount(selectedAsset.tokenId).toNumber()
                        : ''
                    }
                  </AssetTicker>
                </ButtonLeftSide>
                {onShowSelection && <CaratDownIcon />}
              </AssetButton>
            }
          </Row>
          {componentType === 'fromAmount' && !(selectedAsset?.isErc721 || selectedAsset?.isNft) &&
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

      {componentType === 'fromAmount' && validationError === 'fromAmountDecimalsOverflow' &&
        <WarningText>{getLocale('braveWalletDecimalPlacesError')}</WarningText>
      }
      {componentType === 'toAmount' && validationError === 'toAmountDecimalsOverflow' &&
        <WarningText>{getLocale('braveWalletDecimalPlacesError')}</WarningText>
      }
      {showSlippageWarning &&
        <WarningText>{getLocale('braveWalletSlippageToleranceWarning')}</WarningText>
      }
      {componentType === 'toAddress' && addressError &&
        <WarningRow>
          <WarningText>{addressError}</WarningText>
          {addressError === getLocale('braveWalletNotValidChecksumAddressError') &&
            <LearnMoreButton onClick={onClickLearnMore}>
              {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
            </LearnMoreButton>
          }
        </WarningRow>
      }
      {componentType === 'toAddress' && addressWarning &&
        <WarningRow>
          <WarningText isWarning={true}>{addressWarning}</WarningText>
          <LearnMoreButton onClick={onClickLearnMore}>
            {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
          </LearnMoreButton>
        </WarningRow>
      }
      {componentType === 'toAddress' && toAddress !== toAddressOrUrl && !addressError &&
        <>
          <Tooltip
              text={toAddress ?? ''}
              isAddress={true}
            >
              <AddressConfirmationText>{reduceAddress(toAddress ?? '')}</AddressConfirmationText>
            </Tooltip>
        </>
      }
    </BubbleContainer >
  )
}

export default SwapInputComponent
