// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Button from '@brave/leo/react/button'

// Types
import {
  BraveWallet,
  MaxPriorityFeeOptionType,
  MaxPriorityFeeTypes,
  SerializableTransactionInfo,
} from '../../../../constants/types'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetTokenSpotPricesQuery,
} from '../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s, //
} from '../../../../common/slices/constants'

// Utils
import { getLocale } from '../../../../../common/locale'
import {
  makeNetworkAsset, //
} from '../../../../options/asset-options'
import {
  getPriceIdForToken,
  getTokenPriceAmountFromRegistry,
} from '../../../../utils/pricing-utils'
import {
  parseTransactionFeesWithoutPrices, //
} from '../../../../utils/tx-utils'
import Amount from '../../../../utils/amount'

// Styled Components
import { Row } from '../../../shared/style'
import {
  StyledWrapper,
  FeeOptionButton,
  NameText,
  TimeText,
  FeeAmountText,
  FiatAmountText,
  RadioIcon,
} from './suggested_max_priority_fee_selector.styles'

const MAX_PRIORITY_FEE_LABEL_MAP = {
  slow: 'braveSwapSlow',
  average: 'braveSwapAverage',
  fast: 'braveSwapFast',
}

interface Props {
  transactionInfo: SerializableTransactionInfo
  selectedNetwork: BraveWallet.NetworkInfo
  baseFeePerGas: string
  suggestedMaxPriorityFee: MaxPriorityFeeTypes
  suggestedMaxPriorityFeeOptions: MaxPriorityFeeOptionType[]
  setSuggestedMaxPriorityFee: (value: MaxPriorityFeeTypes) => void
  onClickCustom: () => void
}

export function SuggestedMaxPriorityFeeSelector(props: Props) {
  const {
    transactionInfo,
    selectedNetwork,
    baseFeePerGas,
    suggestedMaxPriorityFee,
    suggestedMaxPriorityFeeOptions,
    setSuggestedMaxPriorityFee,
    onClickCustom,
  } = props

  // State
  const [userSelectedMaxPriorityFee, setUserSelectedMaxPriorityFee] =
    React.useState<MaxPriorityFeeTypes>(suggestedMaxPriorityFee)

  // queries
  const networkAsset = React.useMemo(() => {
    return makeNetworkAsset(selectedNetwork)
  }, [selectedNetwork])

  const networkTokenPriceIds = React.useMemo(
    () => (networkAsset ? [getPriceIdForToken(networkAsset)] : []),
    [networkAsset],
  )

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    networkTokenPriceIds.length && defaultFiatCurrency
      ? { ids: networkTokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s,
  )

  // Methods
  const onUpdateMaxPriorityFee = React.useCallback(() => {
    setSuggestedMaxPriorityFee(userSelectedMaxPriorityFee)
  }, [setSuggestedMaxPriorityFee, userSelectedMaxPriorityFee])

  // Memos
  const transactionFees = React.useMemo(
    () => parseTransactionFeesWithoutPrices(transactionInfo),
    [transactionInfo],
  )

  // render
  return (
    <StyledWrapper
      width='100%'
      height='100%'
      justifyContent='space-between'
      padding='16px'
      gap='16px'
    >
      <Row gap='12px'>
        {suggestedMaxPriorityFeeOptions.map((option) => {
          const isSelected = option.id === userSelectedMaxPriorityFee

          const gasFee = new Amount(baseFeePerGas)
            .plus(option.fee)
            .times(transactionFees.gasLimit) // Wei-per-gas → Wei conversion
            .divideByDecimals(selectedNetwork.decimals) // Wei → ETH conversion
            .format(4)

          const gasFeeFiat =
            gasFee
            && spotPriceRegistry
            && new Amount(gasFee)
              .times(
                getTokenPriceAmountFromRegistry(
                  spotPriceRegistry,
                  networkAsset,
                ),
              )
              .formatAsFiat()

          return (
            <FeeOptionButton
              key={option.id}
              isSelected={isSelected}
              onClick={() => setUserSelectedMaxPriorityFee(option.id)}
            >
              <Row justifyContent='space-between'>
                <NameText>
                  {getLocale(MAX_PRIORITY_FEE_LABEL_MAP[option.id])}
                </NameText>
                <RadioIcon
                  isSelected={isSelected}
                  name={isSelected ? 'radio-checked' : 'radio-unchecked'}
                />
              </Row>
              <TimeText>{option.duration}</TimeText>
              <FeeAmountText textAlign='left'>
                {gasFee} {selectedNetwork.symbol}
              </FeeAmountText>
              <FiatAmountText>~${gasFeeFiat}</FiatAmountText>
            </FeeOptionButton>
          )
        })}
      </Row>
      <Button
        kind='plain-faint'
        size='tiny'
        onClick={onClickCustom}
      >
        {getLocale('braveWalletCustom')}
      </Button>
      <Row>
        <Button onClick={onUpdateMaxPriorityFee}>
          {getLocale('braveWalletUpdate')}
        </Button>
      </Row>
    </StyledWrapper>
  )
}

export default SuggestedMaxPriorityFeeSelector
