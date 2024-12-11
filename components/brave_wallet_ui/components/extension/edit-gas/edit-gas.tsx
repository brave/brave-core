// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Alert from '@brave/leo/react/alert'

import { getLocale } from '../../../../common/locale'
import {
  BraveWallet,
  SerializableTransactionInfo
} from '../../../constants/types'
import { UpdateUnapprovedTransactionGasFieldsType } from '../../../common/constants/action_types'

import { NavButton } from '../buttons/nav-button/index'
import { Panel } from '../panel/index'

// Utils
import Amount from '../../../utils/amount'
import { parseTransactionFeesWithoutPrices } from '../../../utils/tx-utils'
import { makeNetworkAsset } from '../../../options/asset-options'
import {
  getTokenPriceAmountFromRegistry,
  getPriceIdForToken
} from '../../../utils/pricing-utils'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetTokenSpotPricesQuery
} from '../../../common/slices/api.slice'
import { querySubscriptionOptions60s } from '../../../common/slices/constants'

// Styled Components
import { Column, ErrorText, Row, Text } from '../../shared/style'
import {
  StyledWrapper,
  FormColumn,
  Input,
  InputLabel,
  ButtonRow,
  Description,
  CurrentBaseRow,
  CurrentBaseText,
  MaximumFeeRow,
  MaximumFeeText,
  GasSlider,
  SliderLabelRow,
  SliderLabel,
  SliderWrapper,
  SliderValue,
  WarningText
} from './edit-gas.styles'

export enum MaxPriorityPanels {
  setSuggested = 0,
  setCustom = 1
}

interface Props {
  onCancel: () => void
  transactionInfo: SerializableTransactionInfo
  selectedNetwork: BraveWallet.NetworkInfo
  baseFeePerGas: string
  suggestedMaxPriorityFeeChoices: string[]
  suggestedSliderStep: string
  maxPriorityPanel: MaxPriorityPanels
  updateUnapprovedTransactionGasFields: (
    payload: UpdateUnapprovedTransactionGasFieldsType
  ) => void
  setSuggestedSliderStep: (value: string) => void
  setMaxPriorityPanel: (value: MaxPriorityPanels) => void
}

export const EditGas = ({
  onCancel,
  selectedNetwork,
  transactionInfo,
  baseFeePerGas,
  suggestedMaxPriorityFeeChoices,
  suggestedSliderStep,
  maxPriorityPanel,
  updateUnapprovedTransactionGasFields,
  setSuggestedSliderStep,
  setMaxPriorityPanel
}: Props) => {
  const transactionFees = React.useMemo(
    () => parseTransactionFeesWithoutPrices(transactionInfo),
    [transactionInfo]
  )
  const { isEIP1559Transaction } = transactionFees

  // state
  const [suggestedMaxPriorityFee, setSuggestedMaxPriorityFee] =
    React.useState<string>(suggestedMaxPriorityFeeChoices[1])
  const [gasLimit, setGasLimit] = React.useState<string>(
    transactionFees.gasLimit
  )
  const [gasPrice, setGasPrice] = React.useState<string>(
    new Amount(transactionFees.gasPrice)
      .divideByDecimals(9) // Wei-per-gas → GWei-per-gas conversion
      .format()
  )
  const [maxPriorityFeePerGas, setMaxPriorityFeePerGas] =
    React.useState<string>(
      new Amount(transactionFees.maxPriorityFeePerGas)
        .divideByDecimals(9) // Wei-per-gas → GWei-per-gas conversion
        .format()
    )
  const [maxFeePerGas, setMaxFeePerGas] = React.useState<string>(
    new Amount(transactionFees.maxFeePerGas)
      .divideByDecimals(9) // Wei-per-gas → GWei-per-gas conversion
      .format()
  )

  // queries
  const networkAsset = React.useMemo(() => {
    return makeNetworkAsset(selectedNetwork)
  }, [selectedNetwork])

  const networkTokenPriceIds = React.useMemo(
    () => (networkAsset ? [getPriceIdForToken(networkAsset)] : []),
    [networkAsset]
  )

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    networkTokenPriceIds.length && defaultFiatCurrency
      ? { ids: networkTokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

  // methods
  const handleGasPriceInputChanged = (
    event: React.ChangeEvent<HTMLInputElement>
  ) => {
    setGasPrice(event.target.value)
  }

  const handleGasLimitInputChanged = React.useCallback(
    ({
      target: {
        value,
        validity: { valid }
      }
    }: React.ChangeEvent<HTMLInputElement>) => {
      if (valid) {
        const val = new Amount(value).toNumber().toString()
        setGasLimit(val)
      }
    },
    []
  )

  const handleMaxPriorityFeePerGasInputChanged = (
    event: React.ChangeEvent<HTMLInputElement>
  ) => {
    const value = event.target.value
    setMaxPriorityFeePerGas(value)

    // GWei-per-gas → Wei-per-gas conversion
    const maxPriorityFeePerGasWei = new Amount(value).multiplyByDecimals(9)

    const computedMaxFeePerGasWei = new Amount(baseFeePerGas).plus(
      maxPriorityFeePerGasWei
    )

    const computedMaxFeePerGasGWei = computedMaxFeePerGasWei
      .divideByDecimals(9) // Wei-per-gas → GWei-per-gas conversion
      .format()

    setMaxFeePerGas(computedMaxFeePerGasGWei)
  }

  const handleMaxFeePerGasInputChanged = (
    event: React.ChangeEvent<HTMLInputElement>
  ) => {
    setMaxFeePerGas(event.target.value)
  }

  const onSetPanelToCustom = () => {
    setMaxPriorityPanel(MaxPriorityPanels.setCustom)
  }

  const onSetPanelToSuggested = () => {
    setMaxPriorityPanel(MaxPriorityPanels.setSuggested)
  }

  const handleSliderChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const suggestedSliderStep = event.target.value
    setSuggestedSliderStep(suggestedSliderStep)
    const hexString =
      suggestedMaxPriorityFeeChoices[Number(suggestedSliderStep)]
    setSuggestedMaxPriorityFee(hexString)
    setMaxPriorityFeePerGas(new Amount(hexString).divideByDecimals(9).format())
    const computedMaxFeePerGasWei = new Amount(baseFeePerGas).plus(hexString)
    const computedMaxFeePerGasGWei = computedMaxFeePerGasWei
      .divideByDecimals(9)
      .format()
    setMaxFeePerGas(computedMaxFeePerGasGWei)
  }

  const onSave = () => {
    if (!isEIP1559Transaction) {
      updateUnapprovedTransactionGasFields({
        chainId: transactionInfo.chainId,
        txMetaId: transactionInfo.id,
        gasPrice: new Amount(gasPrice).multiplyByDecimals(9).toHex(),
        gasLimit: new Amount(gasLimit).toHex()
      })

      onCancel()
      return
    }

    if (maxPriorityPanel === MaxPriorityPanels.setCustom) {
      updateUnapprovedTransactionGasFields({
        chainId: transactionInfo.chainId,
        txMetaId: transactionInfo.id,
        maxPriorityFeePerGas: new Amount(maxPriorityFeePerGas)
          .multiplyByDecimals(9)
          .toHex(),
        maxFeePerGas: new Amount(maxFeePerGas).multiplyByDecimals(9).toHex(),
        gasLimit: new Amount(gasLimit).toHex()
      })
    } else if (maxPriorityPanel === MaxPriorityPanels.setSuggested) {
      updateUnapprovedTransactionGasFields({
        chainId: transactionInfo.chainId,
        txMetaId: transactionInfo.id,
        gasLimit: new Amount(gasLimit).toHex(),
        maxPriorityFeePerGas: suggestedMaxPriorityFee,
        maxFeePerGas: new Amount(baseFeePerGas)
          .plus(suggestedMaxPriorityFee)
          .toHex()
      })
    }

    onCancel()
  }

  // computed & memos
  const showSuggestedMaxPriorityPanel =
    isEIP1559Transaction && maxPriorityPanel === MaxPriorityPanels.setSuggested
  const showCustomMaxPriorityPanel =
    isEIP1559Transaction && maxPriorityPanel === MaxPriorityPanels.setCustom
  const suggestedEIP1559GasFee = showSuggestedMaxPriorityPanel
    ? new Amount(baseFeePerGas)
        .plus(suggestedMaxPriorityFee)
        .times(gasLimit) // Wei-per-gas → Wei conversion
        .divideByDecimals(selectedNetwork.decimals) // Wei → ETH conversion
        .format(6)
    : undefined

  const suggestedEIP1559FiatGasFee =
    suggestedEIP1559GasFee &&
    spotPriceRegistry &&
    new Amount(suggestedEIP1559GasFee)
      .times(getTokenPriceAmountFromRegistry(spotPriceRegistry, networkAsset))
      .formatAsFiat()

  const customEIP1559GasFee = showCustomMaxPriorityPanel
    ? new Amount(maxFeePerGas)
        .multiplyByDecimals(9) // GWei-per-gas → Wei-per-gas conversion
        .times(gasLimit) // Wei-per-gas → Wei
        .divideByDecimals(selectedNetwork.decimals) // Wei → ETH conversion
        .format(6)
    : undefined
  const customEIP1559FiatGasFee =
    customEIP1559GasFee &&
    spotPriceRegistry &&
    new Amount(customEIP1559GasFee)
      .times(getTokenPriceAmountFromRegistry(spotPriceRegistry, networkAsset))
      .formatAsFiat()

  const gasLimitComponent = React.useMemo(
    () => (
      <>
        <InputLabel>{getLocale('braveWalletEditGasLimit')}</InputLabel>
        <Input
          placeholder='0'
          type='text'
          pattern='[0-9]*' // allow number characters only
          value={gasLimit}
          onChange={handleGasLimitInputChanged}
          hasError={gasLimit === '0'}
        />
        {gasLimit === '0' && (
          <ErrorText>{getLocale('braveWalletEditGasLimitError')}</ErrorText>
        )}
      </>
    ),
    [gasLimit, handleGasLimitInputChanged]
  )

  const isZeroGasPrice = React.useMemo(() => {
    return (
      !isEIP1559Transaction &&
      gasPrice !== '' &&
      new Amount(gasPrice).multiplyByDecimals(9).isZero()
    )
  }, [gasPrice, isEIP1559Transaction])

  const isSaveButtonDisabled = React.useMemo(() => {
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
      !isEIP1559Transaction &&
      new Amount(gasPrice).multiplyByDecimals(9).isNegative()
    ) {
      return true
    }

    if (isEIP1559Transaction && maxFeePerGas === '') {
      return true
    }

    return (
      isEIP1559Transaction &&
      new Amount(maxPriorityFeePerGas).multiplyByDecimals(9).isNegative()
    )
  }, [
    gasLimit,
    isEIP1559Transaction,
    gasPrice,
    maxFeePerGas,
    maxPriorityFeePerGas
  ])

  const isCustomGasBelowBaseFee =
    isEIP1559Transaction &&
    new Amount(maxFeePerGas).multiplyByDecimals(9).lt(baseFeePerGas)

  // effects
  React.useEffect(() => {
    const maxPriorityFeePerGasWei = new Amount(
      maxPriorityFeePerGas
    ).multiplyByDecimals(9) // GWei-per-gas → Wei conversion

    const maxFeePerGasWeiValue = new Amount(baseFeePerGas).plus(
      maxPriorityFeePerGasWei
    )

    setMaxFeePerGas(
      maxFeePerGasWeiValue
        .divideByDecimals(9) // Wei-per-gas → GWei-per-gas conversion
        .format()
    )
  }, [maxPriorityFeePerGas, baseFeePerGas])

  // render
  return (
    <Panel
      navAction={onCancel}
      title={
        isEIP1559Transaction
          ? getLocale('braveWalletEditGasTitle1')
          : getLocale('braveWalletEditGasTitle2')
      }
    >
      <StyledWrapper>
        {isEIP1559Transaction && (
          <Description>
            {getLocale('braveWalletEditGasDescription')}
          </Description>
        )}
        {showCustomMaxPriorityPanel && (
          <FormColumn>
            <CurrentBaseRow>
              <CurrentBaseText>
                {getLocale('braveWalletEditGasBaseFee')}
              </CurrentBaseText>
              <CurrentBaseText>
                {`${new Amount(baseFeePerGas).divideByDecimals(9).format()}
                  ${getLocale('braveWalletEditGasGwei')}`}
              </CurrentBaseText>
            </CurrentBaseRow>

            {gasLimitComponent}

            <InputLabel>
              {getLocale('braveWalletEditGasPerTipLimit')}
            </InputLabel>
            <Input
              placeholder='0'
              type='number'
              min={0}
              value={maxPriorityFeePerGas}
              onChange={handleMaxPriorityFeePerGasInputChanged}
              hasError={isCustomGasBelowBaseFee}
            />

            {/* Gas-Per-Price Limit */}
            <InputLabel>
              {getLocale('braveWalletEditGasPerPriceLimit')}
            </InputLabel>
            <Input
              placeholder='0'
              min={0}
              type='number'
              value={maxFeePerGas}
              onChange={handleMaxFeePerGasInputChanged}
              hasError={isCustomGasBelowBaseFee}
            />

            <MaximumFeeRow>
              <MaximumFeeText>
                {getLocale('braveWalletEditGasMaximumFee')}
              </MaximumFeeText>
              <MaximumFeeText>
                ~${customEIP1559FiatGasFee} USD (${customEIP1559GasFee}{' '}
                {selectedNetwork.symbol})
              </MaximumFeeText>
            </MaximumFeeRow>
            {isCustomGasBelowBaseFee && (
              <Row margin={'16px 0px'}>
                <Alert
                  type='error'
                >
                  <Column
                    alignItems='center'
                    justifyContent='center'
                    fullHeight
                  >
                    <Text
                      textAlign='left'
                      textSize='14px'
                    >
                      {getLocale(
                        'braveWalletGasFeeLimitLowerThanBaseFeeWarning'
                      )}
                    </Text>
                  </Column>
                </Alert>
              </Row>
            )}
          </FormColumn>
        )}

        {showSuggestedMaxPriorityPanel && (
          <SliderWrapper>
            <SliderLabel>{getLocale('braveWalletEditGasMaxFee')}:</SliderLabel>
            <SliderValue>
              ~${suggestedEIP1559FiatGasFee} USD ({suggestedEIP1559GasFee}{' '}
              {selectedNetwork.symbol})
            </SliderValue>
            <GasSlider
              type='range'
              min='0'
              max={suggestedMaxPriorityFeeChoices.length - 1}
              value={suggestedSliderStep}
              onChange={handleSliderChange}
            />
            <SliderLabelRow>
              <SliderLabel>{getLocale('braveWalletEditGasLow')}</SliderLabel>
              <SliderLabel>
                {getLocale('braveWalletEditGasOptimal')}
              </SliderLabel>
              <SliderLabel>{getLocale('braveWalletEditGasHigh')}</SliderLabel>
            </SliderLabelRow>
          </SliderWrapper>
        )}

        {!isEIP1559Transaction && (
          <FormColumn>
            {gasLimitComponent}

            <InputLabel>
              {getLocale('braveWalletEditGasPerGasPrice')}
            </InputLabel>
            <Input
              placeholder='0'
              type='number'
              value={gasPrice}
              onChange={handleGasPriceInputChanged}
            />

            {isZeroGasPrice && (
              <WarningText>
                {getLocale('braveWalletEditGasZeroGasPriceWarning')}
              </WarningText>
            )}
          </FormColumn>
        )}

        <ButtonRow>
          <NavButton
            buttonType='secondary'
            needsTopMargin={true}
            text={
              !isEIP1559Transaction
                ? getLocale('braveWalletButtonCancel')
                : maxPriorityPanel === MaxPriorityPanels.setCustom
                ? getLocale('braveWalletEditGasSetSuggested')
                : getLocale('braveWalletEditGasSetCustom')
            }
            onSubmit={
              !isEIP1559Transaction
                ? onCancel
                : maxPriorityPanel === MaxPriorityPanels.setCustom
                ? onSetPanelToSuggested
                : onSetPanelToCustom
            }
          />

          <NavButton
            buttonType='primary'
            text={getLocale('braveWalletAccountSettingsSave')}
            onSubmit={onSave}
            disabled={isSaveButtonDisabled}
          />
        </ButtonRow>
      </StyledWrapper>
    </Panel>
  )
}

export default EditGas
