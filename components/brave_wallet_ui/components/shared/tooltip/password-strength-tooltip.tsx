// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import Tooltip from '.'
import { GreenCheckmark, PasswordStrengthText, PasswordStrengthTextWrapper, PasswordStrengthHeading } from './password-strength-tooltip.style'

// types
import { PasswordStrengthResults } from '../../../common/hooks/use-password-strength'
import { getLocale } from '$web-common/locale'

interface Props {
  isVisible: boolean
  passwordStrength: PasswordStrengthResults
}

const PasswordStrengthDetails = ({
  passwordStrength: {
    containsNumber,
    containsSpecialChar,
    isLongEnough
  }
}: {
  passwordStrength: PasswordStrengthResults
}) => {
  return (
    <PasswordStrengthTextWrapper>
      <PasswordStrengthHeading>
        {getLocale('braveWalletPasswordStrengthTooltipHeading')}
      </PasswordStrengthHeading>

      <PasswordStrengthText isStrong={isLongEnough}>
        {isLongEnough && <GreenCheckmark />}{' '}
        {getLocale('braveWalletPasswordStrengthTooltipIsLongEnough')}
      </PasswordStrengthText>

      <PasswordStrengthText isStrong={containsNumber}>
        {containsNumber && <GreenCheckmark />}{' '}
        {getLocale('braveWalletPasswordStrengthTooltipContainsNumber')}
      </PasswordStrengthText>

      <PasswordStrengthText isStrong={containsSpecialChar}>
        {containsSpecialChar && <GreenCheckmark />}{' '}
        {getLocale('braveWalletPasswordStrengthTooltipContainsSpecialChar')}
      </PasswordStrengthText>

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
      position='left'
      verticalPosition='above'
      isVisible={isVisible}
      pointerPosition={'center'}
      horizontalMarginPx={10}
      text={<PasswordStrengthDetails passwordStrength={passwordStrength} />}
    >
      {children}
    </Tooltip>
  )
}

export default PasswordStrengthTooltip
