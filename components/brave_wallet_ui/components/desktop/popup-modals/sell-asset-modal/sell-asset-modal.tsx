// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet } from '../../../../constants/types'

// Utils
import {
  formatTokenBalanceWithSymbol,
  getPercentAmount
} from '../../../../utils/balance-utils'
import { getLocale } from '../../../../../common/locale'
import Amount from '../../../../utils/amount'

// Hooks
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'
import {
  useScopedBalanceUpdater //
} from '../../../../common/hooks/use-scoped-balance-updater'

// Components
import PopupModal from '../../../desktop/popup-modals'
import {
  withPlaceholderIcon //
} from '../../../shared/create-placeholder-icon/index'

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

import {
  VerticalSpacer,
  Row,
  Column,
  LeoSquaredButton
} from '../../../shared/style'

interface Props {
  selectedAsset: BraveWallet.BlockchainToken
  sellAmount: string
  showSellModal: boolean
  sellAssetBalance: string
  account?: BraveWallet.AccountInfo
  setSellAmount: (value: string) => void
  openSellAssetLink: () => void
  onClose: () => void
}

export const SellAssetModal = (props: Props) => {
  const {
    selectedAsset,
    sellAmount,
    showSellModal,
    sellAssetBalance,
    account,
    setSellAmount,
    openSellAssetLink,
    onClose
  } = props

  // Refs
  const sellAssetModalRef = React.useRef<HTMLDivElement>(null)

  // Memos
  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, {
      size: 'medium',
      marginLeft: 4,
      marginRight: 8
    })
  }, [])

  // Computed
  const insufficientBalance = new Amount(sellAmount)
    .multiplyByDecimals(selectedAsset.decimals)
    .gt(sellAssetBalance)
  const isInvalidAmount =
    sellAmount.startsWith('0') && !sellAmount.includes('.')
  const isNotNumeric = new Amount(sellAmount).isNaN()
  const isSellButtonDisabled =
    isInvalidAmount ||
    isNotNumeric ||
    sellAmount === '' ||
    Number(sellAmount) <= 0 ||
    insufficientBalance

  // Methods
  const handleInputAmountChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSellAmount(event.target.value)
    },
    [setSellAmount]
  )

  const { data: tokenBalancesRegistry } = useScopedBalanceUpdater(
    account && selectedAsset
      ? {
          network: {
            chainId: selectedAsset.chainId,
            coin: account.accountId.coin
          },
          accounts: [account],
          tokens: [selectedAsset]
        }
      : skipToken
  )

  const setPresetAmountValue = React.useCallback(
    (percent: number) => {
      if (!selectedAsset || !account) {
        return
      }

      setSellAmount(
        getPercentAmount(
          selectedAsset,
          account.accountId,
          percent,
          tokenBalancesRegistry
        )
      )
    },
    [setSellAmount, selectedAsset, account, tokenBalancesRegistry]
  )

  const onCloseSellModal = React.useCallback(() => {
    setSellAmount('')
    onClose()
  }, [setSellAmount, onClose])

  // Hooks
  useOnClickOutside(sellAssetModalRef, onCloseSellModal, showSellModal)

  return (
    <PopupModal
      title={`${getLocale('braveWalletSell')} ${selectedAsset.name}`}
      onClose={onCloseSellModal}
      width='512px'
      ref={sellAssetModalRef}
      headerPaddingVertical={'24px'}
      headerPaddingHorizontal={'32px'}
    >
      <StyledWrapper>
        <Column fullWidth={true}>
          <InputSection>
            <Row
              marginBottom={19}
              justifyContent='space-between'
              width='100%'
            >
              <Row width='unset'>
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
                  {formatTokenBalanceWithSymbol(
                    sellAssetBalance,
                    selectedAsset.decimals,
                    selectedAsset.symbol
                  )}
                </Text>
              </Row>
              <Row width='unset'>
                <PresetButton
                  onClick={() => setPresetAmountValue(0.5)}
                  marginRight={4}
                >
                  {getLocale('braveWalletSendHalf')}
                </PresetButton>
                <PresetButton onClick={() => setPresetAmountValue(1)}>
                  {getLocale('braveWalletSendMax')}
                </PresetButton>
              </Row>
            </Row>
            <Row
              marginBottom={16}
              width='100%'
              justifyContent='space-between'
            >
              <Row width='unset'>
                <AmountInput
                  placeholder='0.00'
                  value={sellAmount}
                  onChange={handleInputAmountChange}
                />
              </Row>
              <Row width='unset'>
                <AssetIconWithPlaceholder asset={selectedAsset} />
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
          {insufficientBalance && !isInvalidAmount && (
            <ErrorBox>
              <ErrorIcon />
              <Text
                textSize='14px'
                isBold={false}
                textColor='text01'
                textAlign='left'
              >
                {getLocale('braveWalletNotEnoughBalance').replace(
                  '$1',
                  selectedAsset.symbol
                )}
              </Text>
            </ErrorBox>
          )}
        </Column>
        <VerticalSpacer space={8} />
        <LeoSquaredButton
          isDisabled={isSellButtonDisabled}
          onClick={openSellAssetLink}
        >
          {getLocale('braveWalletSellWithProvider').replace('$1', 'Ramp')}
        </LeoSquaredButton>
      </StyledWrapper>
    </PopupModal>
  )
}
