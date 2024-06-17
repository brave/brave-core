// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import Tooltip, { TooltipProps } from '@brave/leo/react/tooltip'

// style
import { InfoIcon, TooltipTextContent } from './info_icon_tooltip.style'

interface Props {
  text: string
  placement?: TooltipProps['placement']
  maxContentWidth?: string
}

export const InfoIconTooltip = ({
  text,
  placement,
  maxContentWidth
}: Props) => {
  return (
    <Tooltip
      mode='default'
      placement={placement || 'top'}
    >
      <TooltipTextContent
        maxWidth={maxContentWidth ?? '248px'}
        slot='content'
      >
        {text}
      </TooltipTextContent>
      <InfoIcon />
    </Tooltip>
  )
}
