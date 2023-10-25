// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

export const ChartBalance = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const ChartDate = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  font-size: 11px;
  line-height: 16px;
  color: ${leo.color.text.secondary};
`

interface ToolTipProps {
  labelPosition: 'start' | 'middle' | 'end'
  labelTranslate: number
}

export const TooltipWrapper = styled.div.attrs<ToolTipProps>(
  ({ labelPosition, labelTranslate }) => {
    const labelStartTranslate = `calc(-${labelTranslate}px + 4px)`
    const labelEndTranslate = `translateX(calc(-100% + ${labelTranslate}px))`
    const labelMiddleEndCondition =
      labelPosition === 'end' ? labelEndTranslate : 'translateX(-50%)'
    return {
      style: {
        transform:
          labelPosition === 'start'
            ? labelStartTranslate
            : labelMiddleEndCondition
      }
    }
  }
)<ToolTipProps>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  position: absolute;
  white-space: nowrap;
  background-color: ${leo.color.container.background};
  box-shadow: 0px 4px 16px -1px rgba(0, 0, 0, 0.07);
  border-radius: 4px;
  padding: 2px 4px;
  z-index: 100;
`
