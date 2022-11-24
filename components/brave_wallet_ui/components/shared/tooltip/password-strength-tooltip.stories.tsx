// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { PasswordStrengthTooltip } from './password-strength-tooltip'

export const _PasswordStrengthTooltip = () => {
  return <PasswordStrengthTooltip
    isVisible
    passwordStrength={{ isLongEnough: true }}
  >
    Hover
  </PasswordStrengthTooltip>
}

_PasswordStrengthTooltip.storyName = 'Password strength Tooltip'

export const _PasswordStrengthTooltipMini = () => {
  return <div style={{ width: 30 }}>
    <PasswordStrengthTooltip
      isVisible
      passwordStrength={{ isLongEnough: true }}
    >
      Hover
    </PasswordStrengthTooltip>
  </div>
}

_PasswordStrengthTooltipMini.storyName = 'Password strength Tooltip Small'

export default _PasswordStrengthTooltip
