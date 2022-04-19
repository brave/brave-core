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
  BarProgress
} from './password-strength-bar.styles'

interface Props {
  criteria: boolean[]
}

export const PasswordStrengthBar: React.FC<Props> = ({ criteria }) => {
  return (
    <BarAndMessageContainer>
      <Bar>
        <BarBackground />
        <BarProgress criteria={criteria} />
      </Bar>
      <BarMessage criteria={criteria}>
        {criteria.length === criteria.filter(c => !!c).length
          ? getLocale('braveWalletPasswordIsStrong')
          : getLocale('braveWalletPasswordIsWeak')
        }
      </BarMessage>
    </BarAndMessageContainer>
  )
}

export default PasswordStrengthBar
