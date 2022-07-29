/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import * as React from 'react'

// Utils
import { BraveWallet, BuyOption } from '../../../constants/types'

// Components
import { BuyOptionItem } from '../../buy-option/buy-option-item'
import { BackButton } from '../../shared'

// Styled Components
import {
  StyledWrapper,
  SubDivider
} from './select-buy-option-styles'

interface Props {
  buyOptions: BuyOption[]
  selectedOption: BraveWallet.OnRampProvider | undefined
  onSelect: (optionId: BraveWallet.OnRampProvider) => void
  onBack: () => void
}

export const SelectBuyOption = ({ buyOptions, selectedOption, onSelect, onBack }: Props) => {
  return (
    <StyledWrapper>
      <BackButton onSubmit={onBack} />
      {buyOptions.map((option, index) => (
        <div
          key={`${option.name}-${option.id}`}
        >
          <BuyOptionItem
            option={option}
            selectedOption={selectedOption}
            onSelect={onSelect}
          />
          {index !== buyOptions.length - 1 && <SubDivider/>}
        </div>
      ))}

    </StyledWrapper>
  )
}
