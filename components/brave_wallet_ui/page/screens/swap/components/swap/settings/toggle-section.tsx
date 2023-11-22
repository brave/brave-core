// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  StandardSwitch //
} from '../../form-controls/standard-switch/standard-switch'

// Styled Components
import { Row, Column, Text, VerticalSpacer } from '../../shared-swap.styles'

interface Props {
  label: string
  description: string
  isChecked: boolean
  setIsChecked: (value: boolean) => void
}

export const ToggleSection = (props: Props) => {
  const { label, description, isChecked, setIsChecked } = props

  // render
  return (
    <Row
      rowWidth='full'
      verticalAlign='flex-start'
      verticalPadding={16}
    >
      <Column horizontalAlign='flex-start'>
        <Text
          textColor='text02'
          textSize='14px'
          textAlign='left'
          isBold={true}
        >
          {label}
        </Text>
        <VerticalSpacer size={4} />
        <Text
          textColor='text03'
          textSize='12px'
          textAlign='left'
          isBold={false}
        >
          {description}
        </Text>
      </Column>
      <StandardSwitch
        isChecked={isChecked}
        onSetIsChecked={setIsChecked}
      />
    </Row>
  )
}
