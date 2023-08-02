// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import {
  getLocale
} from '../../../../../../../common/locale'

// Types
import {
  CoinType,
  GasEstimate,
  GasFeeOption
} from '../../../../../../constants/types'

import {
  useGetSelectedChainQuery
} from '../../../../../../common/slices/api.slice'

// Options
import {
  gasFeeOptions
} from '../../../../../../options/gas-fee-options'

// Components
import {
  ExpandSection
} from './expand-section'
import {
  GasPresetButton
} from './gas-preset-button'
import {
  StandardButton
} from '../../buttons/standard-button/standard-button'
import {
  SlippageInput
} from '../../inputs/slippage-input/slippage-input'

// Styled Components
import { Modal } from './settings.style'
import {
  Column,
  Row,
  Text,
  VerticalDivider,
  IconButton,
  VerticalSpacer,
  HiddenResponsiveRow,
  ShownResponsiveRow,
  Icon
} from '../../shared-swap.styles'

const slippagePresets = ['0.1', '0.5', '1.0']

interface Props {
  useDirectRoute: boolean
  slippageTolerance: string
  selectedGasFeeOption: GasFeeOption
  gasEstimates: GasEstimate
  setUseDirectRoute: (value: boolean) => void
  setSlippageTolerance: (value: string) => void
  setSelectedGasFeeOption: (value: GasFeeOption) => void
  onClose: () => void
}

export const SwapSettingsModal = (props: Props) => {
  const {
    selectedGasFeeOption,
    setSelectedGasFeeOption,
    setSlippageTolerance,
    // setUseDirectRoute,
    slippageTolerance,
    // useDirectRoute,
    gasEstimates,
    onClose
  } = props

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // State
  const [showExchanges, setShowExchanges] = React.useState<boolean>(false)

  // Methods
  // const handleCheckExchange = React.useCallback(
  //   (id: string, checked: boolean) => {
  //     const exchange = exchanges.find(e => e.id === id)
  //     if (checked && exchange !== undefined) {
  //       const addedList = [exchange, ...userSelectedExchanges]
  //       dispatch({ type: 'updateUserSelectedExchanges', payload: addedList })
  //       return
  //     }
  //     const removedList = userSelectedExchanges.filter(e => e.id !== id)
  //     dispatch({ type: 'updateUserSelectedExchanges', payload: removedList })
  //   },
  //   [userSelectedExchanges, exchanges, dispatch]
  // )

  // Memos
  const customSlippageInputValue: string = React.useMemo(() => {
    return slippagePresets.includes(slippageTolerance) ? '' : slippageTolerance
  }, [slippageTolerance])

  const modalTitle: string = React.useMemo(() => {
    return showExchanges
      ? getLocale('braveSwapExchanges')
      : getLocale('braveSwapSettings')
  }, [getLocale, showExchanges])

  // render
  return (
    <Modal>
      {/* Modal Header */}
      <Row rowWidth='full' marginBottom={2}>
        <Text textColor='text01' textSize='16px' isBold={true}>
          {modalTitle}
        </Text>
        {showExchanges && (
          <IconButton onClick={() => setShowExchanges(false)}>
            <Icon name='close' size={26} />
          </IconButton>
        )}
        {!showExchanges &&
          <ShownResponsiveRow maxWidth={570}>
            <IconButton onClick={onClose}>
              <Icon name='close' size={24} />
            </IconButton>
          </ShownResponsiveRow>
        }
      </Row>

      <ShownResponsiveRow maxWidth={570}>
        <VerticalSpacer size={18} />
      </ShownResponsiveRow>

      {/* {showExchanges && (
        <>
          <VerticalSpacer size={24} />
          <ExchangesColumn>
            {exchanges.map(exchange => (
              <StandardCheckbox
                id={exchange.id}
                isChecked={userSelectedExchanges
                  .some(e => e.id === exchange.id)}
                label={exchange.name}
                key={exchange.id}
                onChange={handleCheckExchange}
                labelSize='14px'
                isBold={true}
              />
            ))}
          </ExchangesColumn>
        </>
      )} */}

      {/* Settings */}
      {!showExchanges && (
        <>
          {/* Slippage Tolerance */}
          <ExpandSection
            label={getLocale('braveSwapSlippageTolerance')}
            value={`${slippageTolerance}%`}
          >
            <Row marginBottom={22} rowWidth='full'>
              <Row horizontalAlign='flex-start'>
                {slippagePresets.map(preset => (
                  <StandardButton
                    onClick={() => setSlippageTolerance(preset)}
                    buttonType='secondary'
                    buttonSize='small'
                    buttonWidth={64}
                    isSelected={slippageTolerance === preset}
                    marginRight={8}
                    key={preset}
                  >
                    {preset}%
                  </StandardButton>
                ))}
              </Row>
              <SlippageInput
                onChange={setSlippageTolerance}
                value={customSlippageInputValue}
              />
            </Row>
          </ExpandSection>

          <HiddenResponsiveRow maxWidth={570}>
            <VerticalDivider />
          </HiddenResponsiveRow>

          {/* Ethereum Only Settings */}
          {selectedNetwork?.coin === CoinType.ETH && (
            <>
              {/* Exchanges disabled until supported */}
              {/* <ExpandSection
                label={getLocale('braveSwapExchanges')}
                value={userSelectedExchanges.length.toString()}
                onExpandOut={() => setShowExchanges(true)}
              />
              <VerticalDivider /> */}

              {/* Network Fee */}
              <ExpandSection
                label={getLocale('braveSwapNetworkFee')}
                value={`$${gasEstimates.gasFeeFiat}`}
                secondaryValue={
                  `${gasEstimates.gasFee} ${selectedNetwork?.symbol ?? ''}`
                }
              >
                <Column columnWidth='full'>
                  {gasFeeOptions.map((option) => (
                    <GasPresetButton
                      option={option}
                      isSelected={selectedGasFeeOption === option}
                      onClick={() => setSelectedGasFeeOption(option)}
                      gasEstimates={gasEstimates}
                      key={option.id}
                    />
                  ))}
                </Column>
              </ExpandSection>
            </>
          )}

          {/* Solana Only Settings */}
          {selectedNetwork?.coin === CoinType.SOL && (
            <>
              {/* Direct Route Toggle is disabled until supported */}
              {/* <ToggleSection
                label={getLocale('braveSwapDirectRouteTitle')}
                description={getLocale('braveSwapDirectRouteDescription')}
                isChecked={useDirectRoute}
                setIsChecked={setUseDirectRoute}
              />
              <VerticalDivider /> */}
            </>
          )}
        </>
      )}
    </Modal>
  )
}
