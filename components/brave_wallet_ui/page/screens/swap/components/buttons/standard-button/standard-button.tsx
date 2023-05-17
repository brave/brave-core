// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Button } from './standard-button.style'

export interface StandardButtonStyleProps {
  isSelected?: boolean
  buttonSize?: 'normal' | 'small'
  buttonStyle?: 'round' | 'square'
  buttonType?: 'primary' | 'secondary'
  buttonWidth?: 'dynamic' | 'full' | number
  disabled?: boolean
  horizontalMargin?: number
  verticalMargin?: number
  marginRight?: number
}

interface Props extends StandardButtonStyleProps {
  buttonText: string
  onClick: () => void
}

export const StandardButton = (props: Props) => {
  const {
    isSelected,
    buttonSize,
    buttonStyle,
    buttonText,
    buttonType,
    buttonWidth,
    disabled,
    horizontalMargin,
    marginRight,
    onClick,
    verticalMargin
  } = props

  return (
    <Button
      buttonStyle={buttonStyle}
      buttonType={buttonType}
      buttonWidth={buttonWidth}
      disabled={disabled}
      horizontalMargin={horizontalMargin}
      onClick={onClick}
      verticalMargin={verticalMargin}
      buttonSize={buttonSize}
      isSelected={isSelected}
      marginRight={marginRight}
    >
      {buttonText}
    </Button>
  )
}
