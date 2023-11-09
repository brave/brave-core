// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Types
import {
  GasFeeOption,
  GasEstimate,
  BraveWallet
} from '../../../../../../constants/types'

// Styled Components
import { Button, ButtonIcon, IconWrapper } from './settings.style'
import { Column, Row, Text } from '../../shared-swap.styles'

interface Props {
  isSelected: boolean
  option: GasFeeOption
  gasEstimates: GasEstimate
  onClick: () => void
  selectedNetwork: BraveWallet.NetworkInfo | undefined
}

export const GasPresetButton = (props: Props) => {
  const { onClick, isSelected, option, gasEstimates, selectedNetwork } = props

  return (
    <Button
      onClick={onClick}
      isSelected={isSelected}
    >
      <Row>
        <IconWrapper>
          <ButtonIcon
            size={20}
            name={option.icon}
          />
        </IconWrapper>
        <Column
          horizontalAlign='flex-start'
          columnHeight='full'
        >
          <Text
            textColor='text02'
            textSize='14px'
            isBold={true}
          >
            {getLocale(option.name)}
          </Text>
          <Text
            textColor='text03'
            textSize='12px'
            isBold={false}
          >
            {'<'} {gasEstimates.time}
          </Text>
        </Column>
      </Row>
      <Column
        horizontalAlign='flex-end'
        columnHeight='full'
      >
        <Text
          textColor='text02'
          textSize='14px'
          isBold={true}
        >
          {gasEstimates.gasFeeGwei} {getLocale('braveSwapGwei')}
        </Text>
        <Text
          textColor='text03'
          textSize='12px'
          isBold={false}
        >
          {gasEstimates.gasFee} {selectedNetwork?.symbol}
        </Text>
      </Column>
    </Button>
  )
}
