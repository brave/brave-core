// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '$web-common/locale'

// style
import {
  Bar,
  BarAndMessageContainer,
  BarBackground,
  BarMessage,
  BarProgress,
  BarProgressTooltipContainer
} from './password-strength-bar.styles'

// types
import { PasswordStrengthResults } from '../../../common/hooks/use-password-strength'

// components
import { PasswordStrengthTooltip } from '../tooltip/password-strength-tooltip'

interface Props {
  criteria: boolean[]
  isVisible: boolean
  passwordStrength: PasswordStrengthResults
}

export const PasswordStrengthBar: React.FC<Props> = ({
  criteria,
  isVisible,
  passwordStrength
}) => {
  // memos
  const strongPasswordCrtieriaPercentComplete = React.useMemo(
    () => ((criteria.filter(c => !!c).length / criteria.length) * 100),
    [criteria]
  )

  // render
  return (
    <BarAndMessageContainer>
      <Bar>
        <BarBackground />
        <BarProgress criteria={criteria}>
          <BarProgressTooltipContainer
            criteria={criteria}
          >
            <PasswordStrengthTooltip
              passwordStrength={passwordStrength}
              isVisible={isVisible}
            />
          </BarProgressTooltipContainer>
        </BarProgress>
      </Bar>

      <BarMessage criteria={criteria}>
        {strongPasswordCrtieriaPercentComplete === 100
          ? getLocale('braveWalletPasswordIsStrong')
          : strongPasswordCrtieriaPercentComplete < 50
            ? getLocale('braveWalletPasswordIsWeak')
            : getLocale('braveWalletPasswordIsMediumStrength')
        }
      </BarMessage>
    </BarAndMessageContainer>
  )
}

export default PasswordStrengthBar
