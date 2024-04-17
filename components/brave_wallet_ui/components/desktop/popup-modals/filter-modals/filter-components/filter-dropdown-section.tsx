// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Types
import { DropdownFilterOption } from '../../../../../constants/types'

// Styled Components
import {
  CheckboxText,
  Description,
  IconWrapper,
  Icon,
  DropdownFilter
} from './filter-components.style'
import { Row, Column } from '../../../../shared/style'

interface Props {
  title: string
  description: string
  icon?: string
  dropdownOptions: DropdownFilterOption[]
  selectedOptionId: string
  onSelectOption: (id: string) => void
}

// ToDo: We need to remove this this declaration once
// `leo-option` includes a key prop for us to use.
// https://github.com/brave/brave-browser/issues/30826
declare global {
  namespace JSX {
    interface IntrinsicElements {
      'dropdown-option': React.HTMLAttributes<HTMLElement> & {
        value?: string
        children?: any
        key?: React.Key
      }
    }
  }
}

export const FilterDropdownSection = (props: Props) => {
  const {
    title,
    description,
    icon,
    dropdownOptions,
    selectedOptionId,
    onSelectOption
  } = props

  // Memos
  const selectedDropdownName = React.useMemo(() => {
    return (
      dropdownOptions.find((option) => option.id === selectedOptionId)?.name ??
      ''
    )
  }, [dropdownOptions, selectedOptionId])

  return (
    <Row
      marginBottom={16}
      justifyContent='space-between'
    >
      <Row
        width='unset'
        justifyContent='flex-start'
      >
        <IconWrapper>
          <Icon name={icon} />
        </IconWrapper>
        <Column alignItems='flex-start'>
          <CheckboxText
            textSize='14px'
            isBold={true}
          >
            {title}
          </CheckboxText>
          <Description
            textSize='12px'
            isBold={false}
            textAlign='left'
          >
            {description}
          </Description>
        </Column>
      </Row>
      <DropdownFilter
        onChange={(e) => onSelectOption(e.value!)}
        value={selectedOptionId}
      >
        <div slot='value'>{getLocale(selectedDropdownName)}</div>
        {dropdownOptions.map((option) => (
          <leo-option
            value={option.id}
            key={option.id}
          >
            {getLocale(option.name)}
          </leo-option>
        ))}
      </DropdownFilter>
    </Row>
  )
}
