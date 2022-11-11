// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Assets
import InfoIcon from '../../../../../assets/svg-icons/info-icon.svg'

// Styled Components
import { Button, ErrorIcon, StandardButtonStyleProps } from './standard-button.style'

interface Props extends StandardButtonStyleProps {
  buttonText: string
  onClick: () => void
}

export const StandardButton = (props: Props) => {
  const {
    buttonText,
    buttonType,
    buttonWidth,
    disabled,
    marginRight,
    hasError,
    onClick
  } = props

  return (
    <Button
      buttonType={buttonType}
      buttonWidth={buttonWidth}
      disabled={disabled}
      onClick={onClick}
      marginRight={marginRight}
      hasError={hasError}
    >
      {hasError &&
        <ErrorIcon icon={InfoIcon} size={22} />
      }
      {buttonText}
    </Button>
  )
}

export default StandardButton
