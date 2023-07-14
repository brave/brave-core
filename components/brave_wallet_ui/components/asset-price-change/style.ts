// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'

export const StyledWrapper = styled.span`
  display: flex;
  align-items: center;
`

export const PriceChange = styled.span<{
  isDown: boolean
}>`
  display: flex;
  align-items: center;
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 500;
  line-height: normal;
  letter-spacing: 0.14px;
  color: ${(p) =>
    p.isDown
      ? leo.color.systemfeedback.successIcon
      : leo.color.systemfeedback.errorIcon};
`

export const Arrow = styled(Icon)<{
  isDown: boolean
}>`
  --leo-icon-size: 24px;
  color: ${(p) =>
    p.isDown
      ? leo.color.systemfeedback.successIcon
      : leo.color.systemfeedback.errorIcon};
`
