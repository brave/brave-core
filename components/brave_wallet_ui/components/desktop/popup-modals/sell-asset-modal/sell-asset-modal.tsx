// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet, WalletAccountType } from '../../../../constants/types'

// Utils
import { formatTokenBalanceWithSymbol } from '../../../../utils/balance-utils'
import { getLocale } from '../../../../../common/locale'
import Amount from '../../../../utils/amount'

// Hooks
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'
import { usePreset } from '../../../../common/hooks'

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

interface Props {
  selectedAsset: BraveWallet.BlockchainToken
  selectedAssetsNetwork: BraveWallet.NetworkInfo | undefined
  sellAmount: string
  showSellModal: boolean
  sellAssetBalance: string
  account?: WalletAccountType
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
    account,
    setSellAmount,
    openSellAssetLink,
    onClose
  } = props

  // Hooks
  const onSelectPresetAmount = usePreset(
    {
      onSetAmount: setSellAmount,
      asset: selectedAsset,
      account: account
    }
  )

  // Refs
  const sellAssetModalRef = React.useRef<HTMLDivElement>(null)

  // Memos
  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'medium', marginLeft: 4, marginRight: 8 })
  }, [])

  // Computed
  const insufficientBalance = new Amount(sellAmount).multiplyByDecimals(selectedAsset.decimals).gt(sellAssetBalance)
  const isInvalidAmount = sellAmount.startsWith('0') && !sellAmount.includes('.')
  const isNotNumeric = new Amount(sellAmount).isNaN()
  const isSellButtonDisabled = isInvalidAmount || isNotNumeric || sellAmount === '' || Number(sellAmount) <= 0 || insufficientBalance


  // Methods
  const handleInputAmountChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSellAmount(event.target.value)
    },
    [setSellAmount]
  )

  const setPresetAmountValue = React.useCallback((percent: number) => {
    onSelectPresetAmount(percent)
  }, [onSelectPresetAmount])

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
                {formatTokenBalanceWithSymbol(sellAssetBalance, selectedAsset.decimals, selectedAsset.symbol)}
              </Text>
            </Row>
            <Row
              width='unset'
            >
              <PresetButton
                onClick={() => setPresetAmountValue(0.5)}
                marginRight={4}
              >
                {getLocale('braveWalletSendHalf')}
              </PresetButton>
              <PresetButton
                onClick={() => setPresetAmountValue(1)}
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
        </InputSection>
        {insufficientBalance && !isInvalidAmount &&
          <ErrorBox>
            <ErrorIcon />
            <Text
              textSize='14px'
              isBold={false}
              textColor='text01'
            >
              {getLocale('braveWalletNotEnoughBalance').replace('$1', selectedAsset.symbol)}
            </Text>
          </ErrorBox>
        }
        <VerticalSpacer space={8} />
        <NavButton
          disabled={isSellButtonDisabled}
          buttonType='primary'
          minHeight='52px'
          text={
            // Ramp is hardcoded for now, but we can update with selectedProvider.name
            // once we add more offramp providers.
            getLocale('braveWalletSellWithProvider').replace('$1', 'Ramp')
          }
          onSubmit={openSellAssetLink}
        />
      </StyledWrapper>
    </PopupModal>
  )
}
