// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { getLocale } from '../../../../../../common/locale'

import {
  StyledWrapper,
  TooltipContent,
  ArrowDown,
  Heading,
  List
} from './error-tooltip.style'

export const ErrorTooltip = () => {
  return (
    <StyledWrapper>
      <TooltipContent>
        <Heading>{getLocale('braveWalletNftPinningErrorTooltipHeading')}</Heading>
        <List>
          <li>{getLocale('braveWalletNftPinningErrorTooltipReasonOne')}</li>
          <li>{getLocale('braveWalletNftPinningErrorTooltipReasonTwo')}</li>
          <li>{getLocale('braveWalletNftPinningErrorTooltipReasonThree')}</li>
        </List>
        <ArrowDown />
      </TooltipContent>
    </StyledWrapper>
  )
}
