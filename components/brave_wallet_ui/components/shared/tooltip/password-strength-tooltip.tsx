// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '$web-common/locale'

// components
import { Tooltip } from './index'

// style
import { GreenCheckmark } from '../style'
import {
  CriteriaCheckContainer,
  PasswordStrengthHeading,
  PasswordStrengthText,
  PasswordStrengthTextWrapper
} from './password-strength-tooltip.style'

// types
import { PasswordStrengthResults } from '../../../common/hooks/use-password-strength'

interface Props {
  isVisible: boolean
  passwordStrength: PasswordStrengthResults
}

const PasswordStrengthDetails = ({
  passwordStrength: {
    isLongEnough
  }
}: Pick<Props, 'passwordStrength'>) => {
  return (
    <PasswordStrengthTextWrapper>
      <PasswordStrengthHeading>
        {getLocale('braveWalletPasswordStrengthTooltipHeading')}
      </PasswordStrengthHeading>

      <CriteriaCheckContainer>
        {isLongEnough && <GreenCheckmark />}
        <PasswordStrengthText isStrong={isLongEnough}>
          {getLocale('braveWalletPasswordStrengthTooltipIsLongEnough')}
        </PasswordStrengthText>
      </CriteriaCheckContainer>

    </PasswordStrengthTextWrapper>
  )
}

export const PasswordStrengthTooltip: React.FC<React.PropsWithChildren<Props>> = ({
  children,
  isVisible,
  passwordStrength
}) => {
  return (
    <Tooltip
      disableHoverEvents
      verticalPosition='below'
      isVisible={isVisible}
      position='right'
      pointerPosition={'center'}
      text={
        <PasswordStrengthDetails
          passwordStrength={passwordStrength}
        />
      }
    >
      {children}
    </Tooltip>
  )
}

export default PasswordStrengthTooltip
