// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import {
  StyledWrapper,
  TooltipContent,
  ArrowUp,
  TooltipText
} from './info-tooltip.style'

interface Props {
  text: string
}

export const InfoTooltip = ({ text }: Props) => {
  return (
    <StyledWrapper>
      <TooltipContent>
        <ArrowUp />
        <TooltipText>{text}</TooltipText>
      </TooltipContent>
    </StyledWrapper>
  )
}
