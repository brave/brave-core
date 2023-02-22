// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  useSelector
} from 'react-redux'

// Types
import { WalletState, BraveWallet } from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'
import { computeFiatAmount, computeFiatAmountToAssetValue } from '../../../../utils/pricing-utils'
import { CurrencySymbols } from '../../../../utils/currency-symbols'
import Amount from '../../../../utils/amount'

// Hooks
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'

// Components
import PopupModal from '../../../desktop/popup-modals'
import { NavButton } from '../../../extension'
import { withPlaceholderIcon } from '../../../shared'

// Styled Components
import {
  StyledWrapper,
  InputSection,
  AssetIcon,
  Text,
  AmountInput,
  PresetButton,
  ErrorBox,
  ErrorIcon
} from './sell-asset-modal.style'

import { VerticalSpacer, Row } from '../../../shared/style'

const MINIMUM_SELL_THRESHOLD = 50

interface Props {
  selectedAsset: BraveWallet.BlockchainToken
  selectedAssetsNetwork: BraveWallet.NetworkInfo | undefined
  sellAmount: string
  showSellModal: boolean
  sellAssetBalance: string
  setSellAmount: (value: string) => void
  openSellAssetLink: () => void
  onClose: () => void
}

export const SellAssetModal = (props: Props) => {
  const {
    selectedAsset,
    selectedAssetsNetwork,
    sellAmount,
    showSellModal,
    sellAssetBalance,
    setSellAmount,
    openSellAssetLink,
    onClose
  } = props

  // Redux
  const defaultCurrencies = useSelector(({ wallet }: { wallet: WalletState }) => wallet.defaultCurrencies)
  const spotPrices = useSelector(({ wallet }: { wallet: WalletState }) => wallet.transactionSpotPrices)

  // Refs
  const sellAssetModalRef = React.useRef<HTMLDivElement>(null)

  // Memos
  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'medium', marginLeft: 4, marginRight: 8 })
  }, [])

  const fiatBalance = React.useMemo(() => {
    return computeFiatAmount(
      spotPrices,
      {
        decimals: selectedAsset?.decimals ?? '',
        symbol: selectedAsset?.symbol ?? '',
        value: sellAssetBalance
      })
  }, [spotPrices, sellAssetBalance, selectedAsset?.symbol, selectedAsset?.decimals])

  const estimatedAssetAmount = React.useMemo(() => {
    if (sellAmount !== '') {
      return `~${computeFiatAmountToAssetValue(sellAmount, spotPrices, selectedAsset?.symbol ?? '')
        .formatAsAsset(6, selectedAsset.symbol)}`
    }
    return `0.00 ${selectedAsset?.symbol}`
  }, [sellAmount, spotPrices, selectedAsset.symbol])

  const formattedFiatBalance = React.useMemo(() => {
    return fiatBalance ? new Amount(fiatBalance.format(2)).formatAsFiat(defaultCurrencies.fiat) : undefined
  }, [defaultCurrencies.fiat, fiatBalance])

  const hasTooManyDecimals = React.useMemo(() => {
    if (sellAmount !== '') {
      return sellAmount.split('.')[1]?.length > 2
    }
    return false
  }, [sellAmount])

  const insufficientBalance = React.useMemo(() => {
    return Number(sellAmount) > new Amount(fiatBalance.format(2)).toNumber()
  }, [sellAmount, fiatBalance])

  // Computed
  const meetsMinimumSellThreshold = Number(sellAmount) >= MINIMUM_SELL_THRESHOLD

  const isSellButtonDisabled = sellAmount === '' || insufficientBalance || hasTooManyDecimals || !meetsMinimumSellThreshold

  const errorMessage = React.useMemo(() => {
    if (!meetsMinimumSellThreshold && sellAmount !== '') {
      return getLocale('braveWalletSellMinimumAmount').replace('$1', new Amount(MINIMUM_SELL_THRESHOLD).formatAsFiat(defaultCurrencies.fiat))
    }
    if (insufficientBalance) {
      return getLocale('braveWalletNotEnoughBalance').replace('$1', selectedAsset.symbol)
    }
    return ''
  }, [selectedAsset.symbol, meetsMinimumSellThreshold, sellAmount, defaultCurrencies.fiat, insufficientBalance])

  // Methods
  const handleInputAmountChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSellAmount(event.target.value)
    },
    [setSellAmount]
  )

  const onClickPreset = React.useCallback((preset: 'half' | 'max') => {
    if (preset === 'half') {
      setSellAmount(fiatBalance.div(2).format(2))
      return
    }
    setSellAmount(fiatBalance.format(2))
  }, [fiatBalance])

  const onCloseSellModal = React.useCallback(() => {
    setSellAmount('')
    onClose()
  }, [setSellAmount, onClose])

  // Hooks
  useOnClickOutside(
    sellAssetModalRef,
    onCloseSellModal,
    showSellModal
  )

  return (
    <PopupModal
      title={`${getLocale('braveWalletSell')} ${selectedAsset.name}`}
      onClose={onCloseSellModal}
      width='512px'
      borderRadius={16}
      ref={sellAssetModalRef}
      headerPaddingVertical={24}
      headerPaddingHorizontal={32}
    >
      <StyledWrapper>
        <InputSection>
          <Row
            marginBottom={19}
            justifyContent='space-between'
            width='100%'
          >
            <Row
              width='unset'
            >
              <Text
                textSize='12px'
                isBold={false}
                textColor='text03'
                marginRight={8}
              >
                {getLocale('braveWalletBalance')}
              </Text>
              <Text
                textSize='12px'
                isBold={true}
                textColor='text01'
              >
                {formattedFiatBalance}
              </Text>
            </Row>
            <Row
              width='unset'
            >
              <PresetButton
                onClick={() => onClickPreset('half')}
                marginRight={4}
              >
                {getLocale('braveWalletSendHalf')}
              </PresetButton>
              <PresetButton
                onClick={() => onClickPreset('max')}
              >
                {getLocale('braveWalletSendMax')}
              </PresetButton>
            </Row>
          </Row>
          <Row
            marginBottom={16}
            width='100%'
            justifyContent='space-between'
          >
            <Row
              width='unset'
            >
              <Text
                textSize='32px'
                isBold={true}
                textColor={sellAmount === '' ? 'text03' : 'text01'}
              >
                {CurrencySymbols[defaultCurrencies.fiat]}
              </Text>
              <AmountInput
                placeholder='0.00'
                value={sellAmount}
                onChange={handleInputAmountChange}
              />
            </Row>
            <Row
              width='unset'
            >
              <AssetIconWithPlaceholder asset={selectedAsset} network={selectedAssetsNetwork} />
              <Text
                textSize='22px'
                isBold={true}
                textColor='text01'
              >
                {selectedAsset.symbol}
              </Text>
            </Row>
          </Row>
          <Row
            justifyContent='flex-end'
            width='100%'
          >
            <Text
              textSize='14px'
              isBold={false}
              textColor='text03'
            >
              {estimatedAssetAmount}
            </Text>
          </Row>
        </InputSection>
        {errorMessage !== '' &&
          <ErrorBox>
            <ErrorIcon />
            <Text
              textSize='14px'
              isBold={false}
              textColor='text01'
            >
              {errorMessage}
            </Text>
          </ErrorBox>
        }
        <VerticalSpacer space={8} />
        <NavButton
          disabled={isSellButtonDisabled}
          buttonType='primary'
          minHeight='52px'
          text={insufficientBalance
            ? getLocale('braveWalletSwapInsufficientBalance')
            // Ramp is hardcoded for now, but we can update with selectedProvider.name
            // once we add more offramp providers.
            : getLocale('braveWalletSellWithProvider').replace('$1', 'Ramp')
          }
          onSubmit={openSellAssetLink}
        />
      </StyledWrapper>
    </PopupModal>
  )
}
