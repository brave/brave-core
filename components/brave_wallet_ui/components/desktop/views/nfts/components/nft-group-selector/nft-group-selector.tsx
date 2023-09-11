// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// styles
import {
  DropdownButton,
  DropdownButtonText,
  DropdownButtonLabel,
  DropDownIcon,
  DropDown,
  DropDownItem,
  DropdownContainer
} from './nft-group-selector.styles'

export type NftGroupId = 'collected' | 'hidden' | 'spam'

export interface DropdownOption {
  id: NftGroupId
  label: string
}

interface Props {
  selectedOption: DropdownOption
  options: DropdownOption[]
  onSelect: (optionId: NftGroupId) => void
}

export const NftGroupSelector = ({ selectedOption, options, onSelect }: Props) => {
  const [isOpen, setIsOpen] = React.useState<boolean>(false)

  return (
    <DropdownContainer>
      <DropdownButton onClick={() => setIsOpen((open) => !open)}>
        <DropdownButtonText>{selectedOption.id}</DropdownButtonText>
        <DropdownButtonLabel>8</DropdownButtonLabel>
        <DropDownIcon name='carat-down' isOpen={isOpen} />
      </DropdownButton>
      <DropDown isOpen={isOpen}>
        {options.map((option) => (
          <DropDownItem key={option.id} onClick={() => onSelect(option.id)}>
            {option.label}
          </DropDownItem>
        ))}
      </DropDown>
    </DropdownContainer>
  )
}
