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

export type NftDropdownOptionId = 'collected' | 'hidden'

export interface NftDropdownOption {
  id: NftDropdownOptionId
  label: string
  labelSummary: string | number
}

interface Props {
  selectedOptionId: NftDropdownOptionId
  options: NftDropdownOption[]
  onSelect: (optionId: NftDropdownOption) => void
}

export const NftDropdown = ({ selectedOptionId, options, onSelect }: Props) => {
  const [isOpen, setIsOpen] = React.useState<boolean>(false)
  const selectedOption =
    options.find((option) => option.id === selectedOptionId) ?? options[0]

  const onSelectOption = (option: NftDropdownOption) => {
    setIsOpen(false)
    if (selectedOption.id === option.id) return
    onSelect(option)
  }

  return (
    <DropdownContainer>
      <DropdownButton onClick={() => setIsOpen((open) => !open)}>
        <DropdownButtonText>{selectedOption.label}</DropdownButtonText>
        {selectedOption.labelSummary !== 0 ? (
          <DropdownButtonLabel>
            {selectedOption.labelSummary}
          </DropdownButtonLabel>
        ) : null}
        <DropDownIcon
          name='carat-down'
          isOpen={isOpen}
        />
      </DropdownButton>
      <DropDown isOpen={isOpen}>
        {options.map((option) => (
          <DropDownItem
            key={option.id}
            onClick={() => onSelectOption(option)}
          >
            {option.label}
          </DropDownItem>
        ))}
      </DropDown>
    </DropdownContainer>
  )
}
