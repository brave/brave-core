// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

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

// components
import { PasswordStrengthTooltip } from '../tooltip/password-strength-tooltip'
import { PasswordStrengthResults } from '../../../common/hooks/use-password-strength'

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
  const percentComplete = React.useMemo(
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
              criteria={criteria}
              passwordStrength={passwordStrength}
              isVisible={isVisible}
            />
          </BarProgressTooltipContainer>
        </BarProgress>
      </Bar>

      <BarMessage criteria={criteria}>
        {percentComplete === 100
          ? getLocale('braveWalletPasswordIsStrong')
          : percentComplete < 50
            ? getLocale('braveWalletPasswordIsWeak')
            : getLocale('braveWalletPasswordIsMediumStrength')
        }
      </BarMessage>
    </BarAndMessageContainer>
  )
}

export default PasswordStrengthBar
