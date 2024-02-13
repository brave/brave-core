// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Tooltip from '@brave/leo/react/tooltip'

// Queries
import {
  useGetDefaultFiatCurrencyQuery //
} from '../../../../common/slices/api.slice'

// Utils
import { getLocale } from '../../../../../common/locale'
import Amount from '../../../../utils/amount'

// Types
import {
  BraveWallet,
  GasEstimate,
  GasFeeOption
} from '../../../../constants/types'

// Options
import { gasFeeOptions } from '../../../../options/gas-fee-options'

// Components
import {
  PopupModal //
} from '../../../../components/desktop/popup-modals/index'

// Styled Components
import {
  SectionWrapper,
  SlippageButton,
  InputWrapper,
  CustomSlippageInput,
  PercentText,
  RadioButton,
  DarkText,
  LightText,
  ButtonRow,
  InfoIcon,
  TooltipContent
} from './advanced_settings_modal.style'
import {
  Row,
  VerticalDivider,
  Column,
  HorizontalSpace
} from '../../../../components/shared/style'

const slippagePresets = ['0.5', '1', '3']

interface Props {
  slippageTolerance: string
  selectedGasFeeOption: GasFeeOption
  gasEstimates: GasEstimate
  setSlippageTolerance: (value: string) => void
  setSelectedGasFeeOption: (value: GasFeeOption) => void
  onClose: () => void
  selectedNetwork: BraveWallet.NetworkInfo | undefined
}

export const AdvancedSettingsModal = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const {
      selectedGasFeeOption,
      setSelectedGasFeeOption,
      setSlippageTolerance,
      slippageTolerance,
      gasEstimates,
      onClose,
      selectedNetwork
    } = props

    // Queries
    const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

    // State
    const [userSelectedSlippage, setUserSelectedSlippage] =
      React.useState<string>(slippageTolerance)
    const [userSelectedGasFeeOption, setUserSelectedGasFeeOption] =
      React.useState<GasFeeOption>(selectedGasFeeOption)
    const [userCustomSlippage, setUserCustomSlippage] = React.useState<string>(
      slippagePresets.includes(slippageTolerance) ? '' : slippageTolerance
    )

    // Computed
    const slippage =
      userCustomSlippage !== '' ? userCustomSlippage : userSelectedSlippage

    // Methods
    const onSlippageInputChange = (
      event: React.ChangeEvent<HTMLInputElement>
    ) => {
      setUserCustomSlippage(event.target.value)
    }

    const onSaveChanges = React.useCallback(() => {
      setSlippageTolerance(slippage)
      setSelectedGasFeeOption(userSelectedGasFeeOption)
      onClose()
    }, [
      setSlippageTolerance,
      setSelectedGasFeeOption,
      onClose,
      userSelectedGasFeeOption,
      slippage
    ])

    // render
    return (
      <PopupModal
        onClose={onClose}
        title={getLocale('braveWalletAdvancedTransactionSettings')}
        width='500px'
        borderRadius={16}
        ref={forwardedRef}
      >
        <Column
          fullHeight={true}
          fullWidth={true}
          justifyContent='space-between'
        >
          <Column fullWidth={true}>
            {/* Slippage Tolerance */}
            <SectionWrapper fullWidth={true}>
              <Row
                marginBottom={16}
                justifyContent='flex-start'
              >
                <DarkText
                  textSize='14px'
                  isBold={true}
                >
                  {getLocale('braveWalletSlippageToleranceTitle')}
                </DarkText>
                <HorizontalSpace space='4px' />
                <Tooltip
                  mode='default'
                  placement='bottom'
                >
                  <TooltipContent slot='content'>
                    {getLocale('braveWalletSlippageToleranceExplanation')}
                  </TooltipContent>
                  <InfoIcon />
                </Tooltip>
              </Row>
              <Row
                width='100%'
                justifyContent='space-between'
              >
                <Row width='unset'>
                  {slippagePresets.map((preset) => (
                    <div key={preset}>
                      <SlippageButton
                        onClick={() => setUserSelectedSlippage(preset)}
                        kind='outline'
                        size='medium'
                        isSelected={slippage === preset}
                      >
                        {preset}%
                      </SlippageButton>
                    </div>
                  ))}
                </Row>
                <InputWrapper>
                  <CustomSlippageInput
                    type='number'
                    value={userCustomSlippage}
                    onChange={onSlippageInputChange}
                  />
                  <PercentText>%</PercentText>
                </InputWrapper>
              </Row>
              <Row
                margin='8px 0px 0px 0px'
                justifyContent='flex-start'
              >
                <LightText textSize='14px'>
                  {getLocale('braveWalletSlippageToleranceDescription')}
                </LightText>
              </Row>
            </SectionWrapper>

            {/* Network Fee - Ethereum Only */}
            {selectedNetwork?.coin === BraveWallet.CoinType.ETH && (
              <>
                <VerticalDivider />
                <SectionWrapper fullWidth={true}>
                  <Row
                    marginBottom={16}
                    justifyContent='flex-start'
                  >
                    <DarkText
                      textSize='14px'
                      isBold={true}
                    >
                      {getLocale('braveSwapNetworkFee')}
                    </DarkText>
                  </Row>
                  {gasFeeOptions.map((option) => (
                    <RadioButton
                      onChange={() => setUserSelectedGasFeeOption(option)}
                      name={option.name}
                      value={option.id}
                      currentValue={userSelectedGasFeeOption.id}
                      key={option.id}
                    >
                      <Row justifyContent='space-between'>
                        <Row width='unset'>
                          <DarkText textSize='14px'>
                            {getLocale(option.name)}
                          </DarkText>
                          <HorizontalSpace space='4px' />
                          <LightText textSize='14px'>
                            {'<'} {gasEstimates.time}
                          </LightText>
                        </Row>
                        <Column alignItems='flex-end'>
                          <DarkText textSize='14px'>
                            {gasEstimates.gasFeeGwei}{' '}
                            {getLocale('braveSwapGwei')}
                          </DarkText>
                          <LightText textSize='14px'>
                            {new Amount(
                              gasEstimates?.gasFeeFiat ?? ''
                            ).formatAsFiat(defaultFiatCurrency)}{' '}
                            {defaultFiatCurrency?.toUpperCase()}
                          </LightText>
                        </Column>
                      </Row>
                    </RadioButton>
                  ))}
                </SectionWrapper>
              </>
            )}
          </Column>
          <ButtonRow>
            <Button
              onClick={onSaveChanges}
              size='medium'
            >
              {getLocale('braveWalletButtonSaveChanges')}
            </Button>
          </ButtonRow>
        </Column>
      </PopupModal>
    )
  }
)
