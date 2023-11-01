// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'

// Styled Components
import {
  CheckboxText,
  Description,
  IconWrapper,
  Icon
} from './filter-components.style'
import { Row, Column } from '../../../../shared/style'

interface Props {
  title: string
  description: string
  icon: string
  isSelected: boolean
  setIsSelected: () => void
}

export const FilterToggleSection = (props: Props) => {
  const { title, description, icon, isSelected, setIsSelected } = props

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
      <Toggle
        checked={isSelected}
        onChange={setIsSelected}
        size='small'
      />
    </Row>
  )
}
