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
  SerializableTransactionInfo,
} from '../../../../constants/types'
import {
  UpdateUnapprovedTransactionGasFieldsType, //
} from '../../../../common/constants/action_types'

// Queries
import {
  querySubscriptionOptions60s, //
} from '../../../../common/slices/constants'
import {
  useGetDefaultFiatCurrencyQuery,
  useGetTokenSpotPricesQuery,
} from '../../../../common/slices/api.slice'

// Utils
import { getLocale } from '../../../../../common/locale'
import {
  parseTransactionFeesWithoutPrices, //
} from '../../../../utils/tx-utils'
import { makeNetworkAsset } from '../../../../options/asset-options'
import {
  getPriceIdForToken,
  getTokenPriceAmountFromRegistry,
} from '../../../../utils/pricing-utils'
import Amount from '../../../../utils/amount'

// Styled Components
import { Column, Row, VerticalDivider } from '../../../shared/style'
import {
  StyledWrapper,
  Card,
  SectionLabel,
  Input,
  InputLabel,
  InputWrapper,
  Description,
} from './custom_network_fee.styles'

interface Props {
  transactionInfo: SerializableTransactionInfo
  selectedNetwork: BraveWallet.NetworkInfo
  baseFeePerGas: string
  onUpdateCustomNetworkFee: (
    payload: UpdateUnapprovedTransactionGasFieldsType,
  ) => void
  onBack: () => void
  onClose: () => void
}

export function CustomNetworkFee(props: Props) {
  const {
    transactionInfo,
    baseFeePerGas,
    selectedNetwork,
    onUpdateCustomNetworkFee,
    onBack,
    onClose,
  } = props

  // Memos
  const transactionFees = React.useMemo(
    () => parseTransactionFeesWithoutPrices(transactionInfo),
    [transactionInfo],
  )
  const { isEIP1559Transaction } = transactionFees

  const networkAsset = React.useMemo(() => {
    return makeNetworkAsset(selectedNetwork)
  }, [selectedNetwork])

  const networkTokenPriceIds = React.useMemo(
    () => (networkAsset ? [getPriceIdForToken(networkAsset)] : []),
    [networkAsset],
  )

  // Queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    networkTokenPriceIds.length && defaultFiatCurrency
      ? { ids: networkTokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s,
  )

  // State
  const [gasLimit, setGasLimit] = React.useState<string>(
    transactionFees.gasLimit,
  )
  const [gasPrice, setGasPrice] = React.useState<string>(
    new Amount(transactionFees.gasPrice)
      .divideByDecimals(9) // Wei-per-gas → GWei-per-gas conversion
      .format(),
  )
  const [maxPriorityFeePerGas, setMaxPriorityFeePerGas] =
    React.useState<string>(
      new Amount(transactionFees.maxPriorityFeePerGas)
        .divideByDecimals(9) // Wei-per-gas → GWei-per-gas conversion
        .format(),
    )
  const [maxFeePerGas, setMaxFeePerGas] = React.useState<string>(
    new Amount(transactionFees.maxFeePerGas)
      .divideByDecimals(9) // Wei-per-gas → GWei-per-gas conversion
      .format(),
  )

  // Methods
  const handleGasLimitInputChanged = (
    event: React.ChangeEvent<HTMLInputElement>,
  ) => {
    setGasLimit(event.target.value)
  }

  const handleMaxPriorityFeePerGasInputChanged = (
    event: React.ChangeEvent<HTMLInputElement>,
  ) => {
    const value = event.target.value
    setMaxPriorityFeePerGas(value)

    // GWei-per-gas → Wei-per-gas conversion
    const maxPriorityFeePerGasWei = new Amount(value).multiplyByDecimals(9)

    const computedMaxFeePerGasWei = new Amount(baseFeePerGas).plus(
      maxPriorityFeePerGasWei,
    )

    const computedMaxFeePerGasGWei = computedMaxFeePerGasWei
      .divideByDecimals(9) // Wei-per-gas → GWei-per-gas conversion
      .format()

    setMaxFeePerGas(computedMaxFeePerGasGWei)
  }

  const handleMaxFeePerGasInputChanged = (
    event: React.ChangeEvent<HTMLInputElement>,
  ) => {
    setMaxFeePerGas(event.target.value)
  }

  const handleGasPriceInputChanged = (
    event: React.ChangeEvent<HTMLInputElement>,
  ) => {
    setGasPrice(event.target.value)
  }

  const onClickUpdate = React.useCallback(() => {
    if (!isEIP1559Transaction) {
      onUpdateCustomNetworkFee({
        chainId: transactionInfo.chainId,
        txMetaId: transactionInfo.id,
        gasPrice: new Amount(gasPrice).multiplyByDecimals(9).toHex(),
        gasLimit: new Amount(gasLimit).toHex(),
      })

      onClose()
      return
    }

    onUpdateCustomNetworkFee({
      chainId: transactionInfo.chainId,
      txMetaId: transactionInfo.id,
      maxPriorityFeePerGas: new Amount(maxPriorityFeePerGas)
        .multiplyByDecimals(9)
        .toHex(),
      maxFeePerGas: new Amount(maxFeePerGas).multiplyByDecimals(9).toHex(),
      gasLimit: new Amount(gasLimit).toHex(),
    })

    onClose()
  }, [
    gasPrice,
    gasLimit,
    maxPriorityFeePerGas,
    maxFeePerGas,
    transactionInfo,
    onClose,
    onUpdateCustomNetworkFee,
    isEIP1559Transaction,
  ])

  // Computed / Memos
  const isCustomGasBelowBaseFee =
    isEIP1559Transaction
    && new Amount(maxFeePerGas).multiplyByDecimals(9).lt(baseFeePerGas)

  const customEIP1559GasFee = new Amount(maxFeePerGas)
    .multiplyByDecimals(9) // GWei-per-gas → Wei-per-gas conversion
    .times(gasLimit) // Wei-per-gas → Wei
    .divideByDecimals(selectedNetwork.decimals) // Wei → ETH conversion
    .format(6)

  const customEIP1559FiatGasFee =
    customEIP1559GasFee
    && spotPriceRegistry
    && new Amount(customEIP1559GasFee)
      .times(getTokenPriceAmountFromRegistry(spotPriceRegistry, networkAsset))
      .formatAsFiat(defaultFiatCurrency)

  const isUpdateButtonDisabled = React.useMemo(() => {
    if (gasLimit === '') {
      return true
    }

    if (new Amount(gasLimit).lte(0)) {
      return true
    }

    if (!isEIP1559Transaction && gasPrice === '') {
      return true
    }

    if (
      !isEIP1559Transaction
      && new Amount(gasPrice).multiplyByDecimals(9).isNegative()
    ) {
      return true
    }

    if (isEIP1559Transaction && maxFeePerGas === '') {
      return true
    }

    return (
      isEIP1559Transaction
      && new Amount(maxPriorityFeePerGas).multiplyByDecimals(9).isNegative()
    )
  }, [
    gasLimit,
    isEIP1559Transaction,
    gasPrice,
    maxFeePerGas,
    maxPriorityFeePerGas,
  ])
  const isZeroGasPrice = React.useMemo(() => {
    return (
      !isEIP1559Transaction
      && gasPrice !== ''
      && new Amount(gasPrice).multiplyByDecimals(9).isZero()
    )
  }, [gasPrice, isEIP1559Transaction])

  // Effects
  React.useEffect(() => {
    const maxPriorityFeePerGasWei = new Amount(
      maxPriorityFeePerGas,
    ).multiplyByDecimals(9) // GWei-per-gas → Wei conversion

    const maxFeePerGasWeiValue = new Amount(baseFeePerGas).plus(
      maxPriorityFeePerGasWei,
    )

    setMaxFeePerGas(
      maxFeePerGasWeiValue
        .divideByDecimals(9) // Wei-per-gas → GWei-per-gas conversion
        .format(),
    )
  }, [maxPriorityFeePerGas, baseFeePerGas])

  // render
  return (
    <StyledWrapper
      width='100%'
      height='100%'
      justifyContent='space-between'
      padding='16px'
      gap='16px'
    >
      <Description
        textColor='tertiary'
        textAlign='left'
      >
        {getLocale('braveWalletEditGasDescription')}
      </Description>
      <Card
        width='100%'
        padding={isEIP1559Transaction ? '16px 16px 8px 16px' : '16px'}
        gap='8px'
      >
        {isEIP1559Transaction && (
          <>
            <Row
              justifyContent='space-between'
              alignItems='flex-start'
              padding='8px 0px'
            >
              <SectionLabel textColor='secondary'>
                {getLocale('braveWalletEditGasBaseFee')}
              </SectionLabel>
              <SectionLabel textColor='primary'>
                {new Amount(baseFeePerGas).divideByDecimals(9).format()}{' '}
                {getLocale('braveWalletEditGasGwei')}
              </SectionLabel>
            </Row>
            <VerticalDivider />
          </>
        )}
        <Row justifyContent='space-between'>
          <SectionLabel textColor='secondary'>
            {getLocale('braveWalletEditGasLimit')}
          </SectionLabel>
          <InputWrapper
            width='140px'
            padding='8px 12px'
            hasError={gasLimit === '0'}
          >
            <Input
              placeholder='0'
              type='number'
              min={0}
              value={gasLimit}
              onChange={handleGasLimitInputChanged}
              data-testid='gas-limit-input'
            />
          </InputWrapper>
        </Row>

        {gasLimit === '0' && (
          <Row justifyContent='flex-start'>
            <InputLabel
              textColor='error'
              textAlign='left'
            >
              {getLocale('braveWalletEditGasLimitError')}
            </InputLabel>
          </Row>
        )}
        <VerticalDivider />
        {!isEIP1559Transaction && (
          <>
            <Row justifyContent='space-between'>
              <SectionLabel textColor='secondary'>
                {getLocale('braveWalletGasPrice')}
              </SectionLabel>
              <InputWrapper
                width='140px'
                gap='6px'
                padding='8px 12px'
                hasError={isZeroGasPrice}
              >
                <Input
                  placeholder='0'
                  type='number'
                  value={gasPrice}
                  onChange={handleGasPriceInputChanged}
                  data-testid='gas-price-input'
                />
                <InputLabel textColor='tertiary'>
                  {getLocale('braveWalletEditGasGwei')}
                </InputLabel>
              </InputWrapper>
            </Row>
            {isZeroGasPrice && (
              <Row justifyContent='flex-start'>
                <InputLabel
                  textColor='error'
                  textAlign='left'
                >
                  {getLocale('braveWalletEditGasZeroGasPriceWarning')}
                </InputLabel>
              </Row>
            )}
          </>
        )}
        {isEIP1559Transaction && (
          <>
            <Row justifyContent='space-between'>
              <SectionLabel textColor='secondary'>
                {getLocale('braveWalletGasTipLimit')}
              </SectionLabel>
              <InputWrapper
                width='140px'
                gap='6px'
                padding='8px 12px'
                hasError={isCustomGasBelowBaseFee}
              >
                <Input
                  placeholder='0'
                  type='number'
                  min={0}
                  value={maxPriorityFeePerGas}
                  onChange={handleMaxPriorityFeePerGasInputChanged}
                />
                <InputLabel textColor='tertiary'>
                  {getLocale('braveWalletEditGasGwei')}
                </InputLabel>
              </InputWrapper>
            </Row>
            <VerticalDivider />
            <Row justifyContent='space-between'>
              <SectionLabel textColor='secondary'>
                {getLocale('braveWalletGasPriceLimit')}
              </SectionLabel>
              <InputWrapper
                width='140px'
                gap='6px'
                padding='8px 12px'
                hasError={isCustomGasBelowBaseFee}
              >
                <Input
                  placeholder='0'
                  min={0}
                  type='number'
                  value={maxFeePerGas}
                  onChange={handleMaxFeePerGasInputChanged}
                  data-testid='gas-price-limit-input'
                />
                <InputLabel textColor='tertiary'>
                  {getLocale('braveWalletEditGasGwei')}
                </InputLabel>
              </InputWrapper>
            </Row>
            {isCustomGasBelowBaseFee && (
              <Row justifyContent='flex-start'>
                <InputLabel
                  textColor='error'
                  textAlign='left'
                >
                  {getLocale('braveWalletGasFeeLimitLowerThanBaseFeeWarning')}
                </InputLabel>
              </Row>
            )}
            <VerticalDivider />
            <Row
              justifyContent='space-between'
              alignItems='flex-start'
              padding='8px 0px'
            >
              <SectionLabel textColor='secondary'>
                {getLocale('braveWalletEditGasEstimatedNetworkFee')}
              </SectionLabel>
              <Column alignItems='flex-end'>
                <SectionLabel textColor='primary'>
                  ~{customEIP1559FiatGasFee}
                </SectionLabel>
                <SectionLabel textColor='primary'>
                  ~
                  {new Amount(customEIP1559GasFee).formatAsAsset(
                    6,
                    networkAsset.symbol,
                  )}
                </SectionLabel>
              </Column>
            </Row>
            <VerticalDivider />
            <Row width='unset'>
              <Button
                kind='plain-faint'
                size='tiny'
                onClick={onBack}
              >
                {getLocale('braveWalletUseDefault')}
              </Button>
            </Row>
          </>
        )}
      </Card>
      <Row>
        <Button
          isDisabled={isUpdateButtonDisabled}
          onClick={onClickUpdate}
        >
          {getLocale('braveWalletUpdate')}
        </Button>
      </Row>
    </StyledWrapper>
  )
}

export default CustomNetworkFee
