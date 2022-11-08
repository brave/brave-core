// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { CSSProperties } from 'styled-components'

// style
import { CheckIcon } from '../style'
import {
  StyledLabel,
  StyledBox,
  StyledText
} from './checkbox.style'

export type Size = 'big' | 'small'

export interface Props {
  id?: string
  isChecked: boolean
  testId?: string
  children: React.ReactNode
  disabled?: boolean
  size?: Size
  onChange?: (selected: boolean) => void
  alignItems?: CSSProperties['alignItems']
}

export const Checkbox: React.FC<Props> = ({
  alignItems,
  children,
  disabled = false,
  isChecked,
  onChange,
  size = 'small',
  testId
}) => {
  // methods
  const onClick = React.useCallback(() => {
    if (onChange) {
      onChange(!isChecked)
    }
  }, [onChange, !isChecked])

  const onKeyPress = React.useCallback((event: React.KeyboardEvent) => {
    // Invoke for space or enter, just like a regular input or button
    if ([' ', 'Enter'].includes(event.key)) {
      onClick()
    }
  }, [onClick])

  // render
  return (
    <StyledLabel
      data-testid={`checkbox-label-${testId}`}
      role='checkbox'
      aria-checked={isChecked ? 'true' : 'false'}
      tabIndex={disabled ? undefined : 0}
      onClick={!disabled ? onClick : undefined}
      onKeyPress={!disabled ? onKeyPress : undefined}
      size={size}
      disabled={disabled}
      selected={isChecked}
      alignItems={alignItems}
    >
      <StyledBox>
        {isChecked ? <CheckIcon /> : null}
      </StyledBox>
      <StyledText size={size}>
        {children}
      </StyledText>
    </StyledLabel>
  )
}
