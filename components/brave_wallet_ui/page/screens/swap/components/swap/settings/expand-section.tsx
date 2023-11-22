// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { MoreButton } from './settings.style'
import {
  Row,
  Column,
  Text,
  HorizontalSpacer,
  Icon
} from '../../shared-swap.styles'

interface Props {
  label: string
  value: string
  secondaryValue?: string
  onExpandOut?: () => void
  children?: React.ReactNode
}

export const ExpandSection = (props: Props) => {
  const { label, value, secondaryValue, children, onExpandOut } = props

  // State
  const [expanded, setExpanded] = React.useState<boolean>(false)

  // Methods
  const toggleExpanded = React.useCallback(() => {
    setExpanded((prev) => !prev)
  }, [])

  // render
  return (
    <Column columnWidth='full'>
      <Row
        rowWidth='full'
        verticalPadding={16}
      >
        <Text
          textColor='text02'
          textSize='14px'
          isBold={true}
        >
          {label}
        </Text>
        <Row>
          <Text
            textColor='text01'
            textSize='14px'
            isBold={true}
          >
            {value}
          </Text>
          {secondaryValue && (
            <>
              <HorizontalSpacer size={4} />
              <Text
                textColor='text03'
                textSize='14px'
                isBold={true}
              >
                {secondaryValue}
              </Text>
            </>
          )}
          <HorizontalSpacer size={16} />
          <MoreButton
            isSelected={expanded}
            expandOut={onExpandOut !== undefined}
            onClick={onExpandOut ?? toggleExpanded}
          >
            <Icon
              name='carat-down'
              size={20}
            />
          </MoreButton>
        </Row>
      </Row>
      {expanded && children}
    </Column>
  )
}
